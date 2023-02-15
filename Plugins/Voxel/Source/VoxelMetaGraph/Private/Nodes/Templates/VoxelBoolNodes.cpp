// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/Templates/VoxelBoolNodes.h"

#include "VoxelMetaGraphGraph.h"

FVoxelTemplateNode::FPin* FVoxelTemplateNode_EqualityBase::ExpandPins(FNode& Node, TArray<FPin*> Pins, const TArray<FPin*>& AllPins) const
{
	const int32 MaxDimension = GetMaxDimension(Pins);

	UScriptStruct* NodeStruct;
	if (All(Pins, IsPinBool))
	{
		NodeStruct = GetBoolInnerNode();
	}
	else if (All(Pins, IsPinInt))
	{
		NodeStruct = GetInt32InnerNode();
	}
	else
	{
		Pins = Apply(Pins, ConvertToFloat);
		NodeStruct = GetFloatInnerNode();
	}

	Pins = Apply(Pins, ScalarToVector, MaxDimension);
	Pins = Apply(Pins, ZeroExpandVector, MaxDimension);
	check(Pins.Num() == 2);

	const TArray<TArray<FPin*>> BrokenPins = ApplyVector(Pins, BreakVector);
	check(BrokenPins.Num() == 2);

	const TArray<FPin*> BooleanPins = Call_Multi(NodeStruct, BrokenPins);

	return Reduce(BooleanPins, [&](FPin* PinA, FPin* PinB)
	{
		return Call_Single(GetConnectionInnerNode(), PinA, PinB);
	});
}

FVoxelPinTypeSet FVoxelTemplateNode_EqualityBase::GetPromotionTypes(const FVoxelPin& Pin) const
{
	FVoxelPinTypeSet OutTypes;

	if (Pin.Name == ResultPin)
	{
		OutTypes.Add<bool>();
		OutTypes.Add<FVoxelBoolBuffer>();
		return OutTypes;
	}

	if (GetFloatInnerNode())
	{
		OutTypes.Add(GetFloatTypes());
	}
	if (GetInt32InnerNode())
	{
		OutTypes.Add(GetIntTypes());
	}
	if (GetBoolInnerNode())
	{
		OutTypes.Add<bool>();
		OutTypes.Add<FVoxelBoolBuffer>();
	}

	return OutTypes;
}

