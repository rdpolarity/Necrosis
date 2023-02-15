// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelGraphEditorToolkit.h"
#include "VoxelEdGraph.h"
#include "VoxelGraphNode.h"
#include "VoxelGraphSchema.h"
#include "VoxelGraphSchemaAction.h"
#include "SVoxelGraphEditorActionMenu.h"

#include "SNodePanel.h"
#include "GraphEditor.h"
#include "GraphEditorActions.h"
#include "EdGraphSchema_K2.h"
#include "EdGraphUtilities.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Framework/Commands/GenericCommands.h"

int32 GVoxelGraphDelayOnGraphChangedScopeCount = 0;
TSet<TWeakPtr<FVoxelGraphEditorToolkit>> GVoxelGraphDelayOnGraphChangedToolkits;

FVoxelGraphDelayOnGraphChangedScope::FVoxelGraphDelayOnGraphChangedScope()
{
	ensure(GVoxelGraphDelayOnGraphChangedScopeCount >= 0);
	GVoxelGraphDelayOnGraphChangedScopeCount++;
}

FVoxelGraphDelayOnGraphChangedScope::~FVoxelGraphDelayOnGraphChangedScope()
{
	GVoxelGraphDelayOnGraphChangedScopeCount--;
	ensure(GVoxelGraphDelayOnGraphChangedScopeCount >= 0);

	if (GVoxelGraphDelayOnGraphChangedScopeCount != 0)
	{
		return;
	}

	const TSet<TWeakPtr<FVoxelGraphEditorToolkit>> Toolkits = MoveTemp(GVoxelGraphDelayOnGraphChangedToolkits);
	for (const TWeakPtr<FVoxelGraphEditorToolkit>& WeakToolkit : Toolkits)
	{
		const TSharedPtr<FVoxelGraphEditorToolkit> Toolkit = WeakToolkit.Pin();
		if (!ensure(Toolkit))
		{
			continue;
		}

		Toolkit->OnGraphChanged();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphEditorToolkit::OnGraphChangedImpl()
{
	VOXEL_FUNCTION_COUNTER();
	ensure(GVoxelGraphDelayOnGraphChangedScopeCount == 0);

	for (const TSharedPtr<SGraphEditor>& GraphEditor : GetGraphEditors())
	{
		UEdGraph* Graph = GraphEditor->GetCurrentGraph();
		if (!ensure(Graph))
		{
			continue;
		}

		// Check pins
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin->ParentPin)
				{
					ensure(Pin->ParentPin->SubPins.Contains(Pin));
				}

				for (const UEdGraphPin* SubPin : Pin->SubPins)
				{
					ensure(SubPin->ParentPin == Pin);
				}
			}
		}
	}
}

