#include "Nodes/Templates/VoxelTemplateNode.h"
#include "VoxelMetaGraphGraph.h"
#include "Nodes/VoxelMathNodes.h"
#include "Nodes/VoxelPassthroughNodes.h"

TArray<FVoxelPinType> FVoxelTemplateNodeUtilities::GetFloatTypes()
{
	static TArray<FVoxelPinType> Result;
	if (Result.Num() == 0)
	{
		Result.Add(FVoxelPinType::Make<float>());
		Result.Add(FVoxelPinType::Make<FVector2D>());
		Result.Add(FVoxelPinType::Make<FVector>());
		Result.Add(FVoxelPinType::Make<FLinearColor>());

		Result.Add(FVoxelPinType::Make<FVoxelFloatBuffer>());
		Result.Add(FVoxelPinType::Make<FVoxelVector2DBuffer>());
		Result.Add(FVoxelPinType::Make<FVoxelVectorBuffer>());
		Result.Add(FVoxelPinType::Make<FVoxelLinearColorBuffer>());
	}

	return Result;
}

TArray<FVoxelPinType> FVoxelTemplateNodeUtilities::GetIntTypes()
{
	static TArray<FVoxelPinType> Result;
	if (Result.Num() == 0)
	{
		Result.Add(FVoxelPinType::Make<int32>());
		Result.Add(FVoxelPinType::Make<FIntPoint>());
		Result.Add(FVoxelPinType::Make<FIntVector>());

		Result.Add(FVoxelPinType::Make<FVoxelInt32Buffer>());
		Result.Add(FVoxelPinType::Make<FVoxelIntPointBuffer>());
		Result.Add(FVoxelPinType::Make<FVoxelIntVectorBuffer>());
	}

	return Result;
}

bool FVoxelTemplateNodeUtilities::IsFloat(const FVoxelPinType& PinType)
{
	if (PinType.IsDerivedFrom<FVoxelBuffer>())
	{
		return IsFloat(PinType.GetInnerType());
	}

	return
		PinType.Is<float>() ||
		PinType.Is<FVector2D>() ||
		PinType.Is<FVector>() ||
		PinType.Is<FLinearColor>();
}

bool FVoxelTemplateNodeUtilities::IsInt(const FVoxelPinType& PinType)
{
	if (PinType.IsDerivedFrom<FVoxelBuffer>())
	{
		return IsInt(PinType.GetInnerType());
	}

	return
		PinType.Is<int32>() ||
		PinType.Is<FIntPoint>() ||
		PinType.Is<FIntVector>();
}

bool FVoxelTemplateNodeUtilities::IsBool(const FVoxelPinType& PinType)
{
	if (PinType.IsDerivedFrom<FVoxelBuffer>())
	{
		return IsBool(PinType.GetInnerType());
	}

	return PinType.Is<bool>();
}

int32 FVoxelTemplateNodeUtilities::GetDimension(const FVoxelPinType& PinType)
{
	if (PinType.IsDerivedFrom<FVoxelBuffer>())
	{
		return GetDimension(PinType.GetInnerType());
	}

	if (PinType.Is<float>() ||
		PinType.Is<int32>() ||
		PinType.Is<bool>())
	{
		return 1;
	}

	if (PinType.Is<FVector2D>() ||
		PinType.Is<FIntPoint>())
	{
		return 2;
	}

	if (PinType.Is<FVector>() ||
		PinType.Is<FIntVector>())
	{
		return 3;
	}

	if (PinType.Is<FLinearColor>())
	{
		return 4;
	}

	ensure(PinType.IsWildcard());
	return 0;
}

FVoxelPinType FVoxelTemplateNodeUtilities::GetScalarType(const FVoxelPinType& PinType)
{
	if (PinType.IsDerivedFrom<FVoxelBuffer>())
	{
		const FVoxelPinType Type = GetScalarType(PinType.GetInnerType());
		return Type.GetBufferType();
	}

	if (PinType.Is<float>() ||
		PinType.Is<int32>() ||
		PinType.Is<bool>())
	{
		return PinType;
	}

	if (PinType.Is<FVector2D>() ||
		PinType.Is<FVector>() ||
		PinType.Is<FLinearColor>())
	{
		return FVoxelPinType::Make<float>();
	}

	if (PinType.Is<FIntPoint>() ||
		PinType.Is<FIntVector>())
	{
		return FVoxelPinType::Make<int32>();
	}

	ensure(false);
	return {};
}