void FVoxelTemplateNode_EqualityBase::PromotePin(FVoxelPin& InPin, const FVoxelPinType& NewType)
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

	const int32 NewDimension = GetDimension(NewType);
	for (FVoxelPin& Pin : GetPins())
	{
		if (Pin.Name != APin &&
			Pin.Name != BPin)
		{
			Pin.SetType(bIsBufferType ? Pin.GetType().GetBufferType() : Pin.GetType().GetInnerType());
			continue;
		}

		const int32 PinDimension = GetDimension(Pin.GetType());
		if (Pin.GetType().IsWildcard() ||
			IsBool(NewType) != IsBool(Pin.GetType()) ||
			InPin.Name == Pin.Name)
		{
			Pin.SetType(NewType);
		}
		else if (
			PinDimension == NewDimension ||
			PinDimension == 1 ||
			NewDimension == 1)
		{
			Pin.SetType(bIsBufferType ? Pin.GetType().GetBufferType() : Pin.GetType().GetInnerType());
		}
		else
		{
			Pin.SetType(NewType);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelTemplateNode::FPin* FVoxelTemplateNode_MultiInputBooleanNode::ExpandPins(FNode& Node, TArray<FPin*> Pins, const TArray<FPin*>& AllPins) const
{
	return Reduce(Pins, [&](FPin* PinA, FPin* PinB)
	{
		return Call_Single(GetBooleanNode(), PinA, PinB);
	});
}

FVoxelPinTypeSet FVoxelTemplateNode_MultiInputBooleanNode::GetPromotionTypes(const FVoxelPin& Pin) const
{
	FVoxelPinTypeSet OutTypes;
	OutTypes.Add<bool>();
	OutTypes.Add<FVoxelBoolBuffer>();
	return OutTypes;
}

void FVoxelTemplateNode_MultiInputBooleanNode::PromotePin(FVoxelPin& InPin, const FVoxelPinType& NewType)
{
	for (FVoxelPin& Pin : GetPins())
	{
		Pin.SetType(NewType);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelTemplateNode_MultiInputBooleanNode::FDefinition::AddInputPin()
{
	AddToCategory(Node.InputPins);

	const FVoxelPinType BoolType = Node.GetPin(Node.ResultPin).GetType();
	for (FVoxelPin& Pin : Node.GetPins())
	{
		Pin.SetType(BoolType);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelPinTypeSet FVoxelTemplateNode_EqualitySingleDimension::GetPromotionTypes(const FVoxelPin& Pin) const
{
	FVoxelPinTypeSet OutTypes;

	if (Pin.Name == ResultPin)
	{
		OutTypes.Add<bool>();
		OutTypes.Add<FVoxelBoolBuffer>();
		return OutTypes;
	}

	if (GetFloatInnerNode())
	{
		OutTypes.Add<float>();
		OutTypes.Add<FVoxelFloatBuffer>();
	}
	if (GetInt32InnerNode())
	{
		OutTypes.Add<int32>();
		OutTypes.Add<FVoxelInt32Buffer>();
	}

	return OutTypes;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelTemplateNode::FPin* FVoxelTemplateNode_NearlyEqual::ExpandPins(FNode& Node, TArray<FPin*> Pins, const TArray<FPin*>& AllPins) const
{
	const int32 MaxDimension = GetMaxDimension(Pins);

	Pins = Apply(Pins, ScalarToVector, MaxDimension);
	Pins = Apply(Pins, ZeroExpandVector, MaxDimension);
	check(Pins.Num() == 3);

	const TArray<TArray<FPin*>> BrokenPins = ApplyVector(Pins, BreakVector);
	check(BrokenPins.Num() == 3);

	const TArray<FPin*> BooleanPins = Call_Multi<FVoxelNode_NearlyEqual>(BrokenPins);

	return Reduce(BooleanPins, [&](FPin* PinA, FPin* PinB)
	{
		return Call_Single<FVoxelNode_BooleanAND>(PinA, PinB);
	});
}

FVoxelPinTypeSet FVoxelTemplateNode_NearlyEqual::GetPromotionTypes(const FVoxelPin& Pin) const
{
	FVoxelPinTypeSet OutTypes;

	if (Pin.Name == ErrorTolerancePin)
	{
		OutTypes.Add<float>();
		OutTypes.Add<FVoxelFloatBuffer>();
		return OutTypes;
	}

	if (Pin.Name == ResultPin)
	{
		OutTypes.Add<bool>();
		OutTypes.Add<FVoxelBoolBuffer>();
		return OutTypes;
	}

	OutTypes.Add(GetFloatTypes());

	return OutTypes;
}

void FVoxelTemplateNode_NearlyEqual::PromotePin(FVoxelPin& InPin, const FVoxelPinType& NewType)
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

	const int32 NewDimension = GetDimension(NewType);
	for (FVoxelPin& Pin : GetPins())
	{
		if (Pin.Name != APin &&
			Pin.Name != BPin)
		{
			Pin.SetType(bIsBufferType ? Pin.GetType().GetBufferType() : Pin.GetType().GetInnerType());
			continue;
		}

		const int32 PinDimension = GetDimension(Pin.GetType());
		if (Pin.GetType().IsWildcard() ||
			InPin.Name == Pin.Name)
		{
			Pin.SetType(NewType);
		}
		else if (
			PinDimension == NewDimension ||
			PinDimension == 1 ||
			NewDimension == 1)
		{
			Pin.SetType(bIsBufferType ? Pin.GetType().GetBufferType() : Pin.GetType().GetInnerType());
		}
		else
		{
			Pin.SetType(NewType);
		}
	}
}