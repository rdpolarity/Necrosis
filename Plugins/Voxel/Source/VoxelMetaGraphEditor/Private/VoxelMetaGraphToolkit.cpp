// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphToolkit.h"
#include "VoxelNode.h"
#include "VoxelEdGraph.h"
#include "VoxelMetaGraph.h"
#include "VoxelGraphMessages.h"
#include "VoxelMetaGraphSchema.h"
#include "Widgets/SVoxelMetaGraphMembers.h"
#include "Widgets/SVoxelMetaGraphPreview.h"
#include "VoxelMetaGraphCompilerUtilities.h"
#include "VoxelMetaGraphEditorCompiler.h"
#include "VoxelMetaGraphEditorCompilerPasses.h"
#include "Nodes/VoxelMetaGraphKnotNode.h"
#include "Nodes/VoxelMetaGraphDebugNode.h"
#include "Nodes/VoxelMetaGraphMacroNode.h"
#include "Nodes/VoxelMetaGraphStructNode.h"
#include "Nodes/VoxelMetaGraphParameterNode.h"
#include "Widgets/SVoxelMetaGraphPreviewStats.h"
#include "Nodes/VoxelMetaGraphLocalVariableNode.h"
#include "Nodes/VoxelMetaGraphMacroParameterNode.h"
#include "Customizations/VoxelMetaGraphNodeCustomization.h"
#include "Customizations/VoxelMetaGraphParameterCustomization.h"

#include "GraphEditor.h"
#include "SGraphPanel.h"
#include "SGraphActionMenu.h"
#include "MessageLogModule.h"
#include "IMessageLogListing.h"

class FVoxelMetaGraphCommands : public TVoxelCommands<FVoxelMetaGraphCommands>
{
public:
	TSharedPtr<FUICommandInfo> RecordStats;
	TSharedPtr<FUICommandInfo> DetailedStats;
	TSharedPtr<FUICommandInfo> InclusiveStats;
	TSharedPtr<FUICommandInfo> UpdateActorsOnChange;
	TSharedPtr<FUICommandInfo> RefreshActors;

	virtual void RegisterCommands() override;
};

DEFINE_VOXEL_COMMANDS(FVoxelMetaGraphCommands);