FVoxelPinType FVoxelTemplateNodeUtilities::GetFloatType(const FVoxelPinType& Type)
{
	if (Type.IsDerivedFrom<FVoxelBuffer>())
	{
		return GetFloatType(Type.GetInnerType()).GetBufferType();
	}

	if (Type.Is<int32>())
	{
		return FVoxelPinType::Make<float>();
	}
	else if (Type.Is<FIntPoint>())
	{
		return FVoxelPinType::Make<FVector2D>();
	}
	else if (Type.Is<FIntVector>())
	{
		return FVoxelPinType::Make<FVector>();
	}

	return Type;
}

FVoxelPinType FVoxelTemplateNodeUtilities::GetIntType(const FVoxelPinType& Type)
{
	if (Type.IsDerivedFrom<FVoxelBuffer>())
	{
		return GetIntType(Type.GetInnerType()).GetBufferType();
	}

	if (Type.Is<float>())
	{
		return FVoxelPinType::Make<int32>();
	}
	else if (Type.Is<FVector2D>())
	{
		return FVoxelPinType::Make<FIntPoint>();
	}
	else if (Type.Is<FVector>())
	{
		return FVoxelPinType::Make<FIntVector>();
	}

	return Type;
}

TVoxelInstancedStruct<FVoxelNode> FVoxelTemplateNodeUtilities::GetBreakNode(const FVoxelPinType& PinType)
{
	if (PinType.IsDerivedFrom<FVoxelBuffer>())
	{
		TVoxelInstancedStruct<FVoxelNode> Node = GetBreakNode(PinType.GetInnerType());
		Node->PromotePin(Node->GetUniqueInputPin(), PinType);
		return Node;
	}

	if (PinType.Is<FVector2D>())
	{
		return FVoxelNode_BreakVector2D::StaticStruct();
	}
	else if (PinType.Is<FVector>())
	{
		return FVoxelNode_BreakVector::StaticStruct();
	}
	else if (PinType.Is<FLinearColor>())
	{
		return FVoxelNode_BreakLinearColor::StaticStruct();
	}
	else if (PinType.Is<FIntPoint>())
	{
		return FVoxelNode_BreakIntPoint::StaticStruct();
	}
	else if (PinType.Is<FIntVector>())
	{
		return FVoxelNode_BreakIntVector::StaticStruct();
	}
	else
	{
		ensure(false);
		return nullptr;
	}
}

