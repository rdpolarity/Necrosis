// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelBuffer.h"

TSharedRef<FVoxelBufferData> FVoxelBufferData::MakeConstant(const FVoxelSharedPinValue& Constant)
{
	TVoxelArray<uint8> Data(Constant.MakeByteView());
	Data.Reserve(Constant.GetType().GetTypeSize() * 8);
	return MakeCpu(Constant.GetType(), MakeShared<TVoxelArray<uint8>>(MoveTemp(Data)));
}

TSharedRef<FVoxelBufferData> FVoxelBufferData::MakeCpu(const FVoxelPinType& InnerType, const TSharedRef<const TVoxelArray<uint8>>& Data)
{
	const int32 TypeSize = InnerType.GetTypeSize();
	ensure(Data->Num() % TypeSize == 0);
	ensure(FVoxelBuffer::AlignNum(Data->Num() / TypeSize) * TypeSize <= Data->Max());

	const TSharedRef<FVoxelBufferData> Result = MakeShared<FVoxelBufferData>();
	Result->PrivateInnerType = InnerType;
	Result->PrivateNum = Data->Num() / TypeSize;

	Result->bHasCpuView = true;
	Result->CpuView = MakeShared<TVoxelFutureValue<FVoxelTerminalBufferView>>(
		FVoxelSharedPinValue::Make(
			FVoxelTerminalBufferView(InnerType, Data),
			InnerType.GetViewType()));

	return Result;
}

TSharedRef<FVoxelBufferData> FVoxelBufferData::MakeGpu(const FVoxelPinType& InnerType, const FVoxelRDGBuffer& Buffer)
{
	check(Buffer.IsValid());
	ensure(Buffer.GetBuffer()->Desc.BytesPerElement == InnerType.GetTypeSize());

	const TSharedRef<FVoxelBufferData> Result = MakeShared<FVoxelBufferData>();
	Result->PrivateInnerType = InnerType;
	Result->PrivateNum = Buffer.Num();

	Result->bHasGpuBuffer = true;
	Result->GpuBuffer = MakeShared<FGpuBuffer>();
	Result->GpuBuffer->SetRDGBuffer(Buffer);

	GVoxelTaskProcessor->AddOnRenderThreadComplete([Result, Buffer](FRDGBuilder& GraphBuilder)
	{
		if (Result.GetSharedReferenceCount() == 1)
		{
			return;
		}

		const TSharedRef<TRefCountPtr<FRDGPooledBuffer>> ExtractedBuffer = MakeShared<TRefCountPtr<FRDGPooledBuffer>>();
		GraphBuilder.QueueBufferExtraction(Buffer, &ExtractedBuffer.Get());
		Result->GpuBuffer->SetExtractedBuffer(ExtractedBuffer, Buffer.GetFormat());
	});

	return Result;
}

TVoxelFutureValue<FVoxelTerminalBufferView> FVoxelBufferData::MakeView() const
{
	if (bHasCpuView)
	{
		check(CpuView);
		return *CpuView;
	}
	check(bHasGpuBuffer);
	check(GpuBuffer);

	VOXEL_FUNCTION_COUNTER();

	static TVoxelKeyedCriticalSection<const FVoxelBufferData*> CriticalSection;
	TVoxelKeyedScopeLock<const FVoxelBufferData*> Lock(CriticalSection, this);

	if (!CpuView)
	{
		const FVoxelPinType ViewType = GetInnerType().GetViewType();

		CpuView = MakeSharedCopy(TVoxelFutureValue<FVoxelTerminalBufferView>(FVoxelTask::New(
			MakeShared<FVoxelTaskStat>(),
			ViewType,
			"Readback",
			EVoxelTaskThread::RenderThread,
			{},
			[this, ViewType, This = AsShared()]
			{
				const TSharedRef<FVoxelFuturePinValueState> State = MakeShared<FVoxelFuturePinValueState>(ViewType);

				const TSharedRef<FVoxelGPUBufferReadback> Readback = FVoxelRenderUtilities::CopyBuffer(FVoxelRDGBuilderScope::Get(), GetGpuBuffer());
				FVoxelRenderUtilities::OnReadbackComplete(Readback, [=]
				{
					(void)This;
					State->SetValue(FVoxelSharedPinValue::Make(FVoxelTerminalBufferView(GetInnerType(), MakeSharedCopy(Readback->AsArray<uint8>())), ViewType));
				});

				return FVoxelFutureValue(State);
			})));

		check(!bHasCpuView);
		bHasCpuView = true;
	}

	return *CpuView;
}

