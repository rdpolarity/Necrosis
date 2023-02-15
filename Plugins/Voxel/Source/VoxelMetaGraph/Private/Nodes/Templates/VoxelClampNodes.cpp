// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/Templates/VoxelClampNodes.h"
#include "VoxelMetaGraphGraph.h"

FVoxelTemplateNode::FPin* FVoxelTemplateNode_AbstractClampBase::ExpandPins(FNode& Node, TArray<FPin*> Pins, const TArray<FPin*>& AllPins) const
{
	const int32 NumPins = Pins.Num();
	const int32 MaxDimension = GetDimension(Node.GetOutputPin(0).Type);

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

	const TArray<TArray<FPin*>> BrokenPins = ApplyVector(Pins, BreakVector);
	check(BrokenPins.Num() == NumPins);

	return MakeVector(Call_Multi(NodeStruct, BrokenPins));
}

FVoxelPinTypeSet FVoxelTemplateNode_AbstractClampBase::GetPromotionTypes(const FVoxelPin& Pin) const
{
	FVoxelPinTypeSet OutTypes;

	OutTypes.Add(GetFloatTypes());

	if (Pin.Name == ResultPin &&
		!GetInt32InnerNode())
	{
		return OutTypes;
	}

	OutTypes.Add(GetIntTypes());

	return OutTypes;
}

void FVoxelTemplateNode_AbstractClampBase::PromotePin(FVoxelPin& InPin, const FVoxelPinType& NewType)
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

	const int32 NewTypeDimension = GetDimension(NewType);

	{
		FVoxelPin& Pin = GetPin(ResultPin);
		const int32 PinDimension = GetDimension(Pin.GetType());
		if (InPin.Name == ResultPin ||
			Pin.GetType().IsWildcard())
		{
			Pin.SetType(NewType);
		}
		else if (
			PinDimension == NewTypeDimension ||
			NewTypeDimension == 1)
		{
			const FVoxelPinType Type = IsFloat(NewType) && IsInt(Pin.GetType()) ? GetFloatType(Pin.GetType()) : Pin.GetType();
			Pin.SetType(bIsBufferType ? Type.GetBufferType() : Type.GetInnerType());
		}
		else
		{
			Pin.SetType(NewType);
		}
	}

	const bool bResultIsInt = IsInt(GetPin(ResultPin).GetType());

	for (FVoxelPin& Pin : GetPins())
	{
		if (Pin.Name == ResultPin)
		{
			continue;
		}

		const int32 PinDimension = GetDimension(Pin.GetType());
		if (InPin.Name == Pin.Name)
		{
			Pin.SetType(NewType);
		}
		else if (Pin.GetType().IsWildcard())
		{
			if (Pin.Name != ValuePin)
			{
				const FVoxelPinType Type = IsFloat(NewType) ? FVoxelPinType::Make<float>() : FVoxelPinType::Make<int32>();
				Pin.SetType(bIsBufferType ? Type.GetBufferType() : Type.GetInnerType());
			}
			else
			{
				Pin.SetType(NewType);
			}
		}
		else if (
			PinDimension == NewTypeDimension ||
			PinDimension == 1 ||
			NewTypeDimension == 1)
		{
			const FVoxelPinType Type = bResultIsInt ? GetIntType(Pin.GetType()) : Pin.GetType();
			Pin.SetType(bIsBufferType ? Type.GetBufferType() : Type.GetInnerType());
		}
		else
		{
			Pin.SetType(NewType);
		}
	}
}