void FVoxelGraphEditorToolkit::OnGraphChanged()
{
	if (bDisableOnGraphChanged ||
		!ensure(!bOnGraphChangedCalled) ||
		// Happens when assets are teared down
		IsGarbageCollecting())
	{
		return;
	}

	TGuardValue<bool> Guard(bOnGraphChangedCalled, true);

	if (GVoxelGraphDelayOnGraphChangedScopeCount > 0)
	{
		GVoxelGraphDelayOnGraphChangedToolkits.Add(SharedThis(this));
		return;
	}

	OnGraphChangedImpl();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SGraphEditor> FVoxelGraphEditorToolkit::CreateGraphEditor(UEdGraph* Graph)
{
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = VOXEL_LOCTEXT("VOXEL");

	SGraphEditor::FGraphEditorEvents Events;
	Events.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FVoxelGraphEditorToolkit::OnSelectedNodesChanged);
	Events.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &FVoxelGraphEditorToolkit::OnNodeTitleCommitted);
	Events.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FVoxelGraphEditorToolkit::OnNodeDoubleClicked);
	Events.OnSpawnNodeByShortcut = MakeWeakPtrDelegate(this, [=](FInputChord Chord, const FVector2D& Position)
	{
		OnSpawnGraphNodeByShortcut(Chord, Position);
		return FReply::Handled();
	});
	Events.OnCreateActionMenu = SGraphEditor::FOnCreateActionMenu::CreateSP(this, &FVoxelGraphEditorToolkit::OnCreateGraphActionMenu);

	return SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.IsEditable(true)
		.Appearance(AppearanceInfo)
		.GraphToEdit(Graph)
		.GraphEvents(Events)
		.AutoExpandActionMenu(false)
		.ShowGraphStateOverlay(false);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphEditorToolkit::BindToolkitCommands()
{
	FGraphEditorCommands::Register();

	ToolkitCommands->MapAction(
		FGenericCommands::Get().Undo,
		MakeLambdaDelegate([]
		{
			GEditor->UndoTransaction();
		}));

	ToolkitCommands->MapAction(
		FGenericCommands::Get().Redo,
		MakeLambdaDelegate([]
		{
			GEditor->RedoTransaction();
		}));

	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().CreateComment,
		FExecuteAction::CreateSP(this, &FVoxelGraphEditorToolkit::CreateComment)
	);

	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().SplitStructPin,
		FExecuteAction::CreateSP(this, &FVoxelGraphEditorToolkit::OnSplitPin)
	);

	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().RecombineStructPin,
		FExecuteAction::CreateSP(this, &FVoxelGraphEditorToolkit::OnRecombinePin)
	);

	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().ResetPinToDefaultValue,
		FExecuteAction::CreateSP(this, &FVoxelGraphEditorToolkit::OnResetPinToDefaultValue),
		FCanExecuteAction::CreateSP(this, &FVoxelGraphEditorToolkit::CanResetPinToDefaultValue)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().SelectAll,
		MakeWeakPtrDelegate(this, [=]
		{
			if (const TSharedPtr<SGraphEditor> ActiveGraphEditor = GetActiveGraphEditor())
			{
				ActiveGraphEditor->SelectAllNodes();
			}
		}));

	GraphEditorCommands->MapAction(FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &FVoxelGraphEditorToolkit::DeleteSelectedNodes),
		FCanExecuteAction::CreateSP(this, &FVoxelGraphEditorToolkit::CanDeleteNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Copy,
		FExecuteAction::CreateSP(this, &FVoxelGraphEditorToolkit::CopySelectedNodes),
		FCanExecuteAction::CreateSP(this, &FVoxelGraphEditorToolkit::CanCopyNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Cut,
		FExecuteAction::CreateSP(this, &FVoxelGraphEditorToolkit::CutSelectedNodes),
		FCanExecuteAction::CreateSP(this, &FVoxelGraphEditorToolkit::CanCutNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Paste,
		FExecuteAction::CreateSP(this, &FVoxelGraphEditorToolkit::PasteNodes),
		FCanExecuteAction::CreateSP(this, &FVoxelGraphEditorToolkit::CanPasteNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Duplicate,
		FExecuteAction::CreateSP(this, &FVoxelGraphEditorToolkit::DuplicateNodes),
		FCanExecuteAction::CreateSP(this, &FVoxelGraphEditorToolkit::CanDuplicateNodes)
	);

	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().AlignNodesTop,
		MakeWeakPtrDelegate(this, [=]
		{
			if (const TSharedPtr<SGraphEditor> ActiveGraphEditor = GetActiveGraphEditor())
			{
				ActiveGraphEditor->OnAlignTop();
			}
		}));

	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().AlignNodesMiddle,
		MakeWeakPtrDelegate(this, [=]
		{
			if (const TSharedPtr<SGraphEditor> ActiveGraphEditor = GetActiveGraphEditor())
			{
				ActiveGraphEditor->OnAlignMiddle();
			}
		}));

	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().AlignNodesBottom,
		MakeWeakPtrDelegate(this, [=]
		{
			if (const TSharedPtr<SGraphEditor> ActiveGraphEditor = GetActiveGraphEditor())
			{
				ActiveGraphEditor->OnAlignBottom();
			}
		}));

	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().AlignNodesLeft,
		MakeWeakPtrDelegate(this, [=]
		{
			if (const TSharedPtr<SGraphEditor> ActiveGraphEditor = GetActiveGraphEditor())
			{
				ActiveGraphEditor->OnAlignLeft();
			}
		}));

	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().AlignNodesCenter,
		MakeWeakPtrDelegate(this, [=]
		{
			if (const TSharedPtr<SGraphEditor> ActiveGraphEditor = GetActiveGraphEditor())
			{
				ActiveGraphEditor->OnAlignCenter();
			}
		}));

	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().AlignNodesRight,
		MakeWeakPtrDelegate(this, [=]
		{
			if (const TSharedPtr<SGraphEditor> ActiveGraphEditor = GetActiveGraphEditor())
			{
				ActiveGraphEditor->OnAlignRight();
			}
		}));

	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().StraightenConnections,
		MakeWeakPtrDelegate(this, [=]
		{
			if (const TSharedPtr<SGraphEditor> ActiveGraphEditor = GetActiveGraphEditor())
			{
				ActiveGraphEditor->OnStraightenConnections();
			}
		}));

	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().DistributeNodesHorizontally,
		MakeWeakPtrDelegate(this, [=]
		{
			if (const TSharedPtr<SGraphEditor> ActiveGraphEditor = GetActiveGraphEditor())
			{
				ActiveGraphEditor->OnDistributeNodesH();
			}
		}));

	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().DistributeNodesVertically,
		MakeWeakPtrDelegate(this, [=]
		{
			if (const TSharedPtr<SGraphEditor> ActiveGraphEditor = GetActiveGraphEditor())
			{
				ActiveGraphEditor->OnDistributeNodesV();
			}
		}));
}