void FVoxelMetaGraphCommands::RegisterCommands()
{
	VOXEL_UI_COMMAND(RecordStats, "Record Stats", "Record node statistics", EUserInterfaceActionType::ToggleButton, {});
	VOXEL_UI_COMMAND(DetailedStats, "Detailed Stats", "", EUserInterfaceActionType::ToggleButton, {});
	VOXEL_UI_COMMAND(InclusiveStats, "Inclusive Stats", "If true, will show stats of the node and all the children calls", EUserInterfaceActionType::ToggleButton, {});
	VOXEL_UI_COMMAND(UpdateActorsOnChange, "Update Actors on Change", "If true, will update actors on every graph update", EUserInterfaceActionType::ToggleButton, {});
	VOXEL_UI_COMMAND(RefreshActors, "Refresh Actors", "On click will refresh all actors", EUserInterfaceActionType::Button, {});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VOXEL_RUN_ON_STARTUP_EDITOR(RegisterFVoxelMetaGraphEditorToolkit)
{
	FVoxelBaseEditorToolkit::RegisterToolkit<FVoxelMetaGraphEditorToolkit>(UVoxelMetaGraph::StaticClass());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FName FVoxelMetaGraphEditorToolkit::FindGraphId(const UEdGraph* Graph) const
{
	for (const auto& It : GetAssetAs<UVoxelMetaGraph>().Graphs)
	{
		if (It.Value == Graph)
		{
			return It.Key;
		}
	}
	return {};
}

UEdGraph* FVoxelMetaGraphEditorToolkit::GetGraph(FName GraphId) const
{
	return GetAssetAs<UVoxelMetaGraph>().Graphs.FindRef(GraphId);
}

TSharedPtr<SGraphEditor> FVoxelMetaGraphEditorToolkit::GetGraphEditor(FName GraphId) const
{
	return GraphEditorsMap.FindRef(GraphId);
}

FName FVoxelMetaGraphEditorToolkit::GetActiveGraphId() const
{
	return bIsInOnDebugGraph ? DebugGraphId : MainGraphId;
}

void FVoxelMetaGraphEditorToolkit::CreateGraph(FName GraphId)
{
	TMap<FName, TObjectPtr<UEdGraph>>& Graphs = GetAssetAs<UVoxelMetaGraph>().Graphs;
	if (!ensure(!Graphs.FindRef(GraphId)))
	{
		return;
	}

	TObjectPtr<UEdGraph>& Graph = Graphs.Add(GraphId);
	Graph = NewObject<UVoxelEdGraph>(&GetAssetAs<UVoxelMetaGraph>(), NAME_None, RF_Transactional);
	Graph->Schema = UVoxelMetaGraphSchema::StaticClass();
	CastChecked<UVoxelEdGraph>(Graph)->WeakToolkit = SharedThis(this);

	if (!GraphEditorCommands)
	{
		// We're in SetupObjectToEdit, let CreateInternalWidgets create the graph editor
		return;
	}

	ensure(!GraphEditorsMap.Contains(GraphId));
	GraphEditorsMap.Add(GraphId, CreateGraphEditor(Graph));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TArray<TSharedPtr<SGraphEditor>> FVoxelMetaGraphEditorToolkit::GetGraphEditors() const
{
	TArray<TSharedPtr<SGraphEditor>> Result;
	GraphEditorsMap.GenerateValueArray(Result);
	return Result;
}

void FVoxelMetaGraphEditorToolkit::BindToolkitCommands()
{
	FVoxelGraphEditorToolkit::BindToolkitCommands();

	ToolkitCommands->MapAction(
		FVoxelMetaGraphCommands::Get().RecordStats,
		MakeLambdaDelegate([=]
		{
			FVoxelTaskStat::bStaticRecordStats = !FVoxelTaskStat::bStaticRecordStats;

			QueueRefresh();
		}),
		MakeLambdaDelegate([]
		{
			return true;
		}),
		MakeLambdaDelegate([]
		{
			return FVoxelTaskStat::bStaticRecordStats;
		}));

	ToolkitCommands->MapAction(
		FVoxelMetaGraphCommands::Get().DetailedStats,
		MakeLambdaDelegate([]
		{
			FVoxelTaskStat::bStaticDetailedStats = !FVoxelTaskStat::bStaticDetailedStats;
		}),
		MakeLambdaDelegate([]
		{
			return true;
		}),
		MakeLambdaDelegate([]
		{
			return FVoxelTaskStat::bStaticDetailedStats;
		}));

	ToolkitCommands->MapAction(
		FVoxelMetaGraphCommands::Get().InclusiveStats,
		MakeLambdaDelegate([]
		{
			FVoxelTaskStat::bStaticInclusiveStats = !FVoxelTaskStat::bStaticInclusiveStats;
		}),
		MakeLambdaDelegate([]
		{
			return true;
		}),
		MakeLambdaDelegate([]
		{
			return FVoxelTaskStat::bStaticInclusiveStats;
		}));

	ToolkitCommands->MapAction(
		FVoxelMetaGraphCommands::Get().UpdateActorsOnChange,
		MakeLambdaDelegate([=]
		{
			bUpdateOnChange = !bUpdateOnChange;
		}),
		MakeLambdaDelegate([]
		{
			return true;
		}),
		MakeLambdaDelegate([=]
		{
			return bUpdateOnChange;
		}));

	ToolkitCommands->MapAction(
		FVoxelMetaGraphCommands::Get().RefreshActors,
		MakeLambdaDelegate([=]
		{
			bRefreshQueued = true;
		}));
}

void FVoxelMetaGraphEditorToolkit::BuildToolbar(FToolBarBuilder& ToolbarBuilder)
{
	FVoxelGraphEditorToolkit::BuildToolbar(ToolbarBuilder);

	ToolbarBuilder.BeginSection("Voxel");
	{
		ToolbarBuilder.AddToolBarButton(FVoxelMetaGraphCommands::Get().RecordStats);
		ToolbarBuilder.AddToolBarButton(FVoxelMetaGraphCommands::Get().DetailedStats);
		ToolbarBuilder.AddToolBarButton(FVoxelMetaGraphCommands::Get().InclusiveStats);
		ToolbarBuilder.AddToolBarButton(FVoxelMetaGraphCommands::Get().UpdateActorsOnChange);
		ToolbarBuilder.AddToolBarButton(FVoxelMetaGraphCommands::Get().RefreshActors);
	}
	ToolbarBuilder.EndSection();
}

TSharedRef<FTabManager::FLayout> FVoxelMetaGraphEditorToolkit::GetLayout() const
{
	return FTabManager::NewLayout("FVoxelMetaGraphEditorToolkit_Layout_v3")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Vertical)
				->SetSizeCoefficient(0.1f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.4f)
					->AddTab(ViewportTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.2f)
					->SetHideTabWell(true)
					->AddTab(PreviewStatsTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.4f)
					->AddTab(MembersTabId, ETabState::OpenedTab)
				)
			)
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Vertical)
				->SetSizeCoefficient(0.8f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.8f)
					->SetHideTabWell(true)
					->AddTab(GraphTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.2f)
					->AddTab(MessagesTabId, ETabState::OpenedTab)
				)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.1f)
				->AddTab(DetailsTabId, ETabState::OpenedTab)
			)
		);
}

