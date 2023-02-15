// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/Templates/VoxelLerpNodes.h"
#include "VoxelMetaGraphGraph.h"

FVoxelTemplateNode::FPin* FVoxelTemplateNode_LerpBase::ExpandPins(FNode& Node, TArray<FPin*> Pins, const TArray<FPin*>& AllPins) const
{
	const int32 NumPins = Pins.Num();
	const int32 MaxDimension = GetDimension(Node.GetOutputPin(0).Type);

	Pins = Apply(Pins, ConvertToFloat);
	Pins = Apply(Pins, ScalarToVector, MaxDimension);
	Pins = Apply(Pins, ZeroExpandVector, MaxDimension);
	check(Pins.Num() == NumPins);

	const TArray<TArray<FPin*>> BrokenPins = ApplyVector(Pins, BreakVector);
	check(BrokenPins.Num() == NumPins);

	return MakeVector(Call_Multi(GetInnerNode(), BrokenPins));
}

FVoxelPinTypeSet FVoxelTemplateNode_LerpBase::GetPromotionTypes(const FVoxelPin& Pin) const
{
	FVoxelPinTypeSet OutTypes;

	OutTypes.Add(GetFloatTypes());

	if (Pin.Name == ResultPin)
	{
		return OutTypes;
	}

	OutTypes.Add(GetIntTypes());

	return OutTypes;
}

void FVoxelTemplateNode_LerpBase::PromotePin(FVoxelPin& InPin, const FVoxelPinType& NewType)
{
	if (InPin.GetType() == NewType)
	{
		return;
	}

	const bool bOnlyContainerChange = InPin.GetType().GetInnerType() == NewType.GetInnerType();
	const bool bIsBufferType = NewType.IsDerivedFrom<FVoxelBuffer>();

	if (bOnlyContainerChange)
	{
		for (FVoxelPin& Pin : GetPins())
		{
			Pin.SetType(bIsBufferType ? Pin.GetType().GetBufferType() : Pin.GetType().GetInnerType());
		}
		return;
	}

	const FVoxelPinType ResultPinType = GetFloatType(NewType);
	const int32 NewTypeDimension = GetDimension(NewType);
	for (FVoxelPin& Pin : GetPins())
	{
		const int32 PinDimension = GetDimension(Pin.GetType());
		FVoxelPinType TargetType = Pin.Name == ResultPin ? ResultPinType : NewType;

		if (Pin.Name == InPin.Name)
		{
			Pin.SetType(TargetType);
		}
		else if (Pin.GetType().IsWildcard())
		{
			if (Pin.Name == AlphaPin)
			{
				Pin.SetType(bIsBufferType ? FVoxelPinType::Make<FVoxelFloatBuffer>() : FVoxelPinType::Make<float>());
			}
			else
			{
				Pin.SetType(TargetType);
			}
		}
		else if (
			PinDimension == NewTypeDimension ||
			(PinDimension == 1 && Pin.Name != ResultPin) ||
			(NewTypeDimension == 1 && InPin.Name != ResultPin))
		{
			Pin.SetType(bIsBufferType ? Pin.GetType().GetBufferType() : Pin.GetType().GetInnerType());
		}
		else
		{
			Pin.SetType(TargetType);
		}
	}
}