// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphEditorCompiler.h"
#include "VoxelGraphMessages.h"
#include "VoxelPinValue.h"
#include "EdGraphSchema_K2.h"
#include "Nodes/VoxelMetaGraphKnotNode.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

void FEditorPin::Check(const FEditorGraph& Graph) const
{
	ensure(!PinName.IsNone());
	ensure(Graph.Nodes.Contains(Node));
	ensure(Node->InputPins.Contains(this) || Node->OutputPins.Contains(this));

	ensure(!LinkedTo.Contains(this));
	for (const FEditorPin* Pin : LinkedTo)
	{
		ensure(Graph.Nodes.Contains(Pin->Node));
		ensure(Pin->Node->InputPins.Contains(Pin) || Pin->Node->OutputPins.Contains(Pin));
		ensure(Pin->LinkedTo.Contains(this));
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FEditorCompilerUtilities::CompileGraph(
	const UEdGraph& EdGraph,
	FEditorGraph& OutGraph,
	const TArray<FFEditorCompilerPass>& Passes)
{
	VOXEL_FUNCTION_COUNTER();

	for (const UEdGraphNode* Node : EdGraph.Nodes)
	{
		for (const UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin->bOrphanedPin)
			{
				return false;
			}
		}
	}

	TMap<UObject*, UObject*> CreatedObjects;

	FObjectDuplicationParameters DuplicationParameters(VOXEL_CONST_CAST(&EdGraph), EdGraph.GetOuter());
	DuplicationParameters.CreatedObjects = &CreatedObjects;
	DuplicationParameters.ApplyFlags |= RF_Transient;
	DuplicationParameters.FlagMask &= ~RF_Transactional;

	FEditorCompiler Compiler;
	Compiler.Graph = CastChecked<UEdGraph>(StaticDuplicateObjectEx(DuplicationParameters));
	ON_SCOPE_EXIT
	{
		Compiler.Graph->MarkAsGarbage();
	};
	
	for (auto& It : CreatedObjects)
	{
		UEdGraphNode* Source = Cast<UEdGraphNode>(It.Key);
		UEdGraphNode* Duplicate = Cast<UEdGraphNode>(It.Value);
		if (!Source || !Duplicate)
		{
			continue;
		}

		Compiler.NodeSourceMap.Add(Duplicate, Source);
	}

	CheckGraph(Compiler);

	if (GVoxelGraphMessages->HasError())
	{
		return false;
	}

	for (const FFEditorCompilerPass& Pass : Passes)
	{
		if (!Pass(Compiler))
		{
			return false;
		}

		CheckGraph(Compiler);

		if (GVoxelGraphMessages->HasError())
		{
			return false;
		}
	}

	return TranslateGraph(Compiler, OutGraph);
}

bool FEditorCompilerUtilities::TranslateGraph(
	const FEditorCompiler& Compiler,
	FEditorGraph& OutGraph)
{
	TMap<UEdGraphPin*, FEditorPin*> AllPins;
	TMap<UEdGraphNode*, FEditorNode*> AllNodes;

	const auto AllocPin = [&](UEdGraphPin* GraphPin)
	{
		FEditorPin*& Pin = AllPins.FindOrAdd(GraphPin);
		if (!Pin)
		{
			Pin = OutGraph.NewPin();
			Pin->Node = AllNodes[GraphPin->GetOwningNode()];
			Pin->PinName = GraphPin->PinName;
			Pin->PinType = GraphPin->PinType;

			if (!Pin->PinType.IsWildcard())
			{
				Pin->DefaultValue = FVoxelPinValue::MakeFromPinDefaultValue(*GraphPin);
			}
		}
		return Pin;
	};
	const auto AllocNode = [&](UVoxelGraphNode* GraphNode)
	{
		FEditorNode*& Node = AllNodes.FindOrAdd(GraphNode);
		if (!Node)
		{
			Node = OutGraph.NewNode();
			Node->GraphNode = GraphNode;
			Node->SourceGraphNode = Compiler.GetSourceNode(GraphNode);

			for (UEdGraphPin* Pin : GraphNode->GetInputPins())
			{
				Node->InputPins.Add(AllocPin(Pin));
			}
			for (UEdGraphPin* Pin : GraphNode->GetOutputPins())
			{
				Node->OutputPins.Add(AllocPin(Pin));
			}
		}
		return Node;
	};

	for (UEdGraphNode* GraphNode : Compiler.GetNodesCopy())
	{
		UVoxelMetaGraphNode* Node = Cast<UVoxelMetaGraphNode>(GraphNode);
		if (!Node ||
			Node->IsA<UVoxelMetaGraphKnotNode>())
		{
			continue;
		}

		OutGraph.Nodes.Add(AllocNode(Node));
	}

	for (const auto& It : AllPins)
	{
		UEdGraphPin* GraphPin = It.Key;
		FEditorPin* Pin = It.Value;

		ensure(!GraphPin->ParentPin);
		ensure(GraphPin->SubPins.Num() == 0);
		ensure(!GraphPin->bOrphanedPin);

		for (const UEdGraphPin* LinkedTo : GraphPin->LinkedTo)
		{
			if (UVoxelMetaGraphKnotNode* Knot = Cast<UVoxelMetaGraphKnotNode>(LinkedTo->GetOwningNode()))
			{
				for (const UEdGraphPin* LinkedToKnot : Knot->GetLinkedPins(GraphPin->Direction))
				{
					if (ensure(AllPins.Contains(LinkedToKnot)))
					{
						Pin->LinkedTo.Add(AllPins[LinkedToKnot]);
					}
				}
				continue;
			}

			if (ensure(AllPins.Contains(LinkedTo)))
			{
				Pin->LinkedTo.Add(AllPins[LinkedTo]);
			}
		}
	}

	for (const FEditorNode* Node : OutGraph.Nodes)
	{
		check(Node->GraphNode);

		for (const FEditorPin* Pin : Node->InputPins)
		{
			check(Pin->Node == Node);
			Pin->Check(OutGraph);
		}
		for (const FEditorPin* Pin : Node->OutputPins)
		{
			check(Pin->Node == Node);
			Pin->Check(OutGraph);
		}
	}

	return true;
}

void FEditorCompilerUtilities::CheckGraph(const FEditorCompiler& Compiler)
{
	for (const UEdGraphNode* Node : Compiler.GetNodesCopy())
	{
		if (!ensure(Node))
		{
			VOXEL_MESSAGE(Error, "{0} is invalid", Node);
			return;
		}

		ensure(Node->HasAllFlags(RF_Transient));
		ensure(Compiler.GetSourceNode(Node));

		for (const UEdGraphPin* Pin : Node->Pins)
		{
			if (!ensure(Pin))
			{
				VOXEL_MESSAGE(Error, "{0} is invalid", Pin);
				return;
			}

			for (const UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				if (!ensure(LinkedPin))
				{
					VOXEL_MESSAGE(Error, "{0} is invalid", LinkedPin);
					return;
				}

				if (!ensure(LinkedPin->LinkedTo.Contains(Pin)))
				{
					VOXEL_MESSAGE(Error, "Link from {0} to {1} is invalid", Pin, LinkedPin);
					return;
				}
			}
		}
	}
}

END_VOXEL_NAMESPACE(MetaGraph)