TVoxelInstancedStruct<FVoxelNode> FVoxelTemplateNodeUtilities::GetMakeNode(const FVoxelPinType& PinType)
{
	if (PinType.IsDerivedFrom<FVoxelBuffer>())
	{
		TVoxelInstancedStruct<FVoxelNode> Node = GetMakeNode(PinType.GetInnerType());
		Node->PromotePin(Node->GetUniqueOutputPin(), PinType);
		return Node;
	}

	if (PinType.Is<FVector2D>())
	{
		return FVoxelNode_MakeVector2D::StaticStruct();
	}
	else if (PinType.Is<FVector>())
	{
		return FVoxelNode_MakeVector::StaticStruct();
	}
	else if (PinType.Is<FLinearColor>())
	{
		return FVoxelNode_MakeLinearColor::StaticStruct();
	}
	else if (PinType.Is<FIntPoint>())
	{
		return FVoxelNode_MakeIntPoint::StaticStruct();
	}
	else if (PinType.Is<FIntVector>())
	{
		return FVoxelNode_MakeIntVector::StaticStruct();
	}
	else
	{
		check(false);
		return nullptr;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UScriptStruct* FVoxelTemplateNodeUtilities::GetConvertToFloatNode(const FPin* Pin, FVoxelPinType& ResultPinType)
{
	const int32 Dimension = GetDimension(Pin->Type);

	UScriptStruct* Struct;
	switch (Dimension)
	{
	default: check(false);
	case 1:
	{
		Struct = FVoxelNode_Conv_IntToFloat::StaticStruct();
		ResultPinType = FVoxelPinType::Make<float>();
		break;
	}
	case 2:
	{
		Struct = FVoxelNode_Conv_IntPointToVector2D::StaticStruct();
		ResultPinType = FVoxelPinType::Make<FVector2D>();
		break;
	}
	case 3:
	{
		Struct = FVoxelNode_Conv_IntVectorToVector::StaticStruct();
		ResultPinType = FVoxelPinType::Make<FVector>();
		break;
	}
	}

	if (Pin->Type.IsDerivedFrom<FVoxelBuffer>())
	{
		ResultPinType = ResultPinType.GetBufferType();
	}

	return Struct;
}

TVoxelInstancedStruct<FVoxelNode> FVoxelTemplateNodeUtilities::GetMakeNode(const FPin* Pin, const int32 Dimension)
{
	check(Dimension > 1);

	const bool bIsFloat = IsPinFloat(Pin);

	switch (Dimension)
	{
	default: check(false);
	case 2:
	{
		if (bIsFloat)
		{
			return FVoxelNode_MakeVector2D::StaticStruct();
		}
		else
		{
			return FVoxelNode_MakeIntPoint::StaticStruct();
		}
	}
	case 3:
	{
		if (bIsFloat)
		{
			return FVoxelNode_MakeVector::StaticStruct();
		}
		else
		{
			return FVoxelNode_MakeIntVector::StaticStruct();
		}
	}
	case 4:
	{
		if (bIsFloat)
		{
			return FVoxelNode_MakeLinearColor::StaticStruct();
		}
		else
		{
			check(false);
		}
	}
	}

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int32 FVoxelTemplateNodeUtilities::GetMaxDimension(const TArray<FPin*>& Pins)
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	int32 HighestDimension = 0;
	for (const FPin* Pin : Pins)
	{
		HighestDimension = FMath::Max(HighestDimension, GetDimension(Pin->Type));
	}

	return HighestDimension;
}

FVoxelTemplateNodeUtilities::FPin* FVoxelTemplateNodeUtilities::GetLinkedPin(FPin* Pin)
{
	check(Pin->GetLinkedTo().Num() == 1);
	return &Pin->GetLinkedTo()[0];
}

FVoxelTemplateNodeUtilities::FPin* FVoxelTemplateNodeUtilities::ConvertToFloat(FPin* Pin)
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	if (IsPinFloat(Pin))
	{
		return Pin;
	}

	if (!IsPinInt(Pin))
	{
		ensure(false);
		return Pin;
	}

	FVoxelPinType ResultType; 
	const TVoxelInstancedStruct<FVoxelNode> Struct = GetConvertToFloatNode(Pin, ResultType);
	
	FNode& ConvNode = Pin->Node.Graph.NewNode(ENodeType::Struct, Pin->Node.Source);
	ConvNode.Struct() = Struct;

	FPin* ResultPin = nullptr;
	for (const FVoxelPin& ConvNodePin : ConvNode.Struct()->GetPins())
	{
		if (ConvNodePin.bIsInput)
		{
			FPin& MakeNodeInputPin = ConvNode.NewInputPin(ConvNodePin.Name, Pin->Type);
			Pin->MakeLinkTo(MakeNodeInputPin);
		}
		else
		{
			FPin& MakeNodeOutputPin = ConvNode.NewOutputPin(ConvNodePin.Name, ResultType);
			ResultPin = &MakeNodeOutputPin;
		}
	}

	return ResultPin;
}

FVoxelTemplateNodeUtilities::FPin* FVoxelTemplateNodeUtilities::ScalarToVector(FPin* Pin, int32 HighestDimension)
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	if (HighestDimension == 1 ||
		GetDimension(Pin->Type) != 1)
	{
		return Pin;
	}

	FNode& MakeNode = CreateNode(Pin, GetMakeNode(Pin, HighestDimension));
	ensure(MakeNode.GetInputPins().Num() == HighestDimension);

	for (FPin& MakeNodePin : MakeNode.GetInputPins())
	{
		Pin->MakeLinkTo(MakeNodePin);
	}

	return &MakeNode.GetOutputPin(0);
}