void FVoxelMetaGraphEditorToolkit::RegisterTabs(FRegisterTab RegisterTab)
{
	RegisterTab(ViewportTabId, VOXEL_LOCTEXT("Preview"), "LevelEditor.Tabs.Viewports", GraphPreview);
	RegisterTab(PreviewStatsTabId, VOXEL_LOCTEXT("Stats"), "MaterialEditor.ToggleMaterialStats.Tab", GraphPreviewStats);
	RegisterTab(DetailsTabId, VOXEL_LOCTEXT("Details"), "LevelEditor.Tabs.Details", GetDetailsView());
	RegisterTab(MembersTabId, VOXEL_LOCTEXT("Members"), "ClassIcon.BlueprintCore", GraphMembers);
	RegisterTab(MessagesTabId, VOXEL_LOCTEXT("Messages"), "MessageLog.TabIcon", MessagesWidget);
	RegisterTab(GraphTabId, VOXEL_LOCTEXT("Graph"), "GraphEditor.EventGraph_16x", GetGraphEditor(MainGraphId));
	RegisterTab(DebugGraphTabId, VOXEL_LOCTEXT("Debug Graph"), "GraphEditor.EventGraph_16x", GetGraphEditor(DebugGraphId));
}

void FVoxelMetaGraphEditorToolkit::SetupObjectToEdit()
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	UVoxelMetaGraph& MetaGraph = GetAssetAs<UVoxelMetaGraph>();

	if (!MetaGraph.Graphs.FindRef(MainGraphId))
	{
		CreateGraph(MainGraphId);
	}

	MetaGraph.OnParametersChanged.AddSP(this, &FVoxelMetaGraphEditorToolkit::FixupGraphProperties);

	if (!MetaGraph.Graphs.FindRef(DebugGraphId))
	{
		CreateGraph(DebugGraphId);
	}

	MetaGraph.Graphs[DebugGraphId]->SetFlags(RF_Transient);
	MetaGraph.Graphs[DebugGraphId]->ClearFlags(RF_Transactional);

	MetaGraph.OnDebugGraph.AddLambda(MakeWeakPtrLambda(this, [=](const TSharedRef<FVoxelMetaGraphDebugGraph>& Graph)
	{
		FVoxelSystemUtilities::DelayedCall(MakeWeakPtrLambda(this, [=]
		{
			OnDebugGraph(*Graph);
		}));
	}));

	FVoxelMessages::OnNodeMessageLogged.Add(MakeWeakPtrDelegate(this, [=](const UEdGraph* InGraph, const TSharedRef<FTokenizedMessage>& Message)
	{
		check(IsInGameThread());

		if (!InGraph ||
			&GetAssetAs<UVoxelMetaGraph>() != InGraph->GetOuter() ||
			// MessagesListing might be null if the error is triggered from a graph using this meta graph as macro
			!MessagesListing)
		{
			return;
		}

		MessagesListing->AddMessage(Message);
	}));

	FVoxelMessages::OnClearNodeMessages.Add(MakeWeakPtrDelegate(this, [=](const UEdGraph* InGraph)
	{
		check(IsInGameThread());
		
		if (!InGraph ||
			&GetAssetAs<UVoxelMetaGraph>() != InGraph->GetOuter() ||
			// MessagesListing might be null if the error is triggered from a graph using this meta graph as macro
			!MessagesListing)
		{
			return;
		}

		MessagesListing->ClearMessages();
	}));

	{
		ensure(!bDisableOnGraphChanged);
		TGuardValue<bool> OnGraphChangedGuard(bDisableOnGraphChanged, true);

		// Disable SetDirty
		ensure(!GIsEditorLoadingPackage);
		TGuardValue<bool> SetDirtyGuard(GIsEditorLoadingPackage, true);

		TMap<FName, TObjectPtr<UEdGraph>>& Graphs = GetAssetAs<UVoxelMetaGraph>().Graphs;
		for (auto It = Graphs.CreateIterator(); It; ++It)
		{
			UEdGraph* Graph = It.Value();
			if (!ensure(Graph))
			{
				It.RemoveCurrent();
				continue;
			}

			CastChecked<UVoxelEdGraph>(Graph)->WeakToolkit = SharedThis(this);

			for (UEdGraphNode* Node : Graph->Nodes)
			{
				Node->ReconstructNode();
			}
		}
	}

	FVoxelSystemUtilities::DelayedCall([=]
	{
		if (!FCompilerUtilities::Compile(GetAssetAs<UVoxelMetaGraph>(), {}, nullptr))
		{
			GetAssetAs<UVoxelMetaGraph>().Modify();
		}
		OnGraphChanged();
	});
}