struct FVoxelBufferDataBlackboard
{
	mutable TSet<TSharedPtr<FVoxelRDGExternalBuffer>> Buffers;
};
RDG_REGISTER_BLACKBOARD_STRUCT(FVoxelBufferDataBlackboard);

FVoxelRDGBuffer FVoxelBufferData::GetGpuBuffer() const
{
	ensure(IsInRenderingThread());

	// Make sure to keep ExternalBuffer alive until the render graph is executed
	ON_SCOPE_EXIT
	{
		if (GpuBuffer->GetExternalBuffer())
		{
			FRDGBuilder& GraphBuilder = FVoxelRDGBuilderScope::Get();
			if (!GraphBuilder.Blackboard.Get<FVoxelBufferDataBlackboard>())
			{
				GraphBuilder.Blackboard.Create<FVoxelBufferDataBlackboard>();
			}
			GraphBuilder.Blackboard.GetChecked<FVoxelBufferDataBlackboard>().Buffers.Add(GpuBuffer->GetExternalBuffer());
		}
	};

	if (bHasGpuBuffer)
	{
		check(GpuBuffer);
		if (GpuBuffer->GetExternalBuffer())
		{
			ensure(!GpuBuffer->GetRDGBuffer());
			return FVoxelRDGBuffer(GpuBuffer->GetExternalBuffer());
		}
		else
		{
			check(GpuBuffer->GetRDGBuffer());
			return GpuBuffer->GetRDGBuffer();
		}
	}
	check(bHasCpuView);
	check(CpuView);
	check(CpuView->IsComplete());

	VOXEL_FUNCTION_COUNTER();

	static TVoxelKeyedCriticalSection<const FVoxelBufferData*> CriticalSection;
	TVoxelKeyedScopeLock<const FVoxelBufferData*> Lock(CriticalSection, this);

	if (!GpuBuffer)
	{
		GpuBuffer = MakeShared<FGpuBuffer>();

		const double StartTime = FPlatformTime::Seconds();

		const TConstVoxelArrayView<uint8> CpuData = CpuView->Get_CheckCompleted().MakeByteView();
		const EPixelFormat Format = GetInnerType().GetPixelFormat();

		if (Num() == 1)
		{
			using uint128 = TPair<uint64, uint64>;

			static TMap<FVoxelPinType, TMap<uint128, TSharedPtr<FVoxelRDGExternalBuffer>>> PinTypesToConstants;
			TMap<uint128, TSharedPtr<FVoxelRDGExternalBuffer>>& Constants = PinTypesToConstants.FindOrAdd(GetInnerType());

			check(CpuData.Num() <= sizeof(uint128));
			uint128 Value;
			FMemory::Memzero(&Value, sizeof(uint128));
			FMemory::Memcpy(&Value, CpuData.GetData(), CpuData.Num());

			TSharedPtr<FVoxelRDGExternalBuffer>& ExternalBuffer = Constants.FindOrAdd(Value);
			if (!ExternalBuffer)
			{
				FVoxelResourceArrayRef ResourceArray(CpuData);
				ExternalBuffer = FVoxelRDGExternalBuffer::Create(CpuData, Format, TEXT("FVoxelBufferData_Constant"));
			}
			GpuBuffer->SetExternalBuffer(ExternalBuffer.ToSharedRef());
		}
		else
		{
			FVoxelResourceArrayRef ResourceArray(CpuData);
			GpuBuffer->SetExternalBuffer(FVoxelRDGExternalBuffer::Create(CpuData, Format, TEXT("FVoxelBufferData_Uploaded")));
		}

		const double EndTime = FPlatformTime::Seconds();

		const TSharedPtr<FVoxelTaskStat> Stat = FVoxelTaskStat::GetScopeStat();
		if (Stat && Num() > 1) // Don't record constant uploads
		{
			Stat->AddTime(FVoxelTaskStat::CopyCpuToGpu, EndTime - StartTime);
		}

		check(!bHasGpuBuffer);
		bHasGpuBuffer = true;
	}

	return FVoxelRDGBuffer(GpuBuffer->GetExternalBuffer());
}