void FVoxelGraphEditorToolkit::CreateInternalWidgets()
{
	FVoxelBaseEditorToolkit::CreateInternalWidgets();

	ensure(!GraphEditorCommands);
	GraphEditorCommands = MakeShared<FUICommandList>();
}

void FVoxelGraphEditorToolkit::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	FVoxelBaseEditorToolkit::NotifyPostChange(PropertyChangedEvent, PropertyThatChanged);

	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive)
	{
		return;
	}

	FVoxelGraphDelayOnGraphChangedScope DelayScope;
	FixupGraphProperties();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphEditorToolkit::PostUndo(bool bSuccess)
{
	// Clear selection, to avoid holding refs to nodes that go away
	for (const TSharedPtr<SGraphEditor>& GraphEditor : GetGraphEditors())
	{
		GraphEditor->ClearSelectionSet();
		GraphEditor->NotifyGraphChanged();
	}

	OnGraphChanged();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphEditorToolkit::OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged) const
{
	if (!ensure(NodeBeingChanged))
	{
		return;
	}
	
	const FVoxelTransaction Transaction(NodeBeingChanged, "Rename Node");
	NodeBeingChanged->OnRenameNode(NewText.ToString());
}

void FVoxelGraphEditorToolkit::OnNodeDoubleClicked(UEdGraphNode* Node)
{
	if (Node->CanJumpToDefinition())
	{
		Node->JumpToDefinition();
	}
}

FActionMenuContent FVoxelGraphEditorToolkit::OnCreateGraphActionMenu(UEdGraph* InGraph, const FVector2D& InNodePosition, const TArray<UEdGraphPin*>& InDraggedPins, bool bAutoExpand, SGraphEditor::FActionMenuClosed InOnMenuClosed) const
{
	const TSharedRef<SVoxelGraphEditorActionMenu> Menu =	
		SNew(SVoxelGraphEditorActionMenu)
		.GraphObj(InGraph)
		.NewNodePosition(InNodePosition)
		.DraggedFromPins(InDraggedPins)
		.AutoExpandActionMenu(bAutoExpand)
		.OnClosedCallback(InOnMenuClosed);

	return FActionMenuContent(Menu, Menu->GetFilterTextBox());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSet<UEdGraphNode*> FVoxelGraphEditorToolkit::GetSelectedNodes() const
{
	const TSharedPtr<SGraphEditor> GraphEditor = GetActiveGraphEditor();
	if (!ensure(GraphEditor))
	{
		return {};
	}

	TSet<UEdGraphNode*> SelectedNodes;
	for (UObject* Object : GraphEditor->GetSelectedNodes())
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(Object);
		if (!ensure(Node))
		{
			continue;
		}

		SelectedNodes.Add(Node);
	}
	return SelectedNodes;
}

void FVoxelGraphEditorToolkit::CreateComment() const
{
	const TSharedPtr<SGraphEditor> GraphEditor = GetActiveGraphEditor();
	if (!ensure(GraphEditor))
	{
		return;
	}

	UEdGraph* Graph = GraphEditor->GetCurrentGraph();
	if (!ensure(Graph))
	{
		return;
	}

	FVoxelGraphSchemaAction_NewComment CommentAction;
	CommentAction.PerformAction(Graph, nullptr, GraphEditor->GetPasteLocation());
}