void FVoxelMetaGraphEditorToolkit::CreateInternalWidgets()
{
	FVoxelGraphEditorToolkit::CreateInternalWidgets();

	GraphPreview =
		SNew(SVoxelMetaGraphPreview)
		.MetaGraph(&GetAssetAs<UVoxelMetaGraph>());

	GraphPreviewStats = GraphPreview->ConstructStats();

	GraphMembers =
		SNew(SVoxelMetaGraphMembers)
		.MetaGraph(&GetAssetAs<UVoxelMetaGraph>())
		.Toolkit(SharedThis(this));

	FMessageLogInitializationOptions LogOptions;
	LogOptions.MaxPageCount = 1;
	LogOptions.bAllowClear = false;
	LogOptions.bShowInLogWindow = false;
	
	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	MessagesListing = MessageLogModule.CreateLogListing("VoxelMetaGraphErrors", LogOptions);
	MessagesWidget = MessageLogModule.CreateLogListingWidget(MessagesListing.ToSharedRef());

	for (const auto& It : GetAssetAs<UVoxelMetaGraph>().Graphs)
	{
		const FName GraphId = It.Key;

		ensure(!GraphEditorsMap.Contains(GraphId));
		GraphEditorsMap.Add(GraphId, CreateGraphEditor(It.Value));
	}
}

void FVoxelMetaGraphEditorToolkit::OnGraphChangedImpl()
{
	VOXEL_FUNCTION_COUNTER();

	if (bIsInOnDebugGraph)
	{
		// Avoid recursive calls
		return;
	}

	FVoxelGraphEditorToolkit::OnGraphChangedImpl();

	UVoxelMetaGraph& MetaGraph = GetAssetAs<UVoxelMetaGraph>();

	// Fixup reroute nodes
	for (UEdGraphNode* GraphNode : GetGraph(MainGraphId)->Nodes)
	{
		if (UVoxelMetaGraphKnotNode* Node = Cast<UVoxelMetaGraphKnotNode>(GraphNode))
		{
			Node->PropagatePinType();
		}
	}

	// Fixup parameters
	{
		TSet<FGuid> UnusedParameters;
		for (const FVoxelMetaGraphParameter& Input : MetaGraph.Parameters)
		{
			if (Input.ParameterType != EVoxelMetaGraphParameterType::MacroInput &&
				Input.ParameterType != EVoxelMetaGraphParameterType::MacroOutput)
			{
				continue;
			}

			UnusedParameters.Add(Input.Guid);
		}

		for (UEdGraphNode* GraphNode : GetGraph(MainGraphId)->Nodes)
		{
			if (const UVoxelMetaGraphMacroParameterNode* MacroParameterNode = Cast<UVoxelMetaGraphMacroParameterNode>(GraphNode))
			{
				UnusedParameters.Remove(MacroParameterNode->Guid);
			}
			else if (const UVoxelMetaGraphLocalVariableDeclarationNode* LocalVariableNode = Cast<UVoxelMetaGraphLocalVariableDeclarationNode>(GraphNode))
			{
				if (const FVoxelMetaGraphParameter* Parameter = LocalVariableNode->GetParameter())
				{
					UEdGraphPin* InputPin = LocalVariableNode->GetInputPin(0);
					Parameter->DefaultValue.ApplyToPinDefaultValue(*InputPin);
				}
			}
		}

		if (UnusedParameters.Num() > 0)
		{
			TGuardValue<bool> Guard(bDisableOnGraphChanged, true);
			const FVoxelTransaction Transaction(MetaGraph);

			for (const FGuid& Guid : UnusedParameters)
			{
				MetaGraph.Parameters.RemoveAll([&](const FVoxelMetaGraphParameter& Parameter)
				{
					return Parameter.Guid == Guid;
				});
			}
		}
	}

	if (!CompileGraph())
	{
		return;
	}
	
	// Compile for errors
	{
		VOXEL_USE_NAMESPACE(MetaGraph);
		FCompilerUtilities::Compile(GetAssetAs<UVoxelMetaGraph>(), {}, nullptr);
	}

	MetaGraph.OnChanged.Broadcast();

	if (bUpdateOnChange)
	{
		QueueRefresh();
	}
}

