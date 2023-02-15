// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/Templates/VoxelOperatorNodes.h"

FVoxelTemplateNode::FPin* FVoxelTemplateNode_OperatorBase::ExpandPins(FNode& Node, TArray<FPin*> Pins, const TArray<FPin*>& AllPins) const
{
	const int32 NumPins = Pins.Num();
	const int32 MaxDimension = GetMaxDimension(Pins);

	UScriptStruct* NodeStruct = GetFloatInnerNode();
	UScriptStruct* IntNodeStruct = GetInt32InnerNode();
	if (All(AllPins, IsPinInt) &&
		IntNodeStruct)
	{
		NodeStruct = IntNodeStruct;
	}
	else
	{
		Pins = Apply(Pins, ConvertToFloat);
	}

	Pins = Apply(Pins, ScalarToVector, MaxDimension);
	Pins = Apply(Pins, ZeroExpandVector, MaxDimension);
	check(Pins.Num() == NumPins);

	if (NumPins == 1)
	{
		return MakeVector(Call_Multi(NodeStruct, BreakVector(Pins[0])));
	}

	return Reduce(Pins, [&](FPin* PinA, FPin* PinB)
	{
		return MakeVector(Call_Multi(NodeStruct, BreakVector(PinA), BreakVector(PinB)));
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelPinTypeSet FVoxelTemplateNode_OperatorBase::GetPromotionTypes(const FVoxelPin& Pin) const
{
	FVoxelPinTypeSet OutTypes;

	if (GetFloatInnerNode())
	{
		OutTypes.Add(GetFloatTypes());
	}

	if (GetInt32InnerNode())
	{
		OutTypes.Add(GetIntTypes());
	}

	return OutTypes;
}

void FVoxelTemplateNode_OperatorBase::PromotePin(FVoxelPin& InPin, const FVoxelPinType& NewType)
{
	InPin.SetType(NewType);

	if (NewType.IsDerivedFrom<FVoxelBuffer>())
	{
		for (FVoxelPin& Pin : GetPins())
		{
			Pin.SetType(Pin.GetType().GetBufferType());
		}
	}
	else
	{
		for (FVoxelPin& Pin : GetPins())
		{
			Pin.SetType(Pin.GetType().GetInnerType());
		}
	}

	FixupPinTypes(&InPin);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelTemplateNode_OperatorBase::FixupPinTypes(const FVoxelPin* SelectedPin)
{
	VOXEL_FUNCTION_COUNTER();

	int32 TargetDimension = 0;
	if (SelectedPin->bIsInput)
	{
		FVoxelPin* OutputPin = nullptr;
		for (FVoxelPin& Pin : GetPins())
		{
			TargetDimension = FMath::Max(TargetDimension, GetDimension(Pin.GetType()));
			if (!Pin.bIsInput)
			{
				OutputPin = &Pin;
			}

			if (Pin.GetType().IsWildcard())
			{
				Pin.SetType(SelectedPin->GetType());
			}
		}

		if (OutputPin)
		{
			if (GetDimension(OutputPin->GetType()) < TargetDimension)
			{
				OutputPin->SetType(SelectedPin->GetType());
			}
			else if (
				!IsInt(SelectedPin->GetType()) &&
				IsInt(OutputPin->GetType()))
			{
				OutputPin->SetType(GetFloatType(OutputPin->GetType()));
			}
		}

		return;
	}

	TargetDimension = GetDimension(SelectedPin->GetType());
	const bool bIsOutputInt = IsInt(SelectedPin->GetType());

	for (FVoxelPin& Pin : GetPins())
	{
		if (Pin.GetType().IsWildcard())
		{
			Pin.SetType(SelectedPin->GetType());
			continue;
		}

		if (GetDimension(Pin.GetType()) > TargetDimension)
		{
			Pin.SetType(SelectedPin->GetType());
			continue;
		}

		if (bIsOutputInt &&
			!IsInt(Pin.GetType()))
		{
			Pin.SetType(SelectedPin->GetType());
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelTemplateNode_CommutativeAssociativeOperator::FDefinition::AddInputPin()
{
	AddToCategory(Node.InputPins);

	FVoxelPinType Type;
	for (const FVoxelPin& Pin : Node.GetPins())
	{
		if (Pin.GetType().IsWildcard())
		{
			continue;
		}

		Type = Pin.GetType();
		break;
	}

	if (!Type.IsValid())
	{
		return;
	}

	for (FVoxelPin& Pin : Node.GetPins())
	{
		if (Pin.GetType().IsWildcard())
		{
			Pin.SetType(Type);
		}
	}
}