FVoxelTemplateNodeUtilities::FPin* FVoxelTemplateNodeUtilities::ZeroExpandVector(FPin* Pin, const int32 HighestDimension)
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	const int32 PinDimension = GetDimension(Pin->Type);

	if (PinDimension == HighestDimension)
	{
		return Pin;
	}

	TArray<FPin*> ScalarPins;
	{
		FNode& BreakNode = CreateNode(Pin, GetBreakNode(Pin->Type));
		ensure(BreakNode.GetOutputPins().Num() == PinDimension);

		Pin->MakeLinkTo(BreakNode.GetInputPin(0));

		for (FPin& BreakNodePin : BreakNode.GetOutputPins())
		{
			ScalarPins.Add(&BreakNodePin);
		}
	}

	FNode& Passthrough = Pin->Node.Graph.NewNode(ENodeType::Passthrough, Pin->Node.Source);
	Passthrough.NewInputPin("ZeroScalarInput", ScalarPins[0]->Type, FVoxelPinValue(ScalarPins[0]->Type));
	FPin* ZeroScalarPin = &Passthrough.NewOutputPin("ZeroScalarOutput", ScalarPins[0]->Type);

	FNode& MakeNode = CreateNode(Pin, GetMakeNode(Pin, HighestDimension));
	ensure(MakeNode.GetInputPins().Num() == HighestDimension);

	for (int32 Index = 0; Index < MakeNode.GetInputPins().Num(); Index++)
	{
		if (ScalarPins.IsValidIndex(Index))
		{
			ScalarPins[Index]->MakeLinkTo(MakeNode.GetInputPin(Index));
		}
		else
		{
			ZeroScalarPin->MakeLinkTo(MakeNode.GetInputPin(Index));
		}
	}

	return &MakeNode.GetOutputPin(0);
}

TArray<FVoxelTemplateNodeUtilities::FPin*> FVoxelTemplateNodeUtilities::BreakVector(FPin* Pin)
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	const int32 PinDimension = GetDimension(Pin->Type);
	if (PinDimension == 1)
	{
		return { Pin };
	}
	ensure(PinDimension > 1);

	TArray<FPin*> ResultPins;

	FNode& BreakNode = CreateNode(Pin, GetBreakNode(Pin->Type));
	ensure(BreakNode.GetPins().Num() == PinDimension + 1);

	Pin->MakeLinkTo(BreakNode.GetInputPin(0));

	for (FPin& BreakNodePin : BreakNode.GetOutputPins())
	{
		ResultPins.Add(&BreakNodePin);
	}

	return ResultPins;
}

FVoxelTemplateNodeUtilities::FPin* FVoxelTemplateNodeUtilities::MakeVector(TArray<FPin*> Pins)
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	if (Pins.Num() == 1)
	{
		return Pins[0];
	}
	ensure(Pins.Num() > 1);

	FNode& MakeNode = CreateNode(Pins[0], GetMakeNode(Pins[0], Pins.Num()));
	ensure(MakeNode.GetInputPins().Num() == Pins.Num());

	for (int32 Index = 0; Index < Pins.Num(); Index++)
	{
		Pins[Index]->MakeLinkTo(MakeNode.GetInputPin(Index));
	}

	return &MakeNode.GetOutputPin(0);
}

bool FVoxelTemplateNodeUtilities::IsPinFloat(const FPin* Pin)
{
	return IsFloat(Pin->Type);
}

bool FVoxelTemplateNodeUtilities::IsPinInt(const FPin* Pin)
{
	return IsInt(Pin->Type);
}

bool FVoxelTemplateNodeUtilities::IsPinBool(const FPin* Pin)
{
	return IsBool(Pin->Type);
}