void FVoxelGraphEditorToolkit::DeleteNodes(const TArray<UEdGraphNode*>& NodesRef)
{
	if (NodesRef.Num() == 0)
	{
		return;
	}

	// Ensure we're not going to edit the array while iterating it, eg if it's directly the graph node array
	const TArray<UEdGraphNode*> Nodes = NodesRef;
	
	const TSharedPtr<SGraphEditor> GraphEditor = GetActiveGraphEditor();
	if (!ensure(GraphEditor))
	{
		return;
	}

	UEdGraph* Graph = GraphEditor->GetCurrentGraph();
	if (!ensure(Graph))
	{
		return;
	}
	
	const FVoxelTransaction Transaction(Graph, "Delete nodes");

	for (UEdGraphNode* Node : Nodes)
	{
		if (!ensure(Node) || !Node->CanUserDeleteNode())
		{
			continue;
		}

		GraphEditor->SetNodeSelection(Node, false);

		Node->Modify();
		Graph->GetSchema()->BreakNodeLinks(*Node);
		Node->DestroyNode();
	}
}

void FVoxelGraphEditorToolkit::OnSplitPin()
{
	const TSharedPtr<SGraphEditor> GraphEditor = GetActiveGraphEditor();
	if (!ensure(GraphEditor))
	{
		return;
	}

	UEdGraphPin* Pin = GraphEditor->GetGraphPinForMenu();
	if (!ensure(Pin))
	{
		return;
	}

	UVoxelGraphNode* Node = Cast<UVoxelGraphNode>(Pin->GetOwningNode());
	if (!ensure(Node) ||
		!ensure(Node->CanSplitPin(*Pin)))
	{
		return;
	}
	
	const FVoxelTransaction Transaction(Node, "Split pin");
	Node->SplitPin(*Pin);
}

void FVoxelGraphEditorToolkit::OnRecombinePin()
{
	const TSharedPtr<SGraphEditor> GraphEditor = GetActiveGraphEditor();
	if (!ensure(GraphEditor))
	{
		return;
	}

	UEdGraphPin* Pin = GraphEditor->GetGraphPinForMenu();
	if (!ensure(Pin))
	{
		return;
	}

	UVoxelGraphNode* Node = Cast<UVoxelGraphNode>(Pin->GetOwningNode());
	if (!ensure(Node) ||
		!ensure(Node->CanRecombinePin(*Pin)))
	{
		return;
	}
	
	const FVoxelTransaction Transaction(Node, "Recombine pin");
	Node->RecombinePin(*Pin);
}

void FVoxelGraphEditorToolkit::OnResetPinToDefaultValue()
{
	const TSharedPtr<SGraphEditor> GraphEditor = GetActiveGraphEditor();
	if (!ensure(GraphEditor))
	{
		return;
	}

	const UEdGraph* Graph = GraphEditor->GetCurrentGraph();
	if (!ensure(Graph))
	{
		return;
	}

	UEdGraphPin* Pin = GraphEditor->GetGraphPinForMenu();
	if (!ensure(Pin))
	{
		return;
	}

	Graph->GetSchema()->ResetPinToAutogeneratedDefaultValue(Pin);
}

bool FVoxelGraphEditorToolkit::CanResetPinToDefaultValue() const
{
	const TSharedPtr<SGraphEditor> GraphEditor = GetActiveGraphEditor();
	if (!ensure(GraphEditor))
	{
		return false;
	}

	const UEdGraphPin* Pin = GraphEditor->GetGraphPinForMenu();
	return Pin && !Pin->DoesDefaultValueMatchAutogenerated();
}

void FVoxelGraphEditorToolkit::DeleteSelectedNodes()
{
	DeleteNodes(GetSelectedNodes().Array());
}

bool FVoxelGraphEditorToolkit::CanDeleteNodes() const
{
	const TSharedPtr<SGraphEditor> GraphEditor = GetActiveGraphEditor();
	if (!GraphEditor)
	{
		return false;
	}

	if (GraphEditor->GetNumberOfSelectedNodes() == 0)
	{
		return false;
	}

	for (const UEdGraphNode* Node : GetSelectedNodes())
	{
		if (!Node->CanUserDeleteNode())
		{
			return false;
		}
	}

	return true;
}

void FVoxelGraphEditorToolkit::CutSelectedNodes()
{
	CopySelectedNodes();
	
	TArray<UEdGraphNode*> NodesToDelete;
	for (UEdGraphNode* Node : GetSelectedNodes())
	{
		if (Node->CanDuplicateNode())
		{
			NodesToDelete.Add(Node);
		}
	}

	DeleteNodes(NodesToDelete);
}

bool FVoxelGraphEditorToolkit::CanCutNodes() const
{
	return CanCopyNodes() && CanDeleteNodes();
}