void FVoxelMetaGraphEditorToolkit::FixupGraphProperties()
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelGraphDelayOnGraphChangedScope DelayScope;

	UVoxelMetaGraph& MetaGraph = GetAssetAs<UVoxelMetaGraph>();

	bool bAddedParameters = false;
	TArray<UEdGraphNode*> NodesToDelete;

	TSet<FGuid> ParametersToAdd;
	TMap<FGuid, const FVoxelMetaGraphParameter*> Parameters;
	for (const FVoxelMetaGraphParameter& Parameter : MetaGraph.Parameters)
	{
		Parameters.Add(Parameter.Guid, &Parameter);

		if (Parameter.ParameterType == EVoxelMetaGraphParameterType::MacroInput ||
			Parameter.ParameterType == EVoxelMetaGraphParameterType::MacroOutput)
		{
			ParametersToAdd.Add(Parameter.Guid);
		}
	}

	for (UEdGraphNode* Node : GetGraph(MainGraphId)->Nodes)
	{
		if (UVoxelMetaGraphParameterNode* ParameterNode = Cast<UVoxelMetaGraphParameterNode>(Node))
		{
			if (MetaGraph.bIsMacroGraph)
			{
				NodesToDelete.Add(Node);
				continue;
			}

			// Reconstruct to be safe
			ParameterNode->ReconstructNode();
		}
		else if (UVoxelMetaGraphMacroParameterNode* MacroParameterNode = Cast<UVoxelMetaGraphMacroParameterNode>(Node))
		{
			if (!MetaGraph.bIsMacroGraph)
			{
				NodesToDelete.Add(Node);
				continue;
			}

			const FVoxelMetaGraphParameter* Parameter = Parameters.FindRef(MacroParameterNode->Guid);
			if (!Parameter)
			{
				NodesToDelete.Add(MacroParameterNode);
				continue;
			}
			
			ParametersToAdd.Remove(MacroParameterNode->Guid);

			// Reconstruct to be safe
			MacroParameterNode->ReconstructNode();
		}
		else if (UVoxelMetaGraphLocalVariableNode* LocalVariableNode = Cast<UVoxelMetaGraphLocalVariableNode>(Node))
		{
			// Reconstruct to be safe
			LocalVariableNode->ReconstructNode();
		}
	}

	for (const FGuid& Guid : ParametersToAdd)
	{
		FVector2D Position;
		Position.X = 400; // TODO: Better search for location...?
		Position.Y = 200; // TODO: Better search for location...?

		FVoxelMetaGraphSchemaAction_NewParameterUsage Action;
		Action.Guid = Guid;
		Action.ParameterType = MetaGraph.FindParameterByGuid(Guid)->ParameterType;
		Action.PerformAction(GetGraph(MainGraphId), nullptr, Position);
	}

	bAddedParameters |= ParametersToAdd.Num() > 0;

	DeleteNodes(NodesToDelete);

	if (bAddedParameters)
	{
		GetGraphEditor(MainGraphId)->ZoomToFit(true);
	}

	GetGraph(MainGraphId)->NotifyGraphChanged();

	// Update parameter nodes if needed
	OnGraphChanged();
}

