// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphGraph.h"
#include "VoxelExecNode.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

void FPin::Check(const FGraph& Graph) const
{
	ensure(!Name.IsNone());
	ensure(Type.IsValid());
	ensure(Graph.Nodes.Contains(&Node));
	ensure(
		Direction == EPinDirection::Output ||
		(!DefaultValue.IsValid() && GetLinkedTo().Num() > 0) ||
		(!DefaultValue.IsValid() && Type.IsWildcard()) ||
		DefaultValue.GetType().IsDerivedFrom(Type));

	if (Direction == EPinDirection::Input)
	{
		ensure(Node.FindInput(Name) == this);
	}
	else
	{
		check(Direction == EPinDirection::Output);
		ensure(Node.FindOutput(Name) == this);
	}

	ensure(TSet<FPin*>(LinkedTo).Num() == LinkedTo.Num());
	ensure(Direction == EPinDirection::Output || LinkedTo.Num() <= 1);

	ensure(!LinkedTo.Contains(this));
	for (const FPin* Pin : LinkedTo)
	{
		ensure(Direction == EPinDirection::Input || Type.IsDerivedFrom(Pin->Type));
		ensure(Direction == EPinDirection::Output || Pin->Type.IsDerivedFrom(Type));
		ensure(Direction != Pin->Direction);
		ensure(Graph.Nodes.Contains(&Pin->Node));
		ensure(Pin->LinkedTo.Contains(this));
		
		if (Pin->Direction == EPinDirection::Input)
		{
			ensure(Pin->Node.FindInput(Pin->Name) == Pin);
		}
		else
		{
			check(Pin->Direction == EPinDirection::Output);
			ensure(Pin->Node.FindOutput(Pin->Name) == Pin);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FPin& FNode::NewInputPin(FName Name, const FVoxelPinType& PinType, const FVoxelPinValue& DefaultValue)
{
	FPin& Pin = Graph.NewPin(Name, PinType, DefaultValue, EPinDirection::Input, *this);

	Pins.Add(&Pin);
	InputPins.Add(&Pin);

	check(!InputPinsMap.Contains(Name));
	InputPinsMap.Add(Name, &Pin);

	return Pin;
}

FPin& FNode::NewOutputPin(FName Name, const FVoxelPinType& PinType)
{
	FPin& Pin = Graph.NewPin(Name, PinType, {}, EPinDirection::Output, *this);

	Pins.Add(&Pin);
	OutputPins.Add(&Pin);

	check(!OutputPinsMap.Contains(Name));
	OutputPinsMap.Add(Name, &Pin);

	return Pin;
}

void FNode::Check()
{
	if (Type == ENodeType::Struct)
	{
		ensure(Struct().IsValid());
	}
	else if (Type == ENodeType::Macro)
	{
		ensure(Macro().Graph);
	}
	else if (Type == ENodeType::Parameter)
	{
		ensure(Parameter().Guid.IsValid());
		ensure(Parameter().Type.IsValid());
	}
	else if (Type == ENodeType::MacroInput)
	{
		ensure(MacroInput().Guid.IsValid());
		ensure(MacroInput().Type.IsValid());
	}
	else if (Type == ENodeType::MacroOutput)
	{
		ensure(MacroOutput().Guid.IsValid());
		ensure(MacroOutput().Type.IsValid());
	}
	else
	{
		check(Type == ENodeType::Passthrough);
	}

	for (const FPin* Pin : Pins)
	{
		Pin->Check(Graph);
	}
}

void FNode::BreakAllLinks()
{
	for (FPin* Pin : Pins)
	{
		Pin->BreakAllLinks();
	}
}

void FNode::RemovePin(FPin* Pin)
{
	ensure(Pin->GetLinkedTo().Num() == 0);
	ensure(Pins.Remove(Pin) == 1);

	if (Pin->Direction == EPinDirection::Input)
	{
		ensure(InputPins.Remove(Pin));
		ensure(InputPinsMap.Remove(Pin->Name));
	}
	else
	{
		check(Pin->Direction == EPinDirection::Output);

		ensure(OutputPins.Remove(Pin));
		ensure(OutputPinsMap.Remove(Pin->Name));
	}
}

const FPin* FNode::FindPropertyPin(const FProperty& Property) const
{
	if (IsFunctionInput(Property))
	{
		return FindInput(Property.GetFName());
	}
	else
	{
		return FindOutput(Property.GetFName());
	}
}

const FPin& FNode::FindPropertyPinChecked(const FProperty& Property) const
{
	if (IsFunctionInput(Property))
	{
		return FindInputChecked(Property.GetFName());
	}
	else
	{
		return FindOutputChecked(Property.GetFName());
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FNode& FGraph::NewNode(ENodeType Type, const FSourceNode& Source)
{
	FNode& Node = *NodeRefs.Add_GetRef(TUniquePtr<FNode>(new FNode(Type, Source, *this)));
	Nodes.Add(&Node);
	return Node;
}

void FGraph::Check()
{
	for (FNode* Node : Nodes)
	{
		Node->Check();
	}
}

void FGraph::CloneTo(
	FGraph& TargetGraph, 
	TMap<const FNode*, FNode*>* OldToNewNodesPtr, 
	TMap<const FPin*, FPin*>* OldToNewPinsPtr) const
{
	VOXEL_FUNCTION_COUNTER();

	TMap<const FNode*, FNode*> OldToNewNodesStorage;
	TMap<const FPin*, FPin*> OldToNewPinsStorage;

	TMap<const FNode*, FNode*>& OldToNewNodes = OldToNewNodesPtr ? *OldToNewNodesPtr : OldToNewNodesStorage;
	TMap<const FPin*, FPin*>& OldToNewPins = OldToNewPinsPtr ? *OldToNewPinsPtr : OldToNewPinsStorage;

	for (const FNode& OldNode : GetNodes())
	{
		FNode& NewNode = TargetGraph.NewNode(OldNode.Type, OldNode.Source);
		OldToNewNodes.Add(&OldNode, &NewNode);

		NewNode.CopyFrom(OldNode);

		for (const FPin& OldPin : OldNode.GetInputPins())
		{
			FPin& NewPin = NewNode.NewInputPin(OldPin.Name, OldPin.Type, OldPin.GetDefaultValue());
			OldToNewPins.Add(&OldPin, &NewPin);
		}
		for (const FPin& OldPin : OldNode.GetOutputPins())
		{
			FPin& NewPin = NewNode.NewOutputPin(OldPin.Name, OldPin.Type);
			OldToNewPins.Add(&OldPin, &NewPin);
		}
	}

	for (const FNode& OldNode : GetNodes())
	{
		for (const FPin& OldPin : OldNode.GetPins())
		{
			FPin& NewPin = *OldToNewPins[&OldPin];
			for (FPin& OtherPin : OldPin.GetLinkedTo())
			{
				NewPin.TryMakeLinkTo(*OldToNewPins[&OtherPin]);
			}
		}
	}

	TargetGraph.Check();
}

TSharedRef<FGraph> FGraph::Clone(
	TMap<const FNode*, FNode*>* OldToNewNodesPtr,
	TMap<const FPin*, FPin*>* OldToNewPinsPtr) const
{
	const TSharedRef<FGraph> Graph = MakeShared<FGraph>();
	CloneTo(*Graph, OldToNewNodesPtr, OldToNewPinsPtr);
	return Graph;
}

FPin& FGraph::NewPin(
	FName Name,
	const FVoxelPinType& Type,
	const FVoxelPinValue& DefaultValue,
	EPinDirection Direction,
	FNode& Node)
{
	return *PinRefs.Add_GetRef(TUniquePtr<FPin>(new FPin(Name, Type, DefaultValue, Direction, Node)));
}

END_VOXEL_NAMESPACE(MetaGraph)