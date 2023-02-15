// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelExecCodeGenNode.h"
#include "Nodes/VoxelPositionNodes.h"
#include "Nodes/VoxelPassthroughNodes.h"
#include "VoxelNodeCodeGen.h"
#include "VoxelMetaGraphCompilerUtilities.h"

TVoxelFunction<FVoxelFutureValue(const FVoxelQuery&)> FVoxelNode_ExecCodeGen::Compile(FName PinName) const
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_USE_NAMESPACE(MetaGraph);
	ensure(InputPinRefs.Num() == GraphInputPins.Num());
	check(GraphOutputPin);
	check(PinName == "Output");

	FState State;

	TMap<const FPin*, int32> PinToRegister;
	for (const FPin* InputPin : GraphInputPins)
	{
		const int32 RegisterWidth = FVoxelNodeCodeGen::GetRegisterWidth(InputPin->Type);
		const FVoxelPinType RegisterType = FVoxelNodeCodeGen::GetRegisterType(InputPin->Type);

		const int32 Register = State.RegisterTypes.Num();
		for (int32 Index = 0; Index < RegisterWidth; Index++)
		{
			State.RegisterTypes.Add(RegisterType);
		}

		PinToRegister.Add(InputPin, Register);
	}

	const TArray<FNode*> SortedNodes = FCompilerUtilities::SortNodes(Graph->GetNodesArray());
	for (int32 NodeIndex = 0; NodeIndex < SortedNodes.Num(); NodeIndex++)
	{
		const FNode& Node = *SortedNodes[NodeIndex];
		const FVoxelNode& Struct = *Node.Struct();
		check(Struct.IsCodeGen());

		FStep& Step = State.Steps.Emplace_GetRef();
		Step.NodeId = FVoxelNodeCodeGen::GetNodeId(Struct.GetStruct());
		Step.bIsPassthrough = Struct.IsA<FVoxelNode_Passthrough>();

		for (const FVoxelPin& Pin : Struct.GetPins())
		{
			const int32 RegisterWidth = FVoxelNodeCodeGen::GetRegisterWidth(Pin.GetType());
			const FVoxelPinType RegisterType = FVoxelNodeCodeGen::GetRegisterType(Pin.GetType());

			if (Pin.bIsInput)
			{
				const FPin& InputPin = Node.FindInputChecked(Pin.Name);
				if (InputPin.GetLinkedTo().Num() == 0 &&
					!GraphInputPins.Contains(&InputPin))
				{
					const int32 Register = State.RegisterTypes.Num();
					for (int32 Index = 0; Index < RegisterWidth; Index++)
					{
						State.RegisterTypes.Add(RegisterType);
					}

					FVoxelPinValue DefaultValue;
					if (InputPin.GetDefaultValue().IsBuffer())
					{
						DefaultValue = InputPin.GetDefaultValue();
						check(DefaultValue.Get<FVoxelBuffer>().Num() > 0);
					}
					else
					{
						DefaultValue = FVoxelPinValue(InputPin.GetDefaultValue().GetType().GetBufferType());
						DefaultValue.Get<FVoxelBuffer>().InitializeFromConstant(FVoxelSharedPinValue(InputPin.GetDefaultValue()));
					}

					int32 Index = 0;
					DefaultValue.Get<FVoxelBuffer>().MakeGenericView().Get_CheckCompleted().ForeachBufferView([&](const FVoxelTerminalBufferView& BufferView)
					{
						FBuffer Buffer;
						Buffer.InnerType = RegisterType;
						Buffer.Num = 1;
						Buffer.Data = BufferView.GetByteArray();
						State.DefaultBuffers.Add(Register + Index, Buffer);

						Step.InputRegisters.Add(Register + Index);

						Index++;
					});
					check(Index == RegisterWidth);
				}
				else
				{
					const int32 Register = PinToRegister[&InputPin];
					check(RegisterType == State.RegisterTypes[Register]);

					for (int32 Index = 0; Index < RegisterWidth; Index++)
					{
						Step.InputRegisters.Add(Register + Index);
					}
				}
			}
			else
			{
				const int32 Register = State.RegisterTypes.Num();
				for (int32 Index = 0; Index < RegisterWidth; Index++)
				{
					State.RegisterTypes.Add(RegisterType);
				}

				const FPin& OutputPin = Node.FindOutputChecked(Pin.Name);
				for (const FPin& OtherPin : OutputPin.GetLinkedTo())
				{
					PinToRegister.Add(&OtherPin, Register);
				}

				if (&OutputPin == GraphOutputPin)
				{
					check(State.OutputRegisters.Num() == 0);
					for (int32 Index = 0; Index < RegisterWidth; Index++)
					{
						State.OutputRegisters.Add(Register + Index);
					}
				}

				for (int32 Index = 0; Index < RegisterWidth; Index++)
				{
					Step.OutputRegisters.Add(Register + Index);
				}
			}
		}
	}

	const TSharedRef<FState> SharedState = MakeSharedCopy(State);

	ENQUEUE_RENDER_COMMAND(InitializeDefaultBuffers)([=](FRHICommandListImmediate& RHICmdList)
	{
		for (auto& It : SharedState->DefaultBuffers)
		{
			FBuffer& Buffer = It.Value;
			const EPixelFormat Format = Buffer.InnerType.GetPixelFormat();

			FVoxelResourceArrayRef ResourceArray(*Buffer.Data);
			Buffer.GpuBuffer = FVoxelRDGExternalBuffer::Create(*Buffer.Data, Format, TEXT("FVoxelExecCodeGen_Default"));
		}
	});

	return [=, Outer = GetOuter()](const FVoxelQuery& Query) -> FVoxelFutureValue
	{
		(void)Outer;

		const bool bIsBuffer = GetPin(OutputPinRef).GetType().IsBuffer();
		for (int32 Index = 0; Index < GraphInputPins.Num(); Index++)
		{
			if (!ensure(bIsBuffer == GetPin(InputPinRefs[Index]).GetType().IsBuffer()))
			{
				return {};
			}
		}

		if (Query.IsGpu() && bIsBuffer)
		{
			return ExecuteGpu(Query, SharedState);
		}
		else
		{
			return ExecuteCpu(Query, bIsBuffer, SharedState);
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelFutureValue FVoxelNode_ExecCodeGen::ExecuteGpu(
	const FVoxelQuery& Query, 
	const TSharedRef<const FState>& State) const
{
	VOXEL_FUNCTION_COUNTER();

	TArray<TValue<FVoxelBuffer>> InputValues;
	for (int32 Index = 0; Index < GraphInputPins.Num(); Index++)
	{
		InputValues.Add(GetNodeRuntime().Get<FVoxelBuffer>(InputPinRefs[Index], Query));
	}

	VOXEL_SETUP_ON_COMPLETE(OutputPinRef);

	return VOXEL_ON_COMPLETE(RenderThread, State, InputValues)
	{
		TVoxelArray<TSharedPtr<const FVoxelBuffer>> InputValuesPtrs;
		for (const TSharedRef<const FVoxelBuffer>& InputValue : InputValues)
		{
			InputValuesPtrs.Add(InputValue);
		}

		return ExecuteGpu(GraphBuilder, InputValuesPtrs, State);
	};
}

FVoxelSharedPinValue FVoxelNode_ExecCodeGen::ExecuteGpu(
	FRDGBuilder& GraphBuilder,
	const TVoxelArray<TSharedPtr<const FVoxelBuffer>>& InputValues,
	const TSharedRef<const FState>& State) const
{
	VOXEL_FUNCTION_COUNTER();

	int32 Num;
	if (!CheckBufferSizes(InputValues, Num))
	{
		return {};
	}

	TVoxelArray<TSharedPtr<FBuffer>> Buffers;
	for (const FVoxelPinType& Type : State->RegisterTypes)
	{
		const TSharedRef<FBuffer> Buffer = MakeShared<FBuffer>();
		Buffer->InnerType = Type;
		Buffer->Num = Num;
		Buffers.Add(Buffer);
	}
	for (const auto& It : State->DefaultBuffers)
	{
		FBuffer& Buffer = *Buffers[It.Key];
		Buffer = It.Value;
		Buffer.RDGBuffer = FVoxelRDGBuffer(Buffer.GpuBuffer);
	}

	{
		int32 RegisterIndex = 0;
		for (const TSharedPtr<const FVoxelBuffer>& InputValue : InputValues)
		{
			InputValue->ForeachBuffer([&](const FVoxelTerminalBuffer& TerminalBuffer)
			{
				FBuffer& Buffer = *Buffers[RegisterIndex++];
				Buffer.Num = TerminalBuffer.Num();
				Buffer.RDGBuffer = TerminalBuffer.GetGpuBuffer();
			});
		}
	}

	for (const FStep& Step : State->Steps)
	{
		if (Step.bIsPassthrough)
		{
			check(Step.InputRegisters.Num() >= Step.OutputRegisters.Num());
			for (int32 Index = 0; Index < Step.OutputRegisters.Num(); Index++)
			{
				Buffers[Step.OutputRegisters[Index]] = Buffers[Step.InputRegisters[Index]];
			}
			continue;
		}

		TVoxelArray<FVoxelNodeCodeGen::FGpuBuffer> BuffersIn;
		TVoxelArray<FVoxelNodeCodeGen::FGpuBuffer> BuffersOut;
		for (const int32 Register : Step.InputRegisters)
		{
			const FBuffer& Buffer = *Buffers[Register];
			BuffersIn.Add({ Buffer.Num, Buffer.RDGBuffer });
		}
		for (const int32 Register : Step.OutputRegisters)
		{
			FBuffer& Buffer = *Buffers[Register];
			if (!Buffer.RDGBuffer)
			{
				if (Buffer.InnerType.Is<bool>())
				{
					Buffer.RDGBuffer = FVoxelRDGBuffer(PF_R8_UINT, FRDGBufferDesc::CreateBufferDesc(sizeof(bool), Num), TEXT("TempBuffer"));
				}
				else if (Buffer.InnerType.Is<uint8>())
				{
					Buffer.RDGBuffer = FVoxelRDGBuffer(PF_R8_UINT, FRDGBufferDesc::CreateBufferDesc(sizeof(uint8), Num), TEXT("TempBuffer"));
				}
				else if (Buffer.InnerType.Is<float>())
				{
					Buffer.RDGBuffer = FVoxelRDGBuffer(PF_R32_FLOAT, FRDGBufferDesc::CreateBufferDesc(sizeof(float), Num), TEXT("TempBuffer"));
				}
				else
				{
					check(Buffer.InnerType.Is<int32>());
					Buffer.RDGBuffer = FVoxelRDGBuffer(PF_R32_SINT, FRDGBufferDesc::CreateBufferDesc(sizeof(int32), Num), TEXT("TempBuffer"));
				}
			}
			BuffersOut.Add({ Buffer.Num, Buffer.RDGBuffer });
		}

		FVoxelNodeCodeGen::ExecuteGpu(GraphBuilder, Step.NodeId, BuffersIn, BuffersOut, Num);
	}
	
	FVoxelPinValue ReturnValue = FVoxelPinValue(GraphOutputPin->Type.GetBufferType());

	int32 RegisterIndex = 0;
	ReturnValue.Get<FVoxelBuffer>().ForeachBuffer([&](FVoxelTerminalBuffer& OutBuffer)
	{
		const FBuffer& Buffer = *Buffers[State->OutputRegisters[RegisterIndex++]];
		OutBuffer = FVoxelBufferData::MakeGpu(Buffer.InnerType, Buffer.RDGBuffer);
	});
	check(ReturnValue.Get<FVoxelBuffer>().Num() == Num);

	return FVoxelSharedPinValue(ReturnValue);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelFutureValue FVoxelNode_ExecCodeGen::ExecuteCpu(
	const FVoxelQuery& Query,
	const bool bIsBuffer,
	const TSharedRef<const FState>& State) const
{
	VOXEL_FUNCTION_COUNTER();

	TArray<TValue<FVoxelBufferView>> InputValues;
	for (int32 Index = 0; Index < GraphInputPins.Num(); Index++)
	{
		const FVoxelPinRef& Pin = InputPinRefs[Index];
		if (bIsBuffer)
		{
			InputValues.Add(GetNodeRuntime().GetBufferView(Pin, Query));
		}
		else
		{
			const FVoxelFutureValue Value = GetNodeRuntime().Get(Pin, Query);

			InputValues.Add(TValue<FVoxelBufferView>(FVoxelTask::New<FVoxelBufferView>(
				MakeShared<FVoxelTaskStat>(),
				TEXT("MakeConstantView"),
				EVoxelTaskThread::AnyThread,
				{ Value },
				[Value]
				{
					FVoxelPinValue Result = FVoxelPinValue(Value.Get_CheckCompleted().GetType().GetBufferType());
					Result.Get<FVoxelBuffer>().InitializeFromConstant(Value.Get_CheckCompleted());
					return Result.Get<FVoxelBuffer>().MakeGenericView();
				})));
		}
	}

	VOXEL_SETUP_ON_COMPLETE(OutputPinRef);

	return VOXEL_ON_COMPLETE(AsyncThread, bIsBuffer, State, InputValues)
	{
		TVoxelArray<TSharedPtr<const FVoxelBufferView>> InputValuesPtrs;
		for (const TSharedRef<const FVoxelBufferView>& InputValue : InputValues)
		{
			InputValuesPtrs.Add(InputValue);
		}

		return ExecuteCpu(InputValuesPtrs, bIsBuffer, State);
	};
}

FVoxelSharedPinValue FVoxelNode_ExecCodeGen::ExecuteCpu(
	const TVoxelArray<TSharedPtr<const FVoxelBufferView>>& InputValues,
	const bool bIsBuffer,
	const TSharedRef<const FState>& State) const
{
	VOXEL_FUNCTION_COUNTER();
	
	int32 Num;
	if (!CheckBufferSizes(InputValues, Num))
	{
		return {};
	}

	TVoxelArray<TSharedPtr<FBuffer>> Buffers;
	for (const FVoxelPinType& Type : State->RegisterTypes)
	{
		const TSharedRef<FBuffer> Buffer = MakeShared<FBuffer>();
		Buffer->InnerType = Type;
		Buffer->Num = Num;
		Buffers.Add(Buffer);
	}
	for (const auto& It : State->DefaultBuffers)
	{
		*Buffers[It.Key] = It.Value;
	}

	{
		int32 RegisterIndex = 0;
		for (const TSharedPtr<const FVoxelBufferView>& InputValue : InputValues)
		{
			InputValue->ForeachBufferView([&](const FVoxelTerminalBufferView& BufferView)
			{
				FBuffer& Buffer = *Buffers[RegisterIndex++];
				Buffer.Num = BufferView.Num();
				Buffer.Data = BufferView.GetByteArray();
			});
		}
	}

	for (const FStep& Step : State->Steps)
	{
		if (Step.bIsPassthrough)
		{
			check(Step.InputRegisters.Num() >= Step.OutputRegisters.Num());
			for (int32 Index = 0; Index < Step.OutputRegisters.Num(); Index++)
			{
				Buffers[Step.OutputRegisters[Index]] = Buffers[Step.InputRegisters[Index]];
			}
			continue;
		}

		TVoxelArray<FVoxelNodeCodeGen::FCpuBuffer> CpuBuffers;
		for (const int32 Register : Step.InputRegisters)
		{
			const FBuffer& Buffer = *Buffers[Register];
			CpuBuffers.Add({ VOXEL_CONST_CAST(Buffer.Data->GetData()), Buffer.Num });
		}
		for (const int32 Register : Step.OutputRegisters)
		{
			FBuffer& Buffer = *Buffers[Register];
			if (!Buffer.Data)
			{
				Buffer.Data = MakeSharedCopy(FVoxelBuffer::AllocateRaw(Num, Buffer.InnerType.GetTypeSize()));
			}
			CpuBuffers.Add({ VOXEL_CONST_CAST(Buffer.Data->GetData()), Buffer.Num });
		}

		FVoxelNodeCodeGen::ExecuteCpu(Step.NodeId, CpuBuffers, Num);
	}
	
	FVoxelPinValue ReturnValue = FVoxelPinValue(GraphOutputPin->Type.GetBufferType());

	int32 RegisterIndex = 0;
	ReturnValue.Get<FVoxelBuffer>().ForeachBuffer([&](FVoxelTerminalBuffer& OutBuffer)
	{
		const FBuffer& Buffer = *Buffers[State->OutputRegisters[RegisterIndex++]];
		OutBuffer = FVoxelBufferData::MakeCpu(Buffer.InnerType, Buffer.Data.ToSharedRef());
	});
	check(ReturnValue.Get<FVoxelBuffer>().Num() == Num);

	if (bIsBuffer)
	{
		return FVoxelSharedPinValue(ReturnValue);
	}
	else
	{
		return ReturnValue.Get<FVoxelBuffer>().MakeGenericView().Get_CheckCompleted().GetGenericConstant();
	}
}

template<typename T>
bool FVoxelNode_ExecCodeGen::CheckBufferSizes(
	const TVoxelArray<TSharedPtr<const T>>& InputValues,
	int32& Num) const
{
	check(InputValues.Num() == GraphInputPins.Num());

	Num = 1;
	int32 NumIndex = 0;

	for (int32 Index = 0; Index < InputValues.Num(); Index++)
	{
		const int32 BufferNum = InputValues[Index]->Num();

		if (BufferNum == 1 ||
			BufferNum == Num)
		{
			continue;
		}

		if (Num == 1)
		{
			Num = BufferNum;
			NumIndex = Index;
			continue;
		}

		VOXEL_MESSAGE(Error, "{0}: {1}, {2}: Inputs have different buffer sizes",
			this,
			GraphInputPins[Index]->Node,
			GraphInputPins[NumIndex]->Node);

		return false;
	}

	return true;
}