void FVoxelMetaGraphEditorToolkit::OnSelectedNodesChanged(const TSet<UObject*>& NewSelection)
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	FVoxelGraphEditorToolkit::OnSelectedNodesChanged(NewSelection);

	if (NewSelection.Num() != 1)
	{
		SelectParameter(FGuid(), false);
		return;
	}

	for (UObject* Node : NewSelection)
	{
		if (const UVoxelMetaGraphParameterNodeBase* ParameterNode = Cast<UVoxelMetaGraphParameterNodeBase>(Node))
		{
			SelectParameter(ParameterNode->Guid, false);
			return;
		}
		else if (UVoxelMetaGraphStructNode* StructNode = Cast<UVoxelMetaGraphStructNode>(Node))
		{
			SelectParameter({}, false);

			GetDetailsView()->SetGenericLayoutDetailsDelegate(MakeLambdaDelegate([this]() -> TSharedRef<IDetailCustomization>
			{
				return MakeShared<FVoxelNodeCustomization>(SharedThis(this));
			}));
			GetDetailsView()->SetObject(StructNode);
			return;
		}
		else
		{
			SelectParameter({}, false);

			GetDetailsView()->SetGenericLayoutDetailsDelegate(nullptr);
			GetDetailsView()->SetObject(Node);
			return;
		}
	}

	SelectParameter({}, false);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMetaGraphEditorToolkit::Tick()
{
	if (bRefreshQueued)
	{
		bRefreshQueued = false;
		GetAssetAs<UVoxelMetaGraph>().UpdateVoxelActors();
		GraphPreview->QueueUpdate();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelMetaGraphEditorToolkit::CompileGraph()
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_USE_NAMESPACE(MetaGraph);

	FVoxelGraphMessages Messages(GetGraph(MainGraphId));

	for (const UEdGraphNode* Node : GetGraph(MainGraphId)->Nodes)
	{
		for (const UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin->bOrphanedPin)
			{
				VOXEL_MESSAGE(Error, "Invalid pin on {0}", Node);
			}
		}
	}

	if (Messages.HasError())
	{
		// Clear nodes to not have outdated errors
		GetAssetAs<UVoxelMetaGraph>().CompiledGraph.Nodes = {};
		return false;
	}

	TArray<FFEditorCompilerPass> Passes;
	Passes.Add([=](FEditorCompiler& Compiler)
	{
		FEditorCompilerPasses::SetupPinPreview(Compiler, GetAssetAs<UVoxelMetaGraph>().PreviewedPin);
		return true;
	});
	Passes.Add(FEditorCompilerPasses::AddToArrayNodes);
	Passes.Add(FEditorCompilerPasses::RemoveSplitPins);
	Passes.Add(FEditorCompilerPasses::FixupLocalVariables);

	FEditorGraph Graph;
	if (!FEditorCompilerUtilities::CompileGraph(*GetGraph(MainGraphId), Graph, Passes))
	{
		VOXEL_MESSAGE(Error, "Failed to translate graph");
		return false;
	}

	TMap<const FEditorNode*, int32> NodeIndices;
	TArray<FVoxelMetaGraphCompiledNode> CompiledNodes;

	for (const FEditorNode* Node : Graph.Nodes)
	{
		FVoxelMetaGraphCompiledNode CompiledNode;
		CompiledNode.SourceGraphNode = Node->SourceGraphNode;
		
		if (const UVoxelMetaGraphStructNode* StructNode = Cast<UVoxelMetaGraphStructNode>(Node->GraphNode))
		{
			CompiledNode.Type = EVoxelMetaGraphCompiledNodeType::Struct;
			CompiledNode.Struct = StructNode->Struct;
		}
		else if (const UVoxelMetaGraphMacroNode* MacroNode = Cast<UVoxelMetaGraphMacroNode>(Node->GraphNode))
		{
			CompiledNode.Type = EVoxelMetaGraphCompiledNodeType::Macro;
			CompiledNode.MetaGraph = MacroNode->MetaGraph;
		}
		else if (const UVoxelMetaGraphParameterNode* ParameterNode = Cast<UVoxelMetaGraphParameterNode>(Node->GraphNode))
		{
			CompiledNode.Type = EVoxelMetaGraphCompiledNodeType::Parameter;
			CompiledNode.Guid = ParameterNode->Guid;
		}
		else if (const UVoxelMetaGraphMacroParameterNode* MacroParameterNode = Cast<UVoxelMetaGraphMacroParameterNode>(Node->GraphNode))
		{
			switch (MacroParameterNode->Type)
			{
			default:
			{
				ensure(false);
				return false;
			}
			case EVoxelMetaGraphParameterType::MacroInput:
			{
				CompiledNode.Type = EVoxelMetaGraphCompiledNodeType::MacroInput;
				CompiledNode.Guid = MacroParameterNode->Guid;
			}
			break;
			case EVoxelMetaGraphParameterType::MacroOutput: 
			{
				CompiledNode.Type = EVoxelMetaGraphCompiledNodeType::MacroOutput;
				CompiledNode.Guid = MacroParameterNode->Guid;
			}
			break;
			}
		}
		else
		{
			ensure(false);
			return false;
		}

		NodeIndices.Add(Node, CompiledNodes.Add(CompiledNode));
	}

	TMap<const FEditorPin*, FVoxelMetaGraphCompiledPin*> PinMap;
	TMap<FVoxelMetaGraphCompiledPin*, int32> PinIndices;
	TMap<FVoxelMetaGraphCompiledPin*, bool> IsInput;
	for (const auto& It : NodeIndices)
	{
		const FEditorNode* Node = It.Key;
		FVoxelMetaGraphCompiledNode& CompiledNode = CompiledNodes[It.Value];

		// Reserve as we take raw ptrs in PinMap
		CompiledNode.InputPins.Reserve(Node->InputPins.Num());
		CompiledNode.OutputPins.Reserve(Node->OutputPins.Num());

		const auto MakePin = [&](const FEditorPin& Pin)
		{
			FVoxelMetaGraphCompiledPin CompiledPin;
			CompiledPin.Type = Pin.PinType;
			CompiledPin.PinName = Pin.PinName;
			CompiledPin.DefaultValue = Pin.DefaultValue;
			return CompiledPin;
		};
		
		for (const FEditorPin* Pin : Node->InputPins)
		{
			FVoxelMetaGraphCompiledPin* CompiledPin = &CompiledNode.InputPins.Add_GetRef(MakePin(*Pin));

			PinMap.Add(Pin, CompiledPin);
			PinIndices.Add(CompiledPin, CompiledNode.InputPins.Num() - 1);
			IsInput.Add(CompiledPin, true);
		}
		for (const FEditorPin* Pin : Node->OutputPins)
		{
			FVoxelMetaGraphCompiledPin* CompiledPin = &CompiledNode.OutputPins.Add_GetRef(MakePin(*Pin));

			PinMap.Add(Pin, CompiledPin);
			PinIndices.Add(CompiledPin, CompiledNode.OutputPins.Num() - 1);
			IsInput.Add(CompiledPin, false);
		}
	}

	// Link the pins once they're all allocated
	for (const auto& It : NodeIndices)
	{
		const FEditorNode* Node = It.Key;

		const auto AddPin = [&](const FEditorPin* Pin)
		{
			FVoxelMetaGraphCompiledPin* CompiledPin = PinMap[Pin];
			for (const FEditorPin* OtherPin : Pin->LinkedTo)
			{
				const FVoxelMetaGraphCompiledPin* OtherCompiledPin = PinMap[OtherPin];

				CompiledPin->LinkedTo.Add(FVoxelMetaGraphCompiledPinRef
				{
					NodeIndices[OtherPin->Node],
					PinIndices[OtherCompiledPin],
					IsInput[OtherCompiledPin]
				});
			}
		};

		for (const FEditorPin* Pin : Node->InputPins)
		{
			AddPin(Pin);
		}
		for (const FEditorPin* Pin : Node->OutputPins)
		{
			AddPin(Pin);
		}
	}

	GetAssetAs<UVoxelMetaGraph>().CompiledGraph.Nodes = CompiledNodes;

	return true;
}