void FVoxelGraphEditorToolkit::CopySelectedNodes()
{
	TSet<UEdGraphNode*> NodesToCopy;
	{
		for (UEdGraphNode* Node : GetSelectedNodes())
		{
			if (!Node->CanDuplicateNode())
			{
				continue;
			}

			Node->PrepareForCopying();
			NodesToCopy.Add(Node);
		}
	}

	FString ExportedText;
	FEdGraphUtilities::ExportNodesToText(reinterpret_cast<TSet<UObject*>&>(NodesToCopy), ExportedText);
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);
}

bool FVoxelGraphEditorToolkit::CanCopyNodes() const
{
	for (const UEdGraphNode* Node : GetSelectedNodes())
	{
		if (Node->CanDuplicateNode())
		{
			return true;
		}
	}
	return false;
}

void FVoxelGraphEditorToolkit::PasteNodes()
{
	const TSharedPtr<SGraphEditor> GraphEditor = GetActiveGraphEditor();
	if (!ensure(GraphEditor))
	{
		return;
	}

	PasteNodesHere(GraphEditor->GetPasteLocation());
}

void FVoxelGraphEditorToolkit::PasteNodesHere(const FVector2D& Location)
{
	const TSharedPtr<SGraphEditor> GraphEditor = GetActiveGraphEditor();
	if (!ensure(GraphEditor))
	{
		return;
	}

	UEdGraph* Graph = GraphEditor->GetCurrentGraph();
	if (!ensure(Graph))
	{
		return;
	}
	
	const FVoxelTransaction Transaction(Graph, "Paste nodes");
	FVoxelGraphDelayOnGraphChangedScope OnGraphChangedScope;

	// Clear the selection set (newly pasted stuff will be selected)
	GraphEditor->ClearSelectionSet();

	// Grab the text to paste from the clipboard.
	FString TextToImport;
	FPlatformApplicationMisc::ClipboardPaste(TextToImport);

	// Import the nodes
	TSet<UEdGraphNode*> PastedNodes;
	FEdGraphUtilities::ImportNodesFromText(Graph, TextToImport, PastedNodes);

	TSet<UEdGraphNode*> CopyPastedNodes = PastedNodes;
	for (UEdGraphNode* Node : CopyPastedNodes)
	{
		if (UVoxelGraphNode* VoxelNode = Cast<UVoxelGraphNode>(Node))
		{
			if (!VoxelNode->CanPasteVoxelNode(PastedNodes))
			{
				Node->DestroyNode();
				PastedNodes.Remove(Node);
			}
		}
	}

	// Average position of nodes so we can move them while still maintaining relative distances to each other
	FVector2D AvgNodePosition(0.0f, 0.0f);

	for (const UEdGraphNode* Node : PastedNodes)
	{
		AvgNodePosition.X += Node->NodePosX;
		AvgNodePosition.Y += Node->NodePosY;
	}

	if (PastedNodes.Num() > 0)
	{
		AvgNodePosition.X /= PastedNodes.Num();
		AvgNodePosition.Y /= PastedNodes.Num();
	}

	for (UEdGraphNode* Node : PastedNodes)
	{
		// Select the newly pasted stuff
		GraphEditor->SetNodeSelection(Node, true);

		Node->NodePosX = (Node->NodePosX - AvgNodePosition.X) + Location.X;
		Node->NodePosY = (Node->NodePosY - AvgNodePosition.Y) + Location.Y;

		Node->SnapToGrid(SNodePanel::GetSnapGridSize());

		// Give new node a different Guid from the old one
		Node->CreateNewGuid();
	}

	// Post paste for local variables
	for (UEdGraphNode* Node : PastedNodes)
	{
		if (UVoxelGraphNode* VoxelNode = Cast<UVoxelGraphNode>(Node))
		{
			VoxelNode->PostPasteVoxelNode(PastedNodes);
		}
	}

	// Update UI
	GraphEditor->NotifyGraphChanged();
}

bool FVoxelGraphEditorToolkit::CanPasteNodes() const
{
	const TSharedPtr<SGraphEditor> GraphEditor = GetActiveGraphEditor();
	if (!ensure(GraphEditor))
	{
		return false;
	}

	const UEdGraph* Graph = GraphEditor->GetCurrentGraph();
	if (!ensure(Graph))
	{
		return false;
	}

	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	return FEdGraphUtilities::CanImportNodesFromText(Graph, ClipboardContent);
}

void FVoxelGraphEditorToolkit::DuplicateNodes()
{
	// Copy and paste current selection
	CopySelectedNodes();
	PasteNodes();
}

bool FVoxelGraphEditorToolkit::CanDuplicateNodes() const
{
	return CanCopyNodes();
}