int64 FVoxelBufferData::GetAllocatedSize() const
{
	int64 AllocatedSize = 0;
	if (bHasCpuView)
	{
		AllocatedSize += Num() * GetInnerType().GetTypeSize();
	}
	if (bHasGpuBuffer)
	{
		AllocatedSize += Num() * GetInnerType().GetTypeSize();
	}
	return AllocatedSize;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelBufferData::FGpuBuffer::SetExternalBuffer(const TSharedRef<FVoxelRDGExternalBuffer>& InExternalBuffer)
{
	ensure(IsInRenderingThread());
	ensure(!ExternalBuffer);
	ensure(!ExtractedBuffer);
	ensure(ExtractedFormat == PF_Unknown);

	ExternalBuffer = InExternalBuffer;
}

void FVoxelBufferData::FGpuBuffer::SetExtractedBuffer(const TSharedPtr<TRefCountPtr<FRDGPooledBuffer>>& InExtractedBuffer, EPixelFormat InExtractedFormat)
{
	ensure(IsInRenderingThread());
	ensure(!ExternalBuffer);
	ensure(!ExtractedBuffer);
	ensure(ExtractedFormat == PF_Unknown);

	ExtractedBuffer = InExtractedBuffer;
	ExtractedFormat = InExtractedFormat;
}

TSharedPtr<FVoxelRDGExternalBuffer> FVoxelBufferData::FGpuBuffer::GetExternalBuffer() const
{
	ensure(IsInRenderingThread());
	if (!ExternalBuffer && 
		ExtractedBuffer &&
		ExtractedBuffer->IsValid())
	{
		ExternalBuffer = FVoxelRDGExternalBuffer::Create(*ExtractedBuffer, ExtractedFormat, TEXT("FVoxelBufferData_Extracted"));
	}
	return ExternalBuffer;
}

void FVoxelBufferData::FGpuBuffer::SetRDGBuffer(const FVoxelRDGBuffer& InRDGBuffer)
{
	ensure(IsInRenderingThread());
	ensure(!RDGBuffer);
	ensure(RDGBuilderId == -1);

	RDGBuffer = InRDGBuffer;
	RDGBuilderId = FVoxelRenderUtilities::GetGraphBuilderId();
}

FVoxelRDGBuffer FVoxelBufferData::FGpuBuffer::GetRDGBuffer() const
{
	ensure(IsInRenderingThread());
	return FVoxelRenderUtilities::GetGraphBuilderId() == RDGBuilderId ? RDGBuffer : FVoxelRDGBuffer();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelTerminalBufferView::ForeachBufferView(TVoxelFunctionRef<void(const FVoxelTerminalBufferView&)> Lambda) const
{
	Lambda(*this);
}

FVoxelRDGBuffer FVoxelTerminalBuffer::GetGpuBuffer() const
{
	if (!Data)
	{
		const FVoxelRDGBuffer Buffer = FVoxelRDGBuffer(
			GetFormat(),
			FRDGBufferDesc::CreateBufferDesc(GPixelFormats[GetFormat()].BlockBytes, 1),
			TEXT("Dummy"));
		AddClearUAVPass(FVoxelRDGBuilderScope::Get(), Buffer, 0);
		return Buffer;
	}

	return Data->GetGpuBuffer();
}

TVoxelFutureValue<FVoxelTerminalBufferView> FVoxelTerminalBuffer::MakeView() const
{
	if (!Data)
	{
		return FVoxelSharedPinValue::Make(
			FVoxelTerminalBufferView(GetInnerType(), MakeShared<TVoxelArray<uint8>>()),
			GetInnerType().GetViewType());
	}

	return Data->MakeView();
}

TOptional<FFloatInterval> FVoxelTerminalBuffer::GetInterval() const
{
	if (!Data)
	{
		return {};
	}

	return Data->GetInterval();
}

void FVoxelTerminalBuffer::SetInterval(const FFloatInterval& Interval)
{
	if (!ensure(Data))
	{
		return;
	}

	Data->SetInterval(Interval);
}

TVoxelFutureValue<FVoxelBufferView> FVoxelTerminalBuffer::MakeGenericView() const
{
	return MakeView();
}

int32 FVoxelTerminalBuffer::Num() const
{
	if (!Data)
	{
		return 0;
	}

	return Data->Num();
}

bool FVoxelTerminalBuffer::Identical(const FVoxelBuffer& Other) const
{
	if (GetStruct() != Other.GetStruct())
	{
		return false;
	}

	// Only check if pointers are equal
	return Data == CastChecked<FVoxelTerminalBuffer>(Other).Data;
}

void FVoxelTerminalBuffer::InitializeFromConstant(const FVoxelSharedPinValue& Constant)
{
	Data = FVoxelBufferData::MakeConstant(Constant);
}

void FVoxelTerminalBuffer::ForeachBuffer(TVoxelFunctionRef<void(FVoxelTerminalBuffer&)> Lambda)
{
	Lambda(*this);
}

void FVoxelTerminalBuffer::ForeachBufferPair(const FVoxelBuffer& Other, TVoxelFunctionRef<void(FVoxelTerminalBuffer&, const FVoxelTerminalBuffer&)> Lambda)
{
	check(GetStruct() == Other.GetStruct());
	Lambda(*this, CastChecked<FVoxelTerminalBuffer>(Other));
}

void FVoxelTerminalBuffer::ForeachBufferArray(TConstVoxelArrayView<const FVoxelBuffer*> Others, TVoxelFunctionRef<void(FVoxelTerminalBuffer&, TConstVoxelArrayView<const FVoxelTerminalBuffer*>)> Lambda)
{
	Lambda(*this, ReinterpretCastVoxelArrayView<const FVoxelTerminalBuffer*>(Others));
}

uint64 FVoxelTerminalBuffer::GetHash() const
{
	return FVoxelUtilities::MurmurHash(Data.Get());
}

int64 FVoxelTerminalBuffer::GetAllocatedSize() const
{
	if (!Data)
	{
		return 0;
	}

	return Data->GetAllocatedSize();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelContainerBufferView::ForeachBufferView(TVoxelFunctionRef<void(const FVoxelTerminalBufferView&)> Lambda) const
{
	for (int32 Index = 0; Index < NumBuffers(); Index++)
	{
		Lambda(*PrivateBuffers[Index]);
	}
}

void FVoxelContainerBufferView::ComputeBuffers() const
{
	check(PrivateBuffers.Data.Num() == 0);
	for (const FProperty& Property : GetStructProperties(GetStruct()))
	{
		const UScriptStruct* Struct = CastFieldChecked<FStructProperty>(Property).Struct;
		check(Struct->IsChildOf(FVoxelBufferView::StaticStruct()));

		if (Struct->IsChildOf(FVoxelTerminalBufferView::StaticStruct()))
		{
			PrivateBuffers.Data.Add(Property.ContainerPtrToValuePtr<FVoxelTerminalBufferView>(VOXEL_CONST_CAST(this)));
		}
		else
		{
			check(Struct->IsChildOf(FVoxelContainerBufferView::StaticStruct()));

			const FVoxelContainerBufferView& Buffer = *Property.ContainerPtrToValuePtr<FVoxelContainerBufferView>(this);
			PrivateBuffers.Data.Append(Buffer.PrivateBuffers.Data);
		}
	}
	check(PrivateBuffers.Data.Num() > 0);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int32 FVoxelContainerBuffer::Num() const
{
	int32 Num = 1;
	ForeachBuffer([&](const FVoxelTerminalBuffer& Buffer)
	{
		const int32 BufferNum = Buffer.Num();

		if (Num == 1)
		{
			Num = BufferNum;
		}
		else
		{
			ensure(BufferNum == Num || BufferNum == 1);
		}
	});
	return Num;
}

bool FVoxelContainerBuffer::IsValid() const
{
	int32 Num = 1;
	bool bValid = true;
	ForeachBuffer([&](const FVoxelTerminalBuffer& Buffer)
	{
		const int32 BufferNum = Buffer.Num();

		if (Num == 1)
		{
			Num = BufferNum;
		}
		else if (BufferNum != Num && BufferNum != 1)
		{
			bValid = false;
		}
	});
	return bValid;
}

bool FVoxelContainerBuffer::Identical(const FVoxelBuffer& Other) const
{
	if (GetStruct() != Other.GetStruct())
	{
		return false;
	}

	bool bIdentical = true;
	ForeachBufferPair(CastChecked<FVoxelContainerBuffer>(Other), [&](const FVoxelTerminalBuffer& Buffer, const FVoxelTerminalBuffer& OtherBuffer)
	{
		bIdentical &= Buffer.Identical(OtherBuffer);
	});
	return bIdentical;
}

int64 FVoxelContainerBuffer::GetAllocatedSize() const
{
	int64 AllocatedSize = GetStruct()->GetStructureSize();
	ForeachBuffer([&](const FVoxelTerminalBuffer& Buffer)
	{
		AllocatedSize += Buffer.GetAllocatedSize();
	});
	return AllocatedSize;
}

void FVoxelContainerBuffer::ForeachBuffer(TVoxelFunctionRef<void(FVoxelTerminalBuffer&)> Lambda)
{
	for (int32 Index = 0; Index < NumBuffers(); Index++)
	{
		Lambda(*PrivateBuffers[Index]);
	}
}

void FVoxelContainerBuffer::ForeachBufferPair(const FVoxelBuffer& InOther, TVoxelFunctionRef<void(FVoxelTerminalBuffer&, const FVoxelTerminalBuffer&)> Lambda)
{
	const FVoxelContainerBuffer& Other = CastChecked<FVoxelContainerBuffer>(InOther);

	checkVoxelSlow(Other.GetStruct() == GetStruct());
	checkVoxelSlow(NumBuffers() == Other.NumBuffers());
	for (int32 Index = 0; Index < NumBuffers(); Index++)
	{
		Lambda(*PrivateBuffers[Index], *Other.PrivateBuffers[Index]);
	}
}

void FVoxelContainerBuffer::ForeachBufferArray(TConstVoxelArrayView<const FVoxelBuffer*> InOthers, TVoxelFunctionRef<void(FVoxelTerminalBuffer&, TConstVoxelArrayView<const FVoxelTerminalBuffer*>)> Lambda)
{
	const TConstVoxelArrayView<const FVoxelContainerBuffer*> Others = ReinterpretCastVoxelArrayView<const FVoxelContainerBuffer*>(InOthers);

	for (const FVoxelContainerBuffer* Other : Others)
	{
		checkVoxelSlow(Other->GetStruct() == GetStruct());
		checkVoxelSlow(NumBuffers() == Other->NumBuffers());
	}

	for (int32 Index = 0; Index < NumBuffers(); Index++)
	{
		TVoxelArray<FVoxelTerminalBuffer*, TInlineAllocator<16>> OtherBuffers;
		for (const FVoxelContainerBuffer* Other : Others)
		{
			OtherBuffers.Add(Other->PrivateBuffers[Index]);
		}

		Lambda(*PrivateBuffers[Index], OtherBuffers);
	}
}

void FVoxelContainerBuffer::ComputeBuffers() const
{
	check(PrivateBuffers.Data.Num() == 0);
	for (const FProperty& Property : GetStructProperties(GetStruct()))
	{
		if (Property.GetFName() == STATIC_FNAME("SerializedValue"))
		{
			continue;
		}

		const UScriptStruct* Struct = CastFieldChecked<FStructProperty>(Property).Struct;
		check(Struct->IsChildOf(FVoxelBuffer::StaticStruct()));

		if (Struct->IsChildOf(FVoxelTerminalBuffer::StaticStruct()))
		{
			PrivateBuffers.Data.Add(Property.ContainerPtrToValuePtr<FVoxelTerminalBuffer>(VOXEL_CONST_CAST(this)));
		}
		else
		{
			check(Struct->IsChildOf(FVoxelContainerBuffer::StaticStruct()));

			const FVoxelContainerBuffer& Buffer = *Property.ContainerPtrToValuePtr<FVoxelContainerBuffer>(this);
			PrivateBuffers.Data.Append(Buffer.PrivateBuffers.Data);
		}
	}
	check(PrivateBuffers.Data.Num() > 0);
}

void FVoxelVectorBuffer::SetBounds(const FVoxelBox& Bounds)
{
	X.SetInterval(FFloatInterval(Bounds.Min.X, Bounds.Max.X));
	Y.SetInterval(FFloatInterval(Bounds.Min.Y, Bounds.Max.Y));
	Z.SetInterval(FFloatInterval(Bounds.Min.Z, Bounds.Max.Z));
}

TOptional<FVoxelBox> FVoxelVectorBuffer::GetBounds() const
{
	const TOptional<FFloatInterval> IntervalX = X.GetInterval();
	const TOptional<FFloatInterval> IntervalY = Y.GetInterval();
	const TOptional<FFloatInterval> IntervalZ = Z.GetInterval();

	if (!IntervalX ||
		!IntervalY ||
		!IntervalZ)
	{
		return {};
	}

	FVoxelBox Bounds;
	Bounds.Min = FVector(IntervalX->Min, IntervalY->Min, IntervalZ->Min);
	Bounds.Max = FVector(IntervalX->Max, IntervalY->Max, IntervalZ->Max);
	return Bounds;
}

template<typename LambdaType>
FORCEINLINE void FVoxelContainerBuffer::ForeachBuffer(LambdaType Lambda) const
{
	for (int32 Index = 0; Index < NumBuffers(); Index++)
	{
		Lambda(*PrivateBuffers[Index]);
	}
}

template<typename LambdaType>
FORCEINLINE void FVoxelContainerBuffer::ForeachBufferPair(const FVoxelContainerBuffer& Other, LambdaType Lambda) const
{
	checkVoxelSlow(Other.GetStruct() == GetStruct());
	checkVoxelSlow(NumBuffers() == Other.NumBuffers());
	for (int32 Index = 0; Index < NumBuffers(); Index++)
	{
		Lambda(*PrivateBuffers[Index], *Other.PrivateBuffers[Index]);
	}
}