void FVoxelMetaGraphEditorToolkit::OnDebugGraph(const FVoxelMetaGraphDebugGraph& InGraph)
{
	VOXEL_USE_NAMESPACE(MetaGraph);
	
	FScopedTransaction Transaction(VOXEL_LOCTEXT("Update debug graph"));

	ensure(!bIsInOnDebugGraph);
	TGuardValue<bool> Guard(bIsInOnDebugGraph, true);

	UEdGraph* DebugGraph = GetGraph(DebugGraphId);
	if (!ensure(DebugGraph) ||
		!ensure(DebugGraph == GetActiveGraph()))
	{
		return;
	}

	DeleteNodes(DebugGraph->Nodes);

	const TSharedRef<FGraph> Graph = InGraph.Clone();

	TMap<const FNode*, UVoxelMetaGraphDebugNode*> NodeMap;
	for (const FNode& Node : Graph->GetNodes())
	{
		FGraphNodeCreator<UVoxelMetaGraphDebugNode> NodeCreator(*DebugGraph);
		UVoxelMetaGraphDebugNode* DebugNode = NodeCreator.CreateNode(false);
		DebugNode->Node = &Node;
		DebugNode->Graph = Graph;
		NodeCreator.Finalize();

		NodeMap.Add(&Node, DebugNode);

		if (!ensure(Node.Source.GraphNodes.Num() > 0))
		{
			continue;
		}

		const UEdGraphNode* GraphNode = Node.Source.GraphNodes[0].Get();
		if (!ensure(GraphNode))
		{
			continue;
		}

		DebugNode->NodePosX = GraphNode->NodePosX * 2;
		DebugNode->NodePosY = GraphNode->NodePosY * 2;
	}
	
	for (const FNode& Node : Graph->GetNodes())
	{
		UVoxelMetaGraphDebugNode* DebugNode = NodeMap[&Node];

		for (const FPin& Pin : Node.GetPins())
		{
			UEdGraphPin* DebugPin = DebugNode->PinMap[&Pin];

			if (Pin.Direction == EPinDirection::Input)
			{
				Pin.GetDefaultValue().ApplyToPinDefaultValue(*DebugPin);
			}

			for (FPin& OtherPin : Pin.GetLinkedTo())
			{
				UVoxelMetaGraphDebugNode* OtherNode = NodeMap[&OtherPin.Node];
				UEdGraphPin* OtherDebugPin = OtherNode->PinMap[&OtherPin];
				DebugPin->MakeLinkTo(OtherDebugPin);
			}
		}
	}

	GetActiveGraphEditor()->ZoomToFit(false);
}