bool FVoxelTemplateNodeUtilities::IsPinOfName(const FPin* Pin, const TSet<FName> Names)
{
	return Names.Contains(Pin->Name);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelTemplateNodeUtilities::FNode& FVoxelTemplateNodeUtilities::CreateNode(const FPin* Pin, TVoxelInstancedStruct<FVoxelNode> NodeStruct)
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	const FNode& Node = Pin->Node;

	FNode& StructNode = Node.Graph.NewNode(ENodeType::Struct, Node.Source);
	StructNode.Struct() = NodeStruct;

	for (FVoxelPin& StructPin : StructNode.Struct()->GetPins())
	{
		if (StructPin.IsPromotable() ||
			EnumHasAllFlags(StructPin.Flags, EVoxelPinFlags::MathPin))
		{
			if (Pin->Type.IsBuffer())
			{
				StructPin.SetType(StructPin.GetType().GetBufferType());
			}
			else
			{
				StructPin.SetType(StructPin.GetType().GetInnerType());
			}
		}

		if (StructPin.bIsInput)
		{
			StructNode.NewInputPin(StructPin.Name, StructPin.GetType());
		}
		else
		{
			StructNode.NewOutputPin(StructPin.Name, StructPin.GetType());
		}
	}

	return StructNode;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelTemplateNodeUtilities::Any(const TArray<FPin*>& Pins, const TFunctionRef<bool(FPin*)> Lambda)
{
	for (FPin* Pin : Pins)
	{
		if (Lambda(Pin))
		{
			return true;
		}
	}

	return false;
}

bool FVoxelTemplateNodeUtilities::All(const TArray<FPin*>& Pins, const TFunctionRef<bool(FPin*)> Lambda)
{
	for (FPin* Pin : Pins)
	{
		if (!Lambda(Pin))
		{
			return false;
		}
	}

	return true;
}

TArray<FVoxelTemplateNodeUtilities::FPin*> FVoxelTemplateNodeUtilities::Filter(const TArray<FPin*>& Pins, const TFunctionRef<bool(const FPin*)> Lambda)
{
	TArray<FPin*> Result;
	for (FPin* Pin : Pins)
	{
		if (Lambda(Pin))
		{
			Result.Add(Pin);
		}
	}

	return Result;
}

FVoxelTemplateNodeUtilities::FPin* FVoxelTemplateNodeUtilities::Reduce(TArray<FPin*> Pins, TFunctionRef<FPin*(FPin*, FPin*)> Lambda)
{
	check(Pins.Num() > 0);

	while (Pins.Num() > 1)
	{
		FPin* PinB = Pins.Pop(false);
		FPin* PinA = Pins.Pop(false);
		Pins.Add(Lambda(PinA, PinB));
	}

	return Pins[0];
}

FVoxelTemplateNodeUtilities::FPin* FVoxelTemplateNodeUtilities::Call_Single(UScriptStruct* NodeStruct, const TArray<FPin*>& Pins)
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	if (!ensure(Pins.Num() > 0))
	{
		return nullptr;
	}

	TArray<TArray<FPin*>> MultiPins;
	for (auto Pin : Pins)
	{
		MultiPins.Add({ Pin });
	}

	TArray<FPin*> OutputPins = Call_Multi(NodeStruct, MultiPins);

	if (!ensure(OutputPins.Num() == 1))
	{
		return nullptr;
	}

	return OutputPins[0];
}

TArray<FVoxelTemplateNodeUtilities::FPin*> FVoxelTemplateNodeUtilities::Call_Multi(UScriptStruct* NodeStruct, const TArray<TArray<FPin*>>& Pins)
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	if (!ensure(Pins.Num() > 0))
	{
		return {};
	}

	const int32 Dimension = Pins[0].Num();
	if (!ensure(Dimension > 0))
	{
		return {};
	}

	TArray<FPin*> ResultPins;

	for (int32 DimensionIndex = 0; DimensionIndex < Dimension; DimensionIndex++)
	{
		FNode& SourceNode = Pins[0][DimensionIndex]->Node;

		FNode& StructNode = SourceNode.Graph.NewNode(ENodeType::Struct, SourceNode.Source);
		StructNode.Struct() = NodeStruct;

		int32 PinIndex = 0;
		for (auto& Pin : StructNode.Struct()->GetPins())
		{
			if (Pin.bIsInput)
			{
				if (!ensure(Pins.IsValidIndex(PinIndex)))
				{
					continue;
				}

				if (Pin.GetType().IsWildcard() ||
					EnumHasAllFlags(Pin.Flags, EVoxelPinFlags::MathPin))
				{
					StructNode.Struct()->PromotePin(Pin, Pins[PinIndex][DimensionIndex]->Type);
				}

				FPin& TargetPin = StructNode.NewInputPin(Pin.Name, Pin.GetType());
				Pins[PinIndex++][DimensionIndex]->MakeLinkTo(TargetPin);
			}
			else
			{
				ResultPins.Add(&StructNode.NewOutputPin(Pin.Name, Pin.GetType()));
			}
		}
	}

	return ResultPins;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelTemplateNode::ExpandNode(FGraph& Graph, FNode& Node) const
{
	TArray<FPin*> InputPins = Node.GetInputPins().Array();
	InputPins = Apply(InputPins, &GetLinkedPin);

	TArray<FPin*> Pins = InputPins;
	Pins.Append(Node.GetOutputPins().Array());

	TArray<FPin*> OutputPins;
	ExpandPins(Node, InputPins, Pins, OutputPins);

	check(Node.GetOutputPins().Num() == OutputPins.Num());

	for (int32 Index = 0; Index < OutputPins.Num(); Index++)
	{
		const FPin& SourceOutputPin = Node.GetOutputPin(Index);
		FPin& TargetOutputPin = *OutputPins[Index];

		if (!ensure(SourceOutputPin.Type == TargetOutputPin.Type))
		{
			VOXEL_MESSAGE(Error, "{0}: internal error when expanding template node", Node);
			return;
		}

		SourceOutputPin.CopyOutputPinTo(TargetOutputPin);
	}
}