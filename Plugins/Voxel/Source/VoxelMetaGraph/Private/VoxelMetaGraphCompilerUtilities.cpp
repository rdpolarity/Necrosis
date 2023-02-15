// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphCompilerUtilities.h"
#include "VoxelExecNode.h"
#include "VoxelGraphMessages.h"
#include "VoxelExposedPinType.h"
#include "Nodes/VoxelExecCodeGenNode.h"
#include "Nodes/Templates/VoxelTemplateNode.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

VOXEL_CONSOLE_VARIABLE(
	VOXELMETAGRAPH_API, bool, GEnableDebugGraph, false,
	"voxel.metagraph.EnableDebugGraph",
	"");

TSharedPtr<FGraph> FCompilerUtilities::Compile(
	const UVoxelMetaGraph& MetaGraph,
	FVoxelMetaGraphVariableCollection Variables,
	FVoxelRuntime* Runtime)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(!MetaGraph.bIsMacroGraph))
	{
		// TODO figure out macro preview
		return nullptr;
	}

	const FVoxelGraphMessages Messages(MetaGraph.GetMainGraph());

	Variables.Fixup(MetaGraph.Parameters);

	CheckMetaGraph(MetaGraph);

	if (Messages.HasError())
	{
		return nullptr;
	}

	TSharedPtr<FGraph> Graph;

	{
		FSourceNode Source;
		Graph = TranslateCompiledGraph(
			MetaGraph.CompiledGraph,
			MetaGraph.Parameters,
			Runtime,
			Source);
	}

	if (Messages.HasError() ||
		!ensure(Graph))
	{
		return nullptr;
	}

#define RUN_PASS(Name, ...) \
	Name(*Graph, ##__VA_ARGS__); \
	if (Messages.HasError()) \
	{ \
		if (GEnableDebugGraph) \
		{ \
			MetaGraph.OnDebugGraph.Broadcast(Graph->Clone()); \
		} \
		return nullptr;  \
	} \
	Graph->Check();

	RUN_PASS(FlattenGraph, Runtime);

	RUN_PASS(RemoveUnusedNodes);

	RUN_PASS(CheckWildcards);
	RUN_PASS(ReplaceTemplates);
	RUN_PASS(CheckWildcards);
	
	if (GEnableDebugGraph)
	{
		MetaGraph.OnDebugGraph.Broadcast(Graph->Clone());
	}

	// Checks for loops
	SortNodes(ReinterpretCastArray<const FNode*>(Graph->GetNodesArray()));
	if (Messages.HasError())
	{
		return nullptr;
	}

	RUN_PASS(RemovePassthroughs);
	RUN_PASS(RemoveUnusedNodes);

	RUN_PASS(CheckOutputs);
	RUN_PASS(ReplaceParameters, Variables, Runtime);
	RUN_PASS(ReplaceCodeGenNodes);

#undef RUN_PASS

	return Graph->Clone();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FCompilerUtilities::CheckMetaGraph(const UVoxelMetaGraph& MetaGraph)
{
	const TFunction<void(const FVoxelMetaGraphCompiledGraph& CompiledGraph, TSet<const UVoxelMetaGraph*>& UsedGraphs)> FindUsedGraphs =
		[&](const FVoxelMetaGraphCompiledGraph& CompiledGraph, TSet<const UVoxelMetaGraph*>& UsedGraphs)
		{
			for (const FVoxelMetaGraphCompiledNode& Node : CompiledGraph.Nodes)
			{
				if (Node.Type != EVoxelMetaGraphCompiledNodeType::Macro ||
					!Node.MetaGraph ||
					UsedGraphs.Contains(Node.MetaGraph))
				{
					continue;
				}

				UsedGraphs.Add(Node.MetaGraph);
				FindUsedGraphs(Node.MetaGraph->CompiledGraph, UsedGraphs);
			}
		};

	TSet<const UVoxelMetaGraph*> GraphsToCheck;
	FindUsedGraphs(MetaGraph.CompiledGraph, GraphsToCheck);
	GraphsToCheck.Add(&MetaGraph);

	for (const UVoxelMetaGraph* GraphToCheck : GraphsToCheck)
	{
		TSet<const UVoxelMetaGraph*> UsedNodes;
		FindUsedGraphs(GraphToCheck->CompiledGraph, UsedNodes);

		if (UsedNodes.Contains(GraphToCheck))
		{
			VOXEL_MESSAGE(Error, "Recursive graph: {0}", GraphToCheck);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<FGraph> FCompilerUtilities::TranslateCompiledGraph(
	const FVoxelMetaGraphCompiledGraph& CompiledGraph,
	const TArray<FVoxelMetaGraphParameter>& Parameters,
	FVoxelRuntime* Runtime,
	const FSourceNode& MetaGraphNodeSource)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	const TSharedRef<FGraph> Graph = MakeShared<FGraph>();

	TMap<const FVoxelMetaGraphCompiledNode*, FNode*> NodesMap;
	for (const FVoxelMetaGraphCompiledNode& CompiledNode : CompiledGraph.Nodes)
	{
		FSourceNode Source = MetaGraphNodeSource;
		Source.GraphNodes.Add(CompiledNode.SourceGraphNode);

		FNode* Node = nullptr;
		switch (CompiledNode.Type)
		{
		default:
		{
			VOXEL_MESSAGE(Error, "Invalid node {0}", CompiledNode);
			return nullptr;
		}
		case EVoxelMetaGraphCompiledNodeType::Struct:
		{
			if (!CompiledNode.Struct.IsValid())
			{
				VOXEL_MESSAGE(Error, "Invalid struct on {0}", CompiledNode);
				return nullptr;
			}

			const FVoxelNode& VoxelNode = CompiledNode.Struct.Get();
			for (const FVoxelPin& Pin : VoxelNode.GetPins())
			{
				const TArray<FVoxelMetaGraphCompiledPin>& CompiledPins = Pin.bIsInput ? CompiledNode.InputPins : CompiledNode.OutputPins;
				if (!CompiledPins.FindByPredicate([&](const FVoxelMetaGraphCompiledPin& CompiledPin)
					{
						return CompiledPin.PinName == Pin.Name;
					}))
				{
					VOXEL_MESSAGE(Error, "Outdated node {0}", CompiledNode);
					return nullptr;
				}
			}
			for (const FVoxelMetaGraphCompiledPin& CompiledPin : CompiledNode.InputPins)
			{
				const TSharedPtr<const FVoxelPin> Pin = VoxelNode.FindPin(CompiledPin.PinName);
				if (!Pin ||
					!Pin->bIsInput)
				{
					VOXEL_MESSAGE(Error, "Outdated node {0}", CompiledNode);
					return nullptr;
				}

				if (CompiledPin.Type != Pin->GetType())
				{
					VOXEL_MESSAGE(Error, "Outdated node {0}", CompiledNode);
					return nullptr;
				}
			}
			for (const FVoxelMetaGraphCompiledPin& CompiledPin : CompiledNode.OutputPins)
			{
				const TSharedPtr<const FVoxelPin> Pin = VoxelNode.FindPin(CompiledPin.PinName);
				if (!Pin ||
					Pin->bIsInput)
				{
					VOXEL_MESSAGE(Error, "Outdated node {0}", CompiledNode);
					return nullptr;
				}

				if (CompiledPin.Type != Pin->GetType())
				{
					VOXEL_MESSAGE(Error, "Outdated node {0}", CompiledNode);
					return nullptr;
				}
			}

			Node = &Graph->NewNode(ENodeType::Struct, Source);
			Node->Struct() = CompiledNode.Struct;
		}
		break;
		case EVoxelMetaGraphCompiledNodeType::Macro:
		{
			if (!CompiledNode.MetaGraph)
			{
				VOXEL_MESSAGE(Error, "Invalid graph {0}", CompiledNode);
				return nullptr;
			}

			Node = &Graph->NewNode(ENodeType::Macro, Source);
			Node->Macro().Graph = CompiledNode.MetaGraph;
		}
		break;
		case EVoxelMetaGraphCompiledNodeType::Parameter:
		{
			const FVoxelMetaGraphParameter* Parameter = Parameters.FindByKey(CompiledNode.Guid);
			if (!Parameter ||
				Parameter->ParameterType != EVoxelMetaGraphParameterType::Parameter ||
				!Parameter->Type.IsValid() ||
				!ensure(CompiledNode.InputPins.Num() == 0) ||
				!ensure(CompiledNode.OutputPins.Num() == 1) ||
				CompiledNode.OutputPins[0].Type != Parameter->Type)
			{
				VOXEL_MESSAGE(Error, "Invalid parameter {0}", CompiledNode);
				return nullptr;
			}

			Node = &Graph->NewNode(ENodeType::Parameter, Source);
			Node->Parameter().Guid = Parameter->Guid;
			Node->Parameter().Name = Parameter->Name;
			Node->Parameter().Type = CompiledNode.OutputPins[0].Type;
		}
		break;
		case EVoxelMetaGraphCompiledNodeType::MacroInput:
		{
			const FVoxelMetaGraphParameter* MacroInput = Parameters.FindByKey(CompiledNode.Guid);
			if (!MacroInput ||
				MacroInput->ParameterType != EVoxelMetaGraphParameterType::MacroInput ||
				!MacroInput->Type.IsValid() ||
				!ensure(CompiledNode.InputPins.Num() == 1) ||
				!ensure(CompiledNode.OutputPins.Num() == 1) ||
				CompiledNode.OutputPins[0].Type != MacroInput->Type)
			{
				VOXEL_MESSAGE(Error, "Invalid input {0}", CompiledNode);
				return nullptr;
			}

			Node = &Graph->NewNode(ENodeType::MacroInput, Source);
			Node->MacroInput().Guid = MacroInput->Guid;
			Node->MacroInput().Name = MacroInput->Name;
			Node->MacroInput().Type = MacroInput->Type;
		}
		break;
		case EVoxelMetaGraphCompiledNodeType::MacroOutput:
		{
			const FVoxelMetaGraphParameter* MacroOutput = Parameters.FindByKey(CompiledNode.Guid);
			if (!MacroOutput ||
				MacroOutput->ParameterType != EVoxelMetaGraphParameterType::MacroOutput ||
				!MacroOutput->Type.IsValid() ||
				!ensure(CompiledNode.InputPins.Num() == 1) ||
				!ensure(CompiledNode.OutputPins.Num() == 0) ||
				CompiledNode.InputPins[0].Type != MacroOutput->Type)
			{
				VOXEL_MESSAGE(Error, "Invalid output {0}", CompiledNode);
				return nullptr;
			}

			Node = &Graph->NewNode(ENodeType::MacroOutput, Source);
			Node->MacroOutput().Guid = MacroOutput->Guid;
			Node->MacroOutput().Name = MacroOutput->Name;
			Node->MacroOutput().Type = MacroOutput->Type;
		}
		break;
		}
		check(Node);

		NodesMap.Add(&CompiledNode, Node);

		for (const FVoxelMetaGraphCompiledPin& CompiledPin : CompiledNode.InputPins)
		{
			if (!CompiledPin.Type.IsValid())
			{
				VOXEL_MESSAGE(Error, "Invalid pin on {0}", CompiledNode);
				return nullptr;
			}

			FVoxelPinValue DefaultValue;
			if (!MakeDefaultValue(CompiledPin.Type, CompiledPin.DefaultValue, Runtime, DefaultValue))
			{
				VOXEL_MESSAGE(Error, "{0}: Invalid default value", &CompiledNode);
				return nullptr;
			}

			ensure(!DefaultValue.IsValid() || DefaultValue.GetType().IsDerivedFrom(CompiledPin.Type.WithoutTag()));

			Node->NewInputPin(CompiledPin.PinName, CompiledPin.Type.WithoutTag(), DefaultValue);
		}

		for (const FVoxelMetaGraphCompiledPin& CompiledPin : CompiledNode.OutputPins)
		{
			if (!CompiledPin.Type.IsValid())
			{
				VOXEL_MESSAGE(Error, "Invalid pin on {0}", CompiledNode);
				return nullptr;
			}

			Node->NewOutputPin(CompiledPin.PinName, CompiledPin.Type.WithoutTag());
		}
	}

	// Fixup links after all nodes are created
	for (const FVoxelMetaGraphCompiledNode& CompiledNode : CompiledGraph.Nodes)
	{
		FNode& Node = *NodesMap[&CompiledNode];

		for (int32 InputPinIndex = 0; InputPinIndex < CompiledNode.InputPins.Num(); InputPinIndex++)
		{
			const FVoxelMetaGraphCompiledPin& InputPin = CompiledNode.InputPins[InputPinIndex];
			if (InputPin.LinkedTo.Num() > 1)
			{
				VOXEL_MESSAGE(Error, "Too many pins linked to {0}.{1}", CompiledNode, InputPin.PinName);
				return nullptr;
			}

			for (const FVoxelMetaGraphCompiledPinRef& OutputPinRef : InputPin.LinkedTo)
			{
				check(!OutputPinRef.bIsInput);

				const FVoxelMetaGraphCompiledPin* OutputPin = CompiledGraph.FindPin(OutputPinRef);
				if (!ensure(OutputPin))
				{
					VOXEL_MESSAGE(Error, "Invalid pin ref on {0}", CompiledNode);
					continue;
				}

				const FVoxelMetaGraphCompiledNode& OtherCompiledNode = CompiledGraph.Nodes[OutputPinRef.NodeIndex];
				FNode& OtherNode = *NodesMap[&OtherCompiledNode];

				if (!OutputPin->Type.IsDerivedFrom(InputPin.Type))
				{
					VOXEL_MESSAGE(Error, "Invalid pin link from {0}.{1} to {2}.{3}: type mismatch: {4} vs {5}",
						CompiledNode,
						OutputPin->PinName,
						OtherCompiledNode,
						InputPin.PinName,
						OutputPin->Type.ToString(),
						InputPin.Type.ToString());

					continue;
				}

				Node.GetInputPin(InputPinIndex).MakeLinkTo(OtherNode.GetOutputPin(OutputPinRef.PinIndex));
			}
		}
	}

	// Input links are used to populate all links, check they're correct with output links
	for (const FVoxelMetaGraphCompiledNode& CompiledNode : CompiledGraph.Nodes)
	{
		FNode& Node = *NodesMap[&CompiledNode];

		for (int32 OutputPinIndex = 0; OutputPinIndex < CompiledNode.OutputPins.Num(); OutputPinIndex++)
		{
			const FVoxelMetaGraphCompiledPin& OutputPin = CompiledNode.OutputPins[OutputPinIndex];
			for (const FVoxelMetaGraphCompiledPinRef& InputPinRef : OutputPin.LinkedTo)
			{
				check(InputPinRef.bIsInput);

				const FVoxelMetaGraphCompiledPin* InputPin = CompiledGraph.FindPin(InputPinRef);
				if (!ensure(InputPin))
				{
					VOXEL_MESSAGE(Error, "Invalid pin ref on {0}", CompiledNode);
					continue;
				}

				const FVoxelMetaGraphCompiledNode& OtherCompiledNode = CompiledGraph.Nodes[InputPinRef.NodeIndex];
				FNode& OtherNode = *NodesMap[&OtherCompiledNode];

				if (!OutputPin.Type.IsDerivedFrom(InputPin->Type))
				{
					VOXEL_MESSAGE(Error, "Invalid pin link from {0}.{1} to {2}.{3}: type mismatch: {4} vs {5}",
						CompiledNode,
						OutputPin.PinName,
						OtherCompiledNode,
						InputPin->PinName,
						OutputPin.Type.ToString(),
						InputPin->Type.ToString());

					continue;
				}

				if (!ensure(Node.GetOutputPin(OutputPinIndex).IsLinkedTo(OtherNode.GetInputPin(InputPinRef.PinIndex))))
				{
					VOXEL_MESSAGE(Error, "Translation error: {0} -> {1}", Node, OtherNode);
				}
			}
		}
	}

	Graph->Check();
	return Graph;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FCompilerUtilities::FlattenGraph(FGraph& Graph, FVoxelRuntime* Runtime)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	while (FlattenGraphImpl(Graph, Runtime))
	{
		if (GVoxelGraphMessages->HasError())
		{
			return;
		}
	}

	if (GVoxelGraphMessages->HasError())
	{
		return;
	}

	for (const FNode& Node : Graph.GetNodes())
	{
		check(Node.Type != ENodeType::Macro);
	}
}

bool FCompilerUtilities::FlattenGraphImpl(FGraph& Graph, FVoxelRuntime* Runtime)
{
	for (FNode& MacroNode : Graph.GetNodes())
	{
		if (MacroNode.Type != ENodeType::Macro)
		{
			continue;
		}

		const UVoxelMetaGraph* MetaGraph = MacroNode.Macro().Graph;

		if (MetaGraph->CompiledGraph.Nodes.Num() == 0)
		{
			VOXEL_MESSAGE(Error, "Failed to compile {0}", MetaGraph);
			return false;
		}

		const TSharedPtr<FGraph> MacroGraph = TranslateCompiledGraph(
			MetaGraph->CompiledGraph,
			MetaGraph->Parameters,
			Runtime,
			MacroNode.Source);

		if (GVoxelGraphMessages->HasError() ||
			!ensure(MacroGraph))
		{
			return false;
		}

		if (GVoxelGraphMessages->HasError())
		{
			return false;
		}

		TMap<const FNode*, FNode*> OldToNewNodes;
		MacroGraph->CloneTo(Graph, &OldToNewNodes);

		TMap<FName, TArray<FNode*>> NameToInputNodes;
		TMap<FName, TArray<FNode*>> NameToOutputNodes;

		for (const auto& It : OldToNewNodes)
		{
			FNode* Node = It.Value;
			if (Node->Type == ENodeType::MacroInput)
			{
				NameToInputNodes.FindOrAdd(*Node->MacroInput().Guid.ToString()).Add(Node);
			}
			if (Node->Type == ENodeType::MacroOutput)
			{
				NameToOutputNodes.FindOrAdd(*Node->MacroOutput().Guid.ToString()).Add(Node);
			}
		}

		for (FPin& InputPin : MacroNode.GetInputPins())
		{
			TArray<FNode*> InputNodes;
			NameToInputNodes.RemoveAndCopyValue(InputPin.Name, InputNodes);

			if (InputNodes.Num() != 1)
			{
				VOXEL_MESSAGE(Error, "Outdated graph node: {0}", MacroNode);
				return false;
			}

			FNode& InputNode = *InputNodes[0];
			check(InputNode.GetInputPins().Num() == 1);
			check(InputNode.GetOutputPins().Num() == 1);
			const FPin& InternalInputPin = InputNode.GetInputPin(0);
			const FPin& InternalOutputPin = InputNode.GetOutputPin(0);

			FNode& Passthrough = Graph.NewNode(ENodeType::Passthrough, MacroNode.Source);
			Passthrough.NewInputPin(InputPin.Name, InputPin.Type);
			Passthrough.NewOutputPin(InputPin.Name, InputPin.Type);

			// If the macro node has something linked to it, or if the internal input node has nothing linked to it,
			// the macro node pin takes precedence
			if (InputPin.GetLinkedTo().Num() > 0 ||
				InternalInputPin.GetLinkedTo().Num() == 0)
			{
				InputPin.CopyInputPinTo(Passthrough.GetInputPin(0));
			}
			else
			{
				// If the macro node has nothing linked to it but the internal input node has something linked to it,
				// the input node takes precedence
				InternalInputPin.CopyInputPinTo(Passthrough.GetInputPin(0));
			}

			InternalOutputPin.CopyOutputPinTo(Passthrough.GetOutputPin(0));

			Graph.RemoveNode(InputNode);
		}

		for (FPin& OutputPin : MacroNode.GetOutputPins())
		{
			TArray<FNode*> OutputNodes;
			NameToOutputNodes.RemoveAndCopyValue(OutputPin.Name, OutputNodes);

			if (OutputNodes.Num() != 1)
			{
				VOXEL_MESSAGE(Error, "Outdated graph node: {0}", MacroNode);
				return false;
			}

			FNode& OutputNode = *OutputNodes[0];
			check(OutputNode.GetInputPins().Num() == 1);
			check(OutputNode.GetOutputPins().Num() == 0);
			const FPin& InputPin = OutputNode.GetInputPin(0);

			FNode& Passthrough = Graph.NewNode(ENodeType::Passthrough, MacroNode.Source);
			Passthrough.NewInputPin(OutputPin.Name, OutputPin.Type);
			Passthrough.NewOutputPin(OutputPin.Name, OutputPin.Type);

			InputPin.CopyInputPinTo(Passthrough.GetInputPin(0));
			OutputPin.CopyOutputPinTo(Passthrough.GetOutputPin(0));

			Graph.RemoveNode(OutputNode);
		}

		if (NameToInputNodes.Num() != 0 ||
			NameToOutputNodes.Num() != 0)
		{
			VOXEL_MESSAGE(Error, "Outdated graph node: {0}", MacroNode);
			return false;
		}

		Graph.RemoveNode(MacroNode);

		return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FCompilerUtilities::CheckWildcards(const FGraph& Graph)
{
	for (const FNode& Node : Graph.GetNodes())
	{
		for (const FPin& Pin : Node.GetPins())
		{
			if (Pin.Type.IsWildcard())
			{
				VOXEL_MESSAGE(Error, "Wildcard pin {0} needs to be converted. Please connect it to another pin or right click it -> Convert", Pin);
			}
		}
	}
}

void FCompilerUtilities::ReplaceTemplates(FGraph& Graph)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	while (ReplaceTemplatesImpl(Graph))
	{
		if (GVoxelGraphMessages->HasError())
		{
			return;
		}
	}
}

bool FCompilerUtilities::ReplaceTemplatesImpl(FGraph& Graph)
{
	for (FNode& Node : Graph.GetNodes())
	{
		if (Node.Type != ENodeType::Struct ||
			!Node.Struct().IsA<FVoxelTemplateNode>())
		{
			continue;
		}

		InitializeTemplatesPassthroughNodes(Graph, Node);

		Node.Struct().Get<FVoxelTemplateNode>().ExpandNode(Graph, Node);
		Graph.RemoveNode(Node);

		return true;
	}

	return false;
}

void FCompilerUtilities::InitializeTemplatesPassthroughNodes(FGraph& Graph, FNode& Node)
{
	for (FPin& InputPin : Node.GetInputPins())
	{
		FNode& Passthrough = Graph.NewNode(ENodeType::Passthrough, Node.Source);
		FPin& PassthroughInputPin = Passthrough.NewInputPin("Input" + InputPin.Name, InputPin.Type);
		FPin& PassthroughOutputPin = Passthrough.NewOutputPin("Output" + InputPin.Name, InputPin.Type);

		InputPin.CopyInputPinTo(PassthroughInputPin);

		InputPin.BreakAllLinks();
		InputPin.MakeLinkTo(PassthroughOutputPin);
	}

	for (FPin& OutputPin : Node.GetOutputPins())
	{
		FNode& Passthrough = Graph.NewNode(ENodeType::Passthrough, Node.Source);
		FPin& PassthroughInputPin = Passthrough.NewInputPin("Input" + OutputPin.Name, OutputPin.Type);
		PassthroughInputPin.SetDefaultValue(FVoxelPinValue(OutputPin.Type));
		FPin& PassthroughOutputPin = Passthrough.NewOutputPin("Output" + OutputPin.Name, OutputPin.Type);

		OutputPin.CopyOutputPinTo(PassthroughOutputPin);

		OutputPin.BreakAllLinks();
		OutputPin.MakeLinkTo(PassthroughInputPin);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FCompilerUtilities::RemovePassthroughs(FGraph& Graph)
{
	Graph.RemoveNodes([&](FNode& Node)
	{
		if (Node.Type != ENodeType::Passthrough)
		{
			return false;
		}

		const int32 Num = Node.GetInputPins().Num();
		check(Num == Node.GetOutputPins().Num());

		for (int32 Index = 0; Index < Num; Index++)
		{
			const FPin& InputPin = Node.GetInputPin(Index);
			const FPin& OutputPin = Node.GetOutputPin(Index);

			for (FPin& LinkedTo : OutputPin.GetLinkedTo())
			{
				InputPin.CopyInputPinTo(LinkedTo);
			}
		}

		return true;
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FCompilerUtilities::CheckOutputs(const FGraph& Graph)
{
	for (const FNode& Node : Graph.GetNodes())
	{
		if (Node.Type == ENodeType::MacroOutput)
		{
			VOXEL_MESSAGE(Error, "Cannot have outputs in main graph: {0}", Node);
		}
	}
}

void FCompilerUtilities::ReplaceParameters(FGraph& Graph, const FVoxelMetaGraphVariableCollection& Variables, FVoxelRuntime* Runtime)
{
	for (FNode& Node : Graph.GetNodesCopy())
	{
		if (Node.Type != ENodeType::Parameter)
		{
			continue;
		}

		const FVoxelMetaGraphVariable* Variable = Variables.Variables.Find(Node.Parameter().Guid);
		if (!ensure(Variable) ||
			!ensure(Variable->Name == Node.Parameter().Name) ||
			!ensure(Variable->GetType() == Node.Parameter().Type.GetInnerType().GetExposedType()))
		{
			VOXEL_MESSAGE(Error, "Invalid input {0}", Node);
			return;
		}

		check(Node.GetOutputPins().Num() == 1);

		FVoxelPinValue NewValue;
		if (!MakeDefaultValue(Node.Parameter().Type, Variable->Value, Runtime, NewValue))
		{
			VOXEL_MESSAGE(Error, "{0}: Invalid default value", Node);
			return;
		}

		for (FPin& LinkedTo : Node.GetOutputPin(0).GetLinkedTo())
		{
			LinkedTo.SetDefaultValue(NewValue);
		}

		Graph.RemoveNode(Node);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FCompilerUtilities::ReplaceCodeGenNodes(FGraph& Graph)
{
	VOXEL_FUNCTION_COUNTER();

	while (ReplaceCodeGenNodesImpl(Graph))
	{
		if (GVoxelGraphMessages->HasError())
		{
			return;
		}
	}

	RemoveUnusedNodes(Graph);

	for (const FNode& Node : Graph.GetNodes())
	{
		ensure(Node.Type == ENodeType::Struct);
	}
}

bool FCompilerUtilities::ReplaceCodeGenNodesImpl(FGraph& Graph)
{
	VOXEL_FUNCTION_COUNTER();

	const auto Process = [&](FPin& MainOutputPin, const TSet<const FNode*>& CodeGenNodes)
	{
		TMap<const FPin*, FPin*> OldToNewPins;
		const TSharedRef<FGraph> SubGraph = MakeSubGraph(Graph, CodeGenNodes.Array(), OldToNewPins);
		
		TMap<FPin*, const FPin*> NewToOldPins;
		for (auto& It : OldToNewPins)
		{
			NewToOldPins.Add(It.Value, It.Key);
		}

		FSourceNode NodeSource;
		for (const FNode* Node : CodeGenNodes)
		{
			NodeSource.ChildNodes.Add(Node->Source);
		}

		FNode& NewNode = Graph.NewNode(ENodeType::Struct, NodeSource);
		NewNode.Struct() = FVoxelInstancedStruct::Make<FVoxelNode_ExecCodeGen>();

		FVoxelNode_ExecCodeGen& NewVoxelNode = NewNode.Struct().Get<FVoxelNode_ExecCodeGen>();
		NewVoxelNode.Graph = SubGraph;

		// Add output pin
		{
			FPin& NewPin = NewNode.NewOutputPin("Output", MainOutputPin.Type);
			for (FPin& InputPin : MainOutputPin.GetLinkedTo())
			{
				if (CodeGenNodes.Contains(&InputPin.Node))
				{
					// A pin could be linked to both a pure node & a regular node
					continue;
				}

				NewPin.MakeLinkTo(InputPin);
			}

			NewVoxelNode.GraphOutputPin = OldToNewPins[&MainOutputPin];
			NewVoxelNode.OutputPinRef = NewVoxelNode.CreateOutputPin(MainOutputPin.Type, "Output");
		}

		// Add input pins
		int32 InputPinIndex = 0;
		for (const FNode* FunctionNode : CodeGenNodes)
		{
			for (const FPin& InputPin : FunctionNode->GetInputPins())
			{
				if (InputPin.GetLinkedTo().Num() == 0)
				{
					continue;
				}
				check(InputPin.GetLinkedTo().Num() == 1);

				FPin& OutputPin = InputPin.GetLinkedTo()[0];
				if (CodeGenNodes.Contains(&OutputPin.Node))
				{
					continue;
				}
				check(OutputPin.Node.Type == ENodeType::Struct);

				const FName PinName = FName("Input", ++InputPinIndex);

				NewNode.NewInputPin(PinName, InputPin.Type, InputPin.GetDefaultValue()).MakeLinkTo(OutputPin);

				NewVoxelNode.GraphInputPins.Add(OldToNewPins[&InputPin]);
				NewVoxelNode.InputPinRefs.Add(NewVoxelNode.CreateInputPin(InputPin.Type, PinName, InputPin.GetDefaultValue()));
			}
		}
		ensure(InputPinIndex == NewVoxelNode.GraphInputPins.Num());
		ensure(InputPinIndex == NewVoxelNode.InputPinRefs.Num());
		ensure(InputPinIndex == NewNode.GetInputPins().Num());

		// Break existing links to the output pin
		MainOutputPin.BreakAllLinks();
	};

	for (FNode& Node : Graph.GetNodes())
	{
		if (Node.Struct()->IsCodeGen())
		{
			continue;
		}

		for (FPin& InputPin : Node.GetInputPins())
		{
			if (InputPin.GetLinkedTo().Num() == 0 ||
				InputPin.Type.IsDerivedFrom<FVoxelExecBase>())
			{
				continue;
			}
			check(InputPin.GetLinkedTo().Num() == 1);

			FPin& OutputPin = InputPin.GetLinkedTo()[0];
			if (!OutputPin.Node.Struct()->IsCodeGen())
			{
				continue;
			}

			TSet<const FNode*> CodeGenNodes = FindNodesPredecessors({ &OutputPin.Node }, [&](const FNode& NodeIt)
			{
				return NodeIt.Struct()->IsCodeGen();
			});

			for (auto It = CodeGenNodes.CreateIterator(); It; ++It)
			{
				if (!(**It).Struct()->IsCodeGen())
				{
					It.RemoveCurrent();
				}
			}

			for (const FNode* CodeGenNode : CodeGenNodes)
			{
				const FVoxelNode::EExecType ExecType = CodeGenNode->Struct()->GetExecType();
				if (ExecType == FVoxelNode::EExecType::Any)
				{
					continue;
				}

				check(ExecType == FVoxelNode::EExecType::CodeGen);
				Process(OutputPin, CodeGenNodes);
				return true;
			}
		}
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FCompilerUtilities::RemoveUnusedNodes(FGraph& Graph)
{
	TArray<const FNode*> Nodes;
	for (const FNode& Node : Graph.GetNodes())
	{
		if (Node.Type == ENodeType::Struct &&
			Node.Struct()->HasSideEffects())
		{
			Nodes.Add(&Node);
		}
	}

	const TSet<const FNode*> ValidNodes = FindNodesPredecessors(Nodes);
	Graph.RemoveNodes([&](const FNode& Node)
	{
		return !ValidNodes.Contains(&Node);
	});
}

TSet<const FNode*> FCompilerUtilities::FindNodesPredecessors(
	const TArray<const FNode*>& Nodes,
	const TFunction<bool(const FNode&)>& ShouldFindPredecessors)
{
	TSet<const FNode*> Predecessors;

	TArray<const FNode*> NodesToVisit = Nodes;
	while (NodesToVisit.Num() > 0)
	{
		const FNode* Node = NodesToVisit.Pop(false);
		if (Predecessors.Contains(Node))
		{
			continue;
		}
		Predecessors.Add(Node);

		if (ShouldFindPredecessors &&
			!ShouldFindPredecessors(*Node))
		{
			continue;
		}

		for (const FPin& Pin : Node->GetInputPins())
		{
			if (Pin.Type.IsDerivedFrom<FVoxelExecBase>())
			{
				continue;
			}

			for (const FPin& LinkedTo : Pin.GetLinkedTo())
			{
				NodesToVisit.Add(&LinkedTo.Node);
			}
		}

		for (const FPin& Pin : Node->GetOutputPins())
		{
			if (!Pin.Type.IsDerivedFrom<FVoxelExecBase>())
			{
				continue;
			}

			for (const FPin& LinkedTo : Pin.GetLinkedTo())
			{
				NodesToVisit.Add(&LinkedTo.Node);
			}
		}
	}

	return Predecessors;
}

TArray<const FNode*> FCompilerUtilities::SortNodes(const TArray<const FNode*>& Nodes)
{
	VOXEL_FUNCTION_COUNTER();

	TSet<const FNode*> VisitedNodes;
	TArray<const FNode*> SortedNodes;
	TArray<const FNode*> NodesToSort = Nodes;

	while (NodesToSort.Num() > 0)
	{
		bool bHasRemovedNode = false;
		for (int32 Index = 0; Index < NodesToSort.Num(); Index++)
		{
			const FNode* Node = NodesToSort[Index];

			const bool bHasInputPin = INLINE_LAMBDA
			{
				for (const FPin& Pin : Node->GetInputPins())
				{
					for (const FPin& LinkedTo : Pin.GetLinkedTo())
					{
						if (!VisitedNodes.Contains(&LinkedTo.Node))
						{
							return true;
						}
					}
				}
				return false;
			};

			if (bHasInputPin)
			{
				continue;
			}

			VisitedNodes.Add(Node);
			SortedNodes.Add(Node);

			NodesToSort.RemoveAtSwap(Index);
			Index--;
			bHasRemovedNode = true;
		}

		if (!bHasRemovedNode &&
			NodesToSort.Num() > 0)
		{
			VOXEL_MESSAGE(Error, "Loop in a graph {0}", NodesToSort);
			return {};
		}
	}

	return SortedNodes;
}

TSharedRef<FGraph> FCompilerUtilities::MakeSubGraph(
	const FGraph& Graph,
	const TArray<const FNode*>& Nodes,
	TMap<const FPin*, FPin*>& OldToNewPins)
{
	VOXEL_FUNCTION_COUNTER();
	check(Nodes.Num() > 0);

	TMap<const FNode*, FNode*> OldToNewNodes;
	const TSharedRef<FGraph> NewGraph = Graph.Clone(&OldToNewNodes, &OldToNewPins);

	TSet<const FNode*> ValidNodes;
	for (const FNode* Node : Nodes)
	{
		ValidNodes.Add(OldToNewNodes[Node]);
	}

	NewGraph->RemoveNodes([&](const FNode& Node)
	{
		return !ValidNodes.Contains(&Node);
	});
	return NewGraph;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FCompilerUtilities::MakeDefaultValue(
	const FVoxelPinType& Type,
	const FVoxelPinValue& ExposedDefaultValue,
	FVoxelRuntime* Runtime,
	FVoxelPinValue& NewValue)
{
	if (Type.IsWildcard())
	{
		return true;
	}

	if (Type.IsBuffer())
	{
		FVoxelPinValue ConstantDefaultValue;
		if (!MakeDefaultValue(Type.GetInnerType(), ExposedDefaultValue, Runtime, ConstantDefaultValue))
		{
			return false;
		}

		NewValue = FVoxelPinValue(Type.WithoutTag());
		NewValue.Get<FVoxelBuffer>().InitializeFromConstant(FVoxelSharedPinValue(ConstantDefaultValue));

		return true;
	}

	if (ExposedDefaultValue.GetType().IsEnum())
	{
		const int64 EnumValue = ExposedDefaultValue.GetEnum();
		ensure(0 <= EnumValue && EnumValue < 256);
		NewValue = FVoxelPinValue::Make<uint8>(EnumValue);
		return true;
	}

	if (const TVoxelInstancedStruct<FVoxelExposedPinType>& ExposedTypeInfo = Type.GetExposedTypeInfo())
	{
		if (Runtime)
		{
			NewValue = ExposedTypeInfo->Compute(ExposedDefaultValue, *Runtime);
		}
		else
		{
			NewValue = ExposedTypeInfo->GetComputedType().WithoutTag().MakeSafeDefault();
		}
		return true;
	}

	if (!ensure(ExposedDefaultValue.GetType() == Type))
	{
		return false;
	}

	NewValue = ExposedDefaultValue.WithoutTag();
	return true;
}

END_VOXEL_NAMESPACE(MetaGraph)