FVector2D FVoxelMetaGraphEditorToolkit::FindLocationInGraph() const
{
	const TSharedPtr<SGraphEditor> Editor = GetActiveGraphEditor();
	UEdGraph* ActiveGraph = GetActiveGraph();

	if (!Editor)
	{
		if (!ActiveGraph)
		{
			return FVector2D::ZeroVector;
		}

		return ActiveGraph->GetGoodPlaceForNewNode();
	}

	const FGeometry& CachedGeometry = Editor->GetCachedGeometry();
	const FVector2D CenterLocation = Editor->GetGraphPanel()->PanelCoordToGraphCoord(CachedGeometry.GetLocalSize() / 2.f);

	FVector2D Location = CenterLocation;

	const FVector2D StaticSize = FVector2D(100.f, 50.f);
	FBox2D CurrentBox(Location, Location + StaticSize);

	const FBox2D ViewportBox(
		Editor->GetGraphPanel()->PanelCoordToGraphCoord(FVector2D::ZeroVector),
		Editor->GetGraphPanel()->PanelCoordToGraphCoord(CachedGeometry.GetLocalSize()));

	if (!ActiveGraph)
	{
		return Location;
	}

	int32 Iterations = 0;
	bool bFoundLocation = false;
	while (!bFoundLocation)
	{
		bFoundLocation = true;
		for (int32 Index = 0; Index < ActiveGraph->Nodes.Num(); Index++)
		{
			const UEdGraphNode* CurrentNode = ActiveGraph->Nodes[Index];
			const FVector2D NodePosition(CurrentNode->NodePosX, CurrentNode->NodePosY);
			FBox2D NodeBox(NodePosition, NodePosition);

			if (const TSharedPtr<SGraphNode> Widget = Editor->GetGraphPanel()->GetNodeWidgetFromGuid(CurrentNode->NodeGuid))
			{
				if (Widget->GetCachedGeometry().GetLocalSize() == FVector2D::ZeroVector)
				{
					continue;
				}

				NodeBox.Max += Widget->GetCachedGeometry().GetAbsoluteSize() / Editor->GetGraphPanel()->GetZoomAmount();
			}
			else
			{
				NodeBox.Max += StaticSize;
			}

			if (CurrentBox.Intersect(NodeBox))
			{
				Location.Y = NodeBox.Max.Y + 30.f;

				CurrentBox.Min = Location;
				CurrentBox.Max = Location + StaticSize;

				bFoundLocation = false;
				break;
			}
		}

		if (!CurrentBox.Intersect(ViewportBox))
		{
			Iterations++;
			if (Iterations == 1)
			{
				Location = CenterLocation + FVector2D(200.f, 0.f);
			}
			else if (Iterations == 2)
			{
				Location = CenterLocation - FVector2D(200.f, 0.f);
			}
			else
			{
				Location = CenterLocation;
				break;
			}
			
			CurrentBox.Min = Location;
			CurrentBox.Max = Location + StaticSize;

			bFoundLocation = false;
		}
	}

	return Location;
}

void FVoxelMetaGraphEditorToolkit::SelectParameter(const FGuid ParameterId, bool bForceRefresh)
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	UVoxelMetaGraph* MetaGraph = &GetAssetAs<UVoxelMetaGraph>();
	if (GetDetailsView()->GetSelectedObjects().Num() == 0 ||
		GetDetailsView()->GetSelectedObjects()[0] != MetaGraph)
	{
		GetDetailsView()->SetGenericLayoutDetailsDelegate(nullptr);
		GetDetailsView()->SetObject(MetaGraph);
	}

	if (ParameterId == TargetParameterId)
	{
		if (bForceRefresh)
		{
			GetDetailsView()->ForceRefresh();
		}
		return;
	}

	TargetParameterId = ParameterId;
	GetGraphMembers()->SelectByParameterId(ParameterId);

	if (TargetParameterId == FGuid())
	{
		GetDetailsView()->SetGenericLayoutDetailsDelegate(nullptr);
		GetDetailsView()->ForceRefresh();
		return;
	}

	GetDetailsView()->SetGenericLayoutDetailsDelegate(MakeLambdaDelegate([this, ParameterId]() -> TSharedRef<IDetailCustomization>
	{
		return MakeShared<FParameterObjectCustomization>(SharedThis(this), ParameterId);
	}));
	GetDetailsView()->ForceRefresh();
}