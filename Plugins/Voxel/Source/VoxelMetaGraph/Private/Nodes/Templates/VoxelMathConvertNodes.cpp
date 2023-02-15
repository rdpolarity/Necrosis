// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/Templates/VoxelMathConvertNodes.h"
#include "VoxelMetaGraphGraph.h"

FVoxelTemplateNode::FPin* FVoxelTemplateNode_AbstractMathConvert::ExpandPins(FNode& Node, TArray<FPin*> Pins, const TArray<FPin*>& AllPins) const
{
	UScriptStruct* NodeStruct = GetInt32InnerNode();
	if (IsPinFloat(&Node.GetOutputPin(0)))
	{
		NodeStruct = GetFloatInnerNode();
	}

	const int32 MaxDimension = GetMaxDimension(Pins);

	Pins = Apply(Pins, ScalarToVector, MaxDimension);
	Pins = Apply(Pins, ZeroExpandVector, MaxDimension);
	check(Pins.Num() == 1);

	const TArray<TArray<FPin*>> BrokenPins = ApplyVector(Pins, BreakVector);
	check(BrokenPins.Num() == 1);

	return MakeVector(Call_Multi(NodeStruct, BrokenPins));
}

FVoxelPinTypeSet FVoxelTemplateNode_AbstractMathConvert::GetPromotionTypes(const FVoxelPin& Pin) const
{
	FVoxelPinTypeSet OutTypes;

	OutTypes.Add<float>();
	OutTypes.Add<FVector2D>();
	OutTypes.Add<FVector>();

	OutTypes.Add<FVoxelFloatBuffer>();
	OutTypes.Add<FVoxelVector2DBuffer>();
	OutTypes.Add<FVoxelVectorBuffer>();

	if (Pin.Name == ResultPin)
	{
		OutTypes.Add<int32>();
		OutTypes.Add<FIntPoint>();
		OutTypes.Add<FIntVector>();

		OutTypes.Add<FVoxelInt32Buffer>();
		OutTypes.Add<FVoxelIntPointBuffer>();
		OutTypes.Add<FVoxelIntVectorBuffer>();
	}

	return OutTypes;
}

void FVoxelTemplateNode_AbstractMathConvert::PromotePin(FVoxelPin& InPin, const FVoxelPinType& NewType)
{
	InPin.SetType(NewType);

	if (InPin.Name == ResultPin)
	{
		FVoxelPin& TargetPin = GetPin(ValuePin);
		TargetPin.SetType(IsFloat(NewType) ? NewType : GetFloatType(NewType));
		return;
	}

	FVoxelPin& TargetPin = GetPin(ResultPin);
	TargetPin.SetType(GetIntType(NewType));
}