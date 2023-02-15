// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/Templates/VoxelVectorNodes.h"

FVoxelTemplateNode::FPin* FVoxelTemplateNode_AbstractVectorBase::ExpandPins(FNode& Node, TArray<FPin*> Pins, const TArray<FPin*>& AllPins) const
{
	const int32 NumPins = Pins.Num();
	const int32 MaxDimension = GetMaxDimension(Pins);

	Pins = Apply(Pins, ConvertToFloat);
	Pins = Apply(Pins, ScalarToVector, MaxDimension);
	check(Pins.Num() == NumPins);

	return Call_Single(MaxDimension == 3 ? GetVectorInnerNode() : GetVector2DInnerNode(), Pins);
}

FVoxelPinTypeSet FVoxelTemplateNode_AbstractVectorBase::GetPromotionTypes(const FVoxelPin& Pin) const
{
	FVoxelPinTypeSet OutTypes;

	if (Pin.Name == ReturnValuePin)
	{
		OutTypes.Add(GetVectorResultType());
		OutTypes.Add(GetVectorResultType().GetBufferType());

		OutTypes.Add(GetVector2DResultType());
		OutTypes.Add(GetVector2DResultType().GetBufferType());

		return OutTypes;
	}

	OutTypes.Add<FVector2D>();
	OutTypes.Add<FVector>();
	OutTypes.Add<FIntPoint>();
	OutTypes.Add<FIntVector>();

	OutTypes.Add<FVoxelVector2DBuffer>();
	OutTypes.Add<FVoxelVectorBuffer>();
	OutTypes.Add<FVoxelIntPointBuffer>();
	OutTypes.Add<FVoxelIntVectorBuffer>();

	return OutTypes;
}

void FVoxelTemplateNode_AbstractVectorBase::PromotePin(FVoxelPin& InPin, const FVoxelPinType& NewType)
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

	FVoxelPinType InputType;
	FVoxelPinType ResultType;

	if (InPin.Name == ReturnValuePin)
	{
		InputType = NewType.GetInnerType() == GetVectorResultType() ? FVoxelPinType::Make<FVector>() : FVoxelPinType::Make<FVector2D>();
		ResultType = NewType;
	}
	else
	{
		InputType = NewType;
		ResultType = IsVector(NewType) ? GetVectorResultType() : GetVector2DResultType();
	}

	if (NewType.IsDerivedFrom<FVoxelBuffer>())
	{
		InputType = InputType.GetBufferType();
		ResultType = ResultType.GetBufferType();
	}

	const int32 NewTypeDimension = GetDimension(InputType);
	for (FVoxelPin& Pin : GetPins())
	{
		if (!Pin.bIsInput)
		{
			Pin.SetType(ResultType);
			continue;
		}

		if (Pin.Name == InPin.Name)
		{
			Pin.SetType(NewType);
		}
		else if (GetDimension(Pin.GetType()) == NewTypeDimension)
		{
			Pin.SetType(bIsBufferType ? Pin.GetType().GetBufferType() : Pin.GetType().GetInnerType());
		}
		else
		{
			Pin.SetType(InputType);
		}
	}
}

bool FVoxelTemplateNode_AbstractVectorBase::IsVector(const FVoxelPinType& Type)
{
	if (Type.IsDerivedFrom<FVoxelBuffer>())
	{
		return IsVector(Type.GetInnerType());
	}

	return Type.Is<FVector>() || Type.Is<FIntVector>();
}