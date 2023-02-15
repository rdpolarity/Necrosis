// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/Templates/VoxelFilterBufferNodes.h"
#include "VoxelMetaGraphGraph.h"
#include "VoxelBufferUtilities.h"

DEFINE_VOXEL_NODE(FVoxelNode_FilterBuffer, OutValue)
{
	const FVoxelFutureValue ValueBuffer = Get(ValuePin, Query);
	const TValue<TBufferView<bool>> ConditionView = GetBufferView(ConditionPin, Query);
	
	return VOXEL_ON_COMPLETE(AnyThread, ValueBuffer, ConditionView)
	{
		if (ConditionView.IsConstant())
		{
			if (ConditionView.GetConstant())
			{
				return ValueBuffer;
			}
			else
			{
				return {};
			}
		}

		return VOXEL_ON_COMPLETE(AsyncThread, ValueBuffer, ConditionView)
		{
			CheckVoxelBuffersNum(ValueBuffer.Get<FVoxelBuffer>(), ConditionView);

			FVoxelPinValue Result(ValueBuffer.MakeValue());
			Result.Get<FVoxelBuffer>().ForeachBufferPair(ValueBuffer.Get<FVoxelBuffer>(), [&](FVoxelTerminalBuffer& ResultBuffer, const FVoxelTerminalBuffer& ValueTerminalBuffer)
			{
				ResultBuffer = FVoxelBufferUtilities::Filter_Cpu(ValueTerminalBuffer.MakeView().Get_CheckCompleted(), ConditionView);
			});
			return FVoxelSharedPinValue(Result);
		};
	};
}

FVoxelPinTypeSet FVoxelNode_FilterBuffer::GetPromotionTypes(const FVoxelPin& Pin) const
{
	FVoxelPinTypeSet OutTypes;
	OutTypes.Add(FVoxelPinType::GetAllBufferTypes());
	return OutTypes;
}

void FVoxelNode_FilterBuffer::PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	GetPin(ValuePin).SetType(NewType);
	GetPin(OutValuePin).SetType(NewType);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelTemplateNode_FilterBuffer::FVoxelTemplateNode_FilterBuffer()
{
	FixupBufferPins();
}

void FVoxelTemplateNode_FilterBuffer::ExpandPins(FNode& Node, TArray<FPin*> Pins, const TArray<FPin*>& AllPins, TArray<FPin*>& OutPins) const
{
	for (int32 Index = 0; Index < NumBufferPins; Index++)
	{
		OutPins.Add(Call_Single<FVoxelNode_FilterBuffer>(Pins[Index], Pins[NumBufferPins]));
	}
}

FVoxelPinTypeSet FVoxelTemplateNode_FilterBuffer::GetPromotionTypes(const FVoxelPin& Pin) const
{
	FVoxelPinTypeSet OutTypes;
	OutTypes.Add(FVoxelPinType::GetAllBufferTypes());
	return OutTypes;
}

void FVoxelTemplateNode_FilterBuffer::PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	const int32 PinIndex = (Pin.bIsInput ? BufferInputPins : BufferOutputPins).IndexOfByPredicate(
		[this, &Pin](const FVoxelPinRef& TargetPin)
		{
			return Pin.Name == TargetPin;
		});

	if (!ensure(PinIndex != -1))
	{
		return;
	}

	if (!ensure(
		BufferInputPins.Num() == NumBufferPins &&
		BufferOutputPins.Num() == NumBufferPins))
	{
		FixupBufferPins();
		return;
	}

	GetPin(BufferInputPins[PinIndex]).SetType(NewType);
	GetPin(BufferOutputPins[PinIndex]).SetType(NewType);

	for (int32 Index = 0; Index < NumBufferPins; Index++)
	{
		if (!GetPin(BufferInputPins[Index]).GetType().IsWildcard() ||
			!GetPin(BufferOutputPins[Index]).GetType().IsWildcard())
		{
			continue;
		}

		GetPin(BufferInputPins[Index]).SetType(NewType);
		GetPin(BufferOutputPins[Index]).SetType(NewType);
	}
}

void FVoxelTemplateNode_FilterBuffer::PostSerialize()
{
	FixupBufferPins();

	FVoxelTemplateNode::PostSerialize();
}

#if WITH_EDITOR
void FVoxelTemplateNode_FilterBuffer::GetExternalPinsData(TArray<FName>& OutPinNames, TArray<FName>& OutCategoryNames) const
{
	OutPinNames.Add("Value");
	OutPinNames.Add("Condition");
	OutPinNames.Add("FilteredValue");
}
#endif

void FVoxelTemplateNode_FilterBuffer::FixupBufferPins()
{
	const int32 OldNumBufferPins = BufferInputPins.Num();
	if (!ensure(OldNumBufferPins == BufferOutputPins.Num()))
	{
		return;
	}

	TArray<FVoxelPinType> Types;
	for (int32 Index = 0; Index < OldNumBufferPins; Index++)
	{
		const FVoxelPinRef InputPin = BufferInputPins[Index];
		const FVoxelPinRef OutputPin = BufferOutputPins[Index];

		const FVoxelPinType Type = GetPin(InputPin).GetType();
		ensure(Type == GetPin(OutputPin).GetType());
		Types.Add(Type);

		RemovePin(InputPin);
		RemovePin(OutputPin);
	}

	BufferInputPins.Reset();
	BufferOutputPins.Reset();

	for (int32 Index = 0; Index < NumBufferPins; Index++)
	{
		BufferInputPins.Add(CreateInputPin(
			FVoxelPinType::MakeWildcard(),
			FName("Value", Index + 2),
			{},
			VOXEL_PIN_METADATA(
				DisplayName("Value " + LexToString(Index + 1)),
				Tooltip(GetExternalPinTooltip("Value")))));

		BufferOutputPins.Add(CreateOutputPin(
			FVoxelPinType::MakeWildcard(),
			FName("FilteredValue", Index + 2),
			VOXEL_PIN_METADATA(
				DisplayName("Filtered Value " + LexToString(Index + 1)),
				Tooltip(GetExternalPinTooltip("FilteredValue")))));

		if (Types.IsValidIndex(Index))
		{
			GetPin(BufferInputPins[Index]).SetType(Types[Index]);
			GetPin(BufferOutputPins[Index]).SetType(Types[Index]);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString FVoxelTemplateNode_FilterBuffer::FDefinition::GetAddPinLabel() const
{
	return "Add Value";
}

FString FVoxelTemplateNode_FilterBuffer::FDefinition::GetAddPinTooltip() const
{
	return "Adds an additional buffer which will be filtered by same condition";
}

void FVoxelTemplateNode_FilterBuffer::FDefinition::AddInputPin()
{
	Node.NumBufferPins++;
	Node.FixupBufferPins();
}

bool FVoxelTemplateNode_FilterBuffer::FDefinition::CanRemoveInputPin() const
{
	return Node.NumBufferPins > 1;
}

void FVoxelTemplateNode_FilterBuffer::FDefinition::RemoveInputPin()
{
	Node.NumBufferPins--;
	Node.FixupBufferPins();
}