// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "SVoxelMetaGraphMembers.h"

#include "GraphEditAction.h"
#include "VoxelMetaGraph.h"
#include "VoxelMetaGraphSchema.h"
#include "VoxelMetaGraphToolkit.h"
#include "VoxelGraphEditorToolkit.h"
#include "SVoxelMetaGraphPaletteItem.h"
#include "Nodes/VoxelMetaGraphLocalVariableNode.h"
#include "Nodes/VoxelMetaGraphMacroParameterNode.h"
#include "SchemaActions/VoxelMetaGraphMemberSchemaAction.h"
#include "DragDropActions/VoxelMetaGraphInputDragDropAction.h"
#include "Customizations/VoxelMetaGraphParameterCustomization.h"
#include "DragDropActions/VoxelMetaGraphCategoryDragDropAction.h"
#include "DragDropActions/VoxelMetaGraphLocalVariableDragDropAction.h"

#include "SNodePanel.h"
#include "GraphEditor.h"
#include "Dialogs/Dialogs.h"
#include "SGraphActionMenu.h"
#include "EdGraphUtilities.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Framework/Commands/GenericCommands.h"
#include "GraphEditor/Private/GraphActionNode.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

static const TArray<FText> NodeSectionNames
{
	VOXEL_LOCTEXT("INVALID"),
	VOXEL_LOCTEXT("Parameters"),
	VOXEL_LOCTEXT("Inputs"),
	VOXEL_LOCTEXT("Outputs"),
	VOXEL_LOCTEXT("Local Variables"),
};

static const TArray<FString> CopyParameterNames
{
	"INVALID",
	"MetaGraphParameter",
	"MetaGraphMacroInput",
	"MetaGraphMacroOutput",
	"MetaGraphLocalVariable",
};

struct FActionsSortHelper
{
	void AddCategoriesSortList(int32 SectionId, const TArray<FString>& Categories)
	{
		SortedCategories.Add(SectionId, Categories);
		SortedCategories[SectionId].Add("");
	}

	void AddAction(const TSharedPtr<FEdGraphSchemaAction>& NewAction, const FString& Category)
	{
		if (TMap<FString, TArray<TSharedPtr<FEdGraphSchemaAction>>>* SectionActionsPtr = Actions.Find(NewAction->GetSectionID()))
		{
			if (TArray<TSharedPtr<FEdGraphSchemaAction>>* ActionsPtr = SectionActionsPtr->Find(Category))
			{
				ActionsPtr->Add(NewAction);
				return;
			}
			
			SectionActionsPtr->Add(Category, {NewAction});
			return;
		}

		TMap<FString, TArray<TSharedPtr<FEdGraphSchemaAction>>> SectionActions;
		SectionActions.Add(Category, {NewAction});
		
		Actions.Add(NewAction->GetSectionID(), SectionActions);
	}

	void GetAllActions(FGraphActionListBuilderBase& OutAllActions)
	{
		for (const auto& It : SortedCategories)
		{
			const auto& ActionsMapPtr = Actions.Find(It.Key);
			if (!ActionsMapPtr)
			{
				continue;
			}

			for (const FString& Category : It.Value)
			{
				if (TArray<TSharedPtr<FEdGraphSchemaAction>>* ActionsPtr = ActionsMapPtr->Find(Category))
				{
					for (const TSharedPtr<FEdGraphSchemaAction>& Action : *ActionsPtr)
					{
						OutAllActions.AddAction(Action);
					}
				}
			}
		}
	}

private:
	TMap<int32, TArray<FString>> SortedCategories;
	TMap<int32, TMap<FString, TArray<TSharedPtr<FEdGraphSchemaAction>>>> Actions;
};

void SMembers::Construct(const FArguments& Args)
{
	MetaGraph = Args._MetaGraph;
	WeakToolkit = Args._Toolkit;
	check(MetaGraph.IsValid() &&
		WeakToolkit.IsValid());
	
	CommandList = MakeShared<FUICommandList>();
	
	CommandList->MapAction( FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &SMembers::OnDeleteEntry));

	CommandList->MapAction( FGenericCommands::Get().Duplicate,
		FExecuteAction::CreateSP(this, &SMembers::OnDuplicateAction));
	
	CommandList->MapAction(FGenericCommands::Get().Rename,
		FExecuteAction::CreateSP(this, &SMembers::OnRequestRenameOnActionNode),
		FCanExecuteAction::CreateSP(this, &SMembers::CanRequestRenameOnActionNode));

	CommandList->MapAction(FGenericCommands::Get().Copy,
		FExecuteAction::CreateSP(this, &SMembers::OnCopy));
	
	CommandList->MapAction(FGenericCommands::Get().Cut,
		FExecuteAction::CreateSP(this, &SMembers::OnCut));
	
	CommandList->MapAction(FGenericCommands::Get().Paste,
		FExecuteAction::CreateSP(this, &SMembers::OnPaste),
		FCanExecuteAction::CreateSP(this, &SMembers::CanPaste));
	
	SAssignNew(FilterBox, SSearchBox)
	.OnTextChanged(this, &SMembers::OnFilterTextChanged);
	
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
			.Padding(4.0f)
			.BorderImage(FEditorAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("MyBlueprintPanel")))
			[
				FilterBox.ToSharedRef()
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SAssignNew(MembersMenu, SGraphActionMenu, false)
			.AlphaSortItems(false)
			.OnCollectStaticSections(this, &SMembers::CollectStaticSections)
			.OnGetSectionTitle(this, &SMembers::OnGetSectionTitle)
			.OnGetSectionWidget(this, &SMembers::OnGetMenuSectionWidget)
			.OnCollectAllActions(this, &SMembers::CollectAllActions)
			.OnCreateWidgetForAction(this, &SMembers::OnCreateWidgetForAction)
			.OnCanRenameSelectedAction_Lambda([](TWeakPtr<FGraphActionNode>) { return true; })
			.OnActionSelected(this, &SMembers::OnActionSelected)
			.OnActionMatchesName(this, &SMembers::HandleActionMatchesName)
			.OnActionDragged(this, &SMembers::OnActionDragged)
			.OnCategoryTextCommitted(this, &SMembers::OnCategoryNameCommitted)
			.OnCategoryDragged(this, &SMembers::OnCategoryDragged)
			.OnGetFilterText(this, &SMembers::GetFilterText)
			.OnContextMenuOpening(this, &SMembers::OnContextMenuOpening)
			.OnActionDoubleClicked(this, &SMembers::OnMemberActionDoubleClicked)
			.UseSectionStyling(true)
		]
	];

	MetaGraph->OnParametersChanged.AddSP(this, &SMembers::OnObjectPropertyChanged);

	if (const TSharedPtr<FVoxelMetaGraphEditorToolkit> Toolkit = WeakToolkit.Pin())
	{
		Toolkit->GetActiveGraph()->AddOnGraphChangedHandler(
			FOnGraphChanged::FDelegate::CreateSP(this, &SMembers::OnGraphChanged)
		);
	}
}

void SMembers::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	
	if (bNeedsRefresh)
	{
		Refresh();
	}
}

FReply SMembers::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (CommandList.IsValid() &&
		CommandList->ProcessCommandBindings(InKeyEvent))
	{
		return FReply::Handled();
	}
	
	return FReply::Unhandled();
}

void SMembers::SelectAndRequestRename(const FGuid& ItemId, EMembersNodeSection Section)
{
	FName TargetName;

	if (Section == EMembersNodeSection::Parameters ||
		Section == EMembersNodeSection::MacroInputs ||
		Section == EMembersNodeSection::MacroOutputs ||
		Section == EMembersNodeSection::LocalVariables)
	{
		const FVoxelMetaGraphParameter* ParameterPtr = MetaGraph->FindParameterByGuid(ItemId);
		if (!ParameterPtr)
		{
			return;
		}

		TargetName = ParameterPtr->Name;
	}
	else
	{
		ensure(false);
		return;
	}

	MembersMenu->SelectItemByName(TargetName, ESelectInfo::Direct, GetSectionId(Section));
	OnRequestRenameOnActionNode();

	TArray<TSharedPtr<FEdGraphSchemaAction>> SelectedActions;
	MembersMenu->GetSelectedActions(SelectedActions);
	OnActionSelected(SelectedActions, ESelectInfo::OnMouseClick);
}

void SMembers::SelectByParameterId(const FGuid& ItemId) const
{
	const FVoxelMetaGraphParameter* ParameterPtr = MetaGraph->FindParameterByGuid(ItemId);
	if (!ParameterPtr)
	{
		MembersMenu->SelectItemByName("");
		return;
	}

	const FName TargetName = ParameterPtr->Name;

	const EMembersNodeSection Section = GetSection(ParameterPtr->ParameterType);
	if (Section != EMembersNodeSection::Parameters &&
		Section != EMembersNodeSection::MacroInputs &&
		Section != EMembersNodeSection::MacroOutputs &&
		Section != EMembersNodeSection::LocalVariables)
	{
		ensure(false);
		return;
	}

	MembersMenu->SelectItemByName(TargetName, ESelectInfo::Direct, GetSectionId(Section));
}

void SMembers::SelectByName(const FName& Name, ESelectInfo::Type SelectInfo, int32 InSectionID)
{
	Refresh();
	MembersMenu->SelectItemByName(Name, SelectInfo, InSectionID);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SMembers::PostUndo(bool bSuccess)
{
	bNeedsRefresh = true;
}

void SMembers::PostRedo(bool bSuccess)
{
	bNeedsRefresh = true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SMembers::OnGraphChanged(const FEdGraphEditAction& Action)
{
	bNeedsRefresh = true;
}

void SMembers::OnObjectPropertyChanged()
{
	bNeedsRefresh = true;
}

void SMembers::Refresh()
{
	bNeedsRefresh = false;
	
	bIsRefreshing = true;
	MembersMenu->RefreshAllActions(true);
	bIsRefreshing = false;
}

FString SMembers::GetPasteCategory() const
{
	TArray<TSharedPtr<FEdGraphSchemaAction> > SelectedActions;
	MembersMenu->GetSelectedActions(SelectedActions);
	
	if (SelectedActions.Num() == 0 &&
		MembersMenu.IsValid())
	{
		const FString CategoryName = MembersMenu->GetSelectedCategoryName();
		if (!CategoryName.IsEmpty())
		{
			return MembersMenu->GetSelectedCategoryName();
		}
	}
	
	return "";
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SMembers::CollectStaticSections(TArray<int32>& StaticSectionIDs)
{
	check(MetaGraph.IsValid());

	if (MetaGraph->bIsMacroGraph)
	{
		StaticSectionIDs.Add(GetSectionId(EMembersNodeSection::MacroInputs));
		StaticSectionIDs.Add(GetSectionId(EMembersNodeSection::MacroOutputs));
	}
	else
	{
		StaticSectionIDs.Add(GetSectionId(EMembersNodeSection::Parameters));
	}

	StaticSectionIDs.Add(GetSectionId(EMembersNodeSection::LocalVariables));
}

FText SMembers::OnGetSectionTitle(int32 InSectionID)
{
	if (!ensure(NodeSectionNames.IsValidIndex(InSectionID)))
	{
		return FText::GetEmpty();
	}

	return NodeSectionNames[InSectionID];
}

TSharedRef<SWidget> SMembers::OnGetMenuSectionWidget(TSharedRef<SWidget> RowWidget, int32 InSectionID)
{
	switch (GetSection(InSectionID))
	{
	default: check(false);
	case EMembersNodeSection::Parameters: return CreateAddButton(InSectionID, VOXEL_LOCTEXT("Parameter"), "AddNewParameter");
	case EMembersNodeSection::MacroInputs: return CreateAddButton(InSectionID, VOXEL_LOCTEXT("Input"), "AddNewInput");
	case EMembersNodeSection::MacroOutputs: return CreateAddButton(InSectionID, VOXEL_LOCTEXT("Output"), "AddNewOutput");
	case EMembersNodeSection::LocalVariables: return CreateAddButton(InSectionID, VOXEL_LOCTEXT("Local Variable"), "AddNewLocalVariable");
	}
}

void SMembers::CollectAllActions(FGraphActionListBuilderBase& OutAllActions)
{
	check(MetaGraph.IsValid());

	FActionsSortHelper SortList;
	if (MetaGraph->bIsMacroGraph)
	{
		SortList.AddCategoriesSortList(GetSectionId(EMembersNodeSection::MacroInputs), MetaGraph->GetCategories(EVoxelMetaGraphParameterType::MacroInput));
		SortList.AddCategoriesSortList(GetSectionId(EMembersNodeSection::MacroOutputs), MetaGraph->GetCategories(EVoxelMetaGraphParameterType::MacroOutput));
	}
	else
	{
		SortList.AddCategoriesSortList(GetSectionId(EMembersNodeSection::Parameters), MetaGraph->GetCategories(EVoxelMetaGraphParameterType::Parameter));
	}

	SortList.AddCategoriesSortList(GetSectionId(EMembersNodeSection::LocalVariables), MetaGraph->GetCategories(EVoxelMetaGraphParameterType::LocalVariable));

	for (const FVoxelMetaGraphParameter& Parameter : MetaGraph->Parameters)
	{
		TSharedPtr<FVoxelMetaGraphMemberSchemaAction> NewFuncAction = MakeShared<FVoxelMetaGraphMemberSchemaAction>(FText::FromString(Parameter.Category), FText::FromName(Parameter.Name), FText::FromString(Parameter.Description), 1, FText(), GetSectionId(Parameter.ParameterType));
		NewFuncAction->ParameterGuid = Parameter.Guid;
		NewFuncAction->MetaGraph = MetaGraph;
		NewFuncAction->WeakToolkit = WeakToolkit;
		
		SortList.AddAction(NewFuncAction, Parameter.Category);
	}

	SortList.GetAllActions(OutAllActions);
}

TSharedRef<SWidget> SMembers::OnCreateWidgetForAction(FCreateWidgetForActionData* const InCreateData)
{
	return
		SNew(SPaletteItem, InCreateData)
		.MembersWidget(SharedThis(this))
		.Toolkit(WeakToolkit);
}

void SMembers::OnActionSelected(const TArray<TSharedPtr<FEdGraphSchemaAction>>& InActions, ESelectInfo::Type InSelectionType)
{
	if (InSelectionType != ESelectInfo::OnMouseClick &&
		InSelectionType != ESelectInfo::OnKeyPress &&
		InSelectionType != ESelectInfo::OnNavigation &&
		!InActions.IsEmpty())
	{
		return;
	}

	if (InActions.IsEmpty())
	{
		if (!bIsRefreshing)
		{
			SelectParameterDetailsView(nullptr, !MembersMenu->GetSelectedCategoryName().IsEmpty());
		}
		return;
	}

	const TSharedPtr<FVoxelMetaGraphMemberSchemaAction> ParameterAction = StaticCastSharedPtr<FVoxelMetaGraphMemberSchemaAction>(InActions[0]);
	SelectParameterDetailsView(ParameterAction->GetParameter());
}

bool SMembers::HandleActionMatchesName(FEdGraphSchemaAction* InAction, const FName& InName)
{
	return FName(*InAction->GetMenuDescription().ToString()) == InName;
}

FReply SMembers::OnActionDragged(const TArray<TSharedPtr<FEdGraphSchemaAction>>& InActions, const FPointerEvent& MouseEvent)
{
	const TSharedPtr<FEdGraphSchemaAction> InAction = !InActions.IsEmpty() ? InActions[0] : nullptr;
	if (!InAction.IsValid())
	{
		return FReply::Unhandled();
	}

	const TSharedPtr<FVoxelMetaGraphMemberSchemaAction> ParameterAction = StaticCastSharedPtr<FVoxelMetaGraphMemberSchemaAction>(InActions[0]);

	const EMembersNodeSection Section = GetSection(ParameterAction->SectionID);
	if (Section == EMembersNodeSection::LocalVariables)
	{
		const TSharedRef<FLocalVariableDragDropAction> DragOperation = FLocalVariableDragDropAction::New(InAction, ParameterAction->ParameterGuid, MetaGraph);
		DragOperation->SetAltDrag(MouseEvent.IsAltDown());
		DragOperation->SetCtrlDrag(MouseEvent.IsLeftControlDown() || MouseEvent.IsRightControlDown());
		return FReply::Handled().BeginDragDrop(DragOperation);
	}
	else
	{
		return FReply::Handled().BeginDragDrop(FInputDragDropAction::New(InAction, ParameterAction->ParameterGuid, MetaGraph));
	}
}

void SMembers::OnMemberActionDoubleClicked(const TArray<TSharedPtr<FEdGraphSchemaAction>>& InActions)
{
	if (!MetaGraph.IsValid())
	{
		return;
	}

	const TSharedPtr<FEdGraphSchemaAction> InAction = !InActions.IsEmpty() ? InActions[0] : nullptr;
	if (!InAction)
	{
		return;
	}

	UClass* TargetClass;

	const TSharedPtr<FVoxelMetaGraphMemberSchemaAction> ParameterAction = StaticCastSharedPtr<FVoxelMetaGraphMemberSchemaAction>(InAction);
	const FGuid TargetGuid = ParameterAction->ParameterGuid;

	const EMembersNodeSection Section = GetSection(InAction->SectionID);
	if (Section == EMembersNodeSection::Parameters)
	{
		TargetClass = UVoxelMetaGraphParameterNode::StaticClass();
	}
	else if (
		Section == EMembersNodeSection::MacroInputs ||
		Section == EMembersNodeSection::MacroOutputs)
	{
		TargetClass = UVoxelMetaGraphMacroParameterNode::StaticClass();
	}
	else if (Section == EMembersNodeSection::LocalVariables)
	{
		TargetClass = UVoxelMetaGraphLocalVariableNode::StaticClass();
	}
	else
	{
		ensure(false);
		return;
	}

	const TSharedPtr<FVoxelMetaGraphEditorToolkit> PinnedToolkit = WeakToolkit.Pin();
	if (!PinnedToolkit)
	{
		return;
	}
	
	PinnedToolkit->GetActiveGraphEditor()->ClearSelectionSet();

	bool bFoundNodes = false;
	
	for (const TObjectPtr<UEdGraphNode>& Node : PinnedToolkit->GetActiveGraph()->Nodes)
	{
		if (!Node.IsA(TargetClass))
		{
			continue;
		}

		if (const UVoxelMetaGraphParameterNode* ParameterNode = Cast<UVoxelMetaGraphParameterNode>(Node))
		{
			if (ParameterNode->Guid != TargetGuid)
			{
				continue;
			}
		}
		else if (const UVoxelMetaGraphMacroParameterNode* MacroParameterNode = Cast<UVoxelMetaGraphMacroParameterNode>(Node))
		{
			if (MacroParameterNode->Guid != TargetGuid)
			{
				continue;
			}
		}
		else if (const UVoxelMetaGraphLocalVariableNode* LocalVariableNode = Cast<UVoxelMetaGraphLocalVariableNode>(Node))
		{
			if (LocalVariableNode->Guid != TargetGuid)
			{
				continue;
			}
		}

		PinnedToolkit->GetActiveGraphEditor()->SetNodeSelection(Node, true);
		bFoundNodes = true;
	}
	
	if (bFoundNodes)
	{
		PinnedToolkit->GetActiveGraphEditor()->ZoomToFit(true);
	}
}

void SMembers::OnCategoryNameCommitted(const FText& InNewText, ETextCommit::Type InTextCommit, TWeakPtr<FGraphActionNode> InAction)
{
	const FString CategoryName = InNewText.ToString().TrimStartAndEnd();
	
	TArray<TSharedPtr<FEdGraphSchemaAction>> Actions;
	MembersMenu->GetCategorySubActions(InAction, Actions);
	
	if (!Actions.IsEmpty())
	{
		{
			const FVoxelTransaction Transaction(MetaGraph, "Rename Category");

			for (int32 i = 0; i < Actions.Num(); ++i)
			{
				const TSharedPtr<FVoxelMetaGraphMemberSchemaAction> ParameterAction = StaticCastSharedPtr<FVoxelMetaGraphMemberSchemaAction>(Actions[i]);
				if (FVoxelMetaGraphParameter* Parameter = ParameterAction->GetParameter())
				{
					Parameter->Category = CategoryName;
				}
			}
		}

		Refresh();
		MembersMenu->SelectItemByName(FName(*CategoryName), ESelectInfo::OnMouseClick, InAction.Pin()->SectionID, true);
	}
}

FReply SMembers::OnCategoryDragged(const FText& InCategory, const FPointerEvent& MouseEvent)
{
	TArray<TSharedPtr<FEdGraphSchemaAction>> Actions;
	MembersMenu->GetSelectedCategorySubActions(Actions);
	
	if (Actions.IsEmpty())
	{
		return FReply::Handled();
	}

	const TSharedRef<FCategoryDragDropAction> DragDropAction = FCategoryDragDropAction::New(InCategory.ToString(), MetaGraph, GetSection(Actions[0]->SectionID));
	return FReply::Handled().BeginDragDrop(DragDropAction);
}

FText SMembers::GetFilterText() const
{
	return FilterBox->GetText();
}

TSharedPtr<SWidget> SMembers::OnContextMenuOpening()
{
	FMenuBuilder MenuBuilder(true, CommandList);
	
	TArray<TSharedPtr<FEdGraphSchemaAction> > SelectedActions;
	MembersMenu->GetSelectedActions(SelectedActions);
	
	if (!SelectedActions.IsEmpty())
	{
		MenuBuilder.BeginSection("BasicOperations");
		{
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Rename, NAME_None, VOXEL_LOCTEXT("Rename"), VOXEL_LOCTEXT("Renames this parameter from graph.") );
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Cut);
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Copy);
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Duplicate);
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Delete);
		}
		MenuBuilder.EndSection();
	}
	else
	{
		check(MetaGraph.IsValid());

		MenuBuilder.BeginSection("AddNewItem", VOXEL_LOCTEXT("Add New"));
		{
			if (MetaGraph->bIsMacroGraph)
			{
				MenuBuilder.AddMenuEntry(
					VOXEL_LOCTEXT("Add new input"),
					FText(),
					FSlateIcon(UE_501_SWITCH(FEditorStyle::GetStyleSetName(), FAppStyle::GetAppStyleSetName()), "BlueprintEditor.AddNewVariable"),
					FUIAction{
						FExecuteAction::CreateSP(this, &SMembers::OnAddNewParameter, EMembersNodeSection::MacroInputs)
					});
				MenuBuilder.AddMenuEntry(
					VOXEL_LOCTEXT("Add new output"),
					FText(),
					FSlateIcon(UE_501_SWITCH(FEditorStyle::GetStyleSetName(), FAppStyle::GetAppStyleSetName()), "BlueprintEditor.AddNewVariable"),
					FUIAction{
						FExecuteAction::CreateSP(this, &SMembers::OnAddNewParameter, EMembersNodeSection::MacroOutputs)
					});
			}
			else
			{
				MenuBuilder.AddMenuEntry(
					VOXEL_LOCTEXT("Add new parameter"),
					FText(),
					FSlateIcon(UE_501_SWITCH(FEditorStyle::GetStyleSetName(), FAppStyle::GetAppStyleSetName()), "BlueprintEditor.AddNewVariable"),
					FUIAction{
						FExecuteAction::CreateSP(this, &SMembers::OnAddNewParameter, EMembersNodeSection::Parameters)
					});
			}

			MenuBuilder.AddMenuEntry(
				VOXEL_LOCTEXT("Add new local variable"),
				FText(),
				FSlateIcon(UE_501_SWITCH(FEditorStyle::GetStyleSetName(), FAppStyle::GetAppStyleSetName()), "BlueprintEditor.AddNewVariable"),
				FUIAction{
					FExecuteAction::CreateSP(this, &SMembers::OnAddNewParameter, EMembersNodeSection::LocalVariables)
				});

			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Paste);
		}
		MenuBuilder.EndSection();
	}
	
	return MenuBuilder.MakeWidget();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SMembers::OnDeleteEntry()
{
	static const TArray<FString> VariableTypeName
	{
		"INVALID",
		"Parameter",
		"Input",
		"Output",
		"Local Variable",
	};

	TArray<TSharedPtr<FEdGraphSchemaAction> > SelectedActions;
	MembersMenu->GetSelectedActions(SelectedActions);

	if (SelectedActions.IsEmpty())
	{
		return;
	}

	const TSharedPtr<FVoxelMetaGraphMemberSchemaAction> ParameterAction = StaticCastSharedPtr<FVoxelMetaGraphMemberSchemaAction>(SelectedActions[0]);
	TArray<FVoxelMetaGraphParameter>& Parameters = ParameterAction->MetaGraph->Parameters;

	const int32 Index = Parameters.IndexOfByKey(ParameterAction->ParameterGuid);
	if (Index == -1)
	{
		return;
	}

	bool bDeleteNodes = false;
	const EMembersNodeSection Section = GetSection(ParameterAction->SectionID);
	if (Section != EMembersNodeSection::MacroInputs &&
		Section != EMembersNodeSection::MacroOutputs &&
		IsParameterUsed(ParameterAction->ParameterGuid))
	{
		{
			const FText Title = FText::FromString("Delete " + VariableTypeName[ParameterAction->SectionID]);
			const EAppReturnType::Type DialogResult = FMessageDialog::Open(
				EAppMsgType::YesNo,
				EAppReturnType::No,
				FText::FromString(VariableTypeName[ParameterAction->SectionID] + " " + ParameterAction->GetParameter()->Name.ToString() + " is in use, are you sure you want to delete it?"),
				&Title);
		
			if (DialogResult == EAppReturnType::No)
			{
				return;
			}
		}

		{
			const FText Title = FText::FromString("Delete " + VariableTypeName[ParameterAction->SectionID] + " usages");
			const EAppReturnType::Type DialogResult = FMessageDialog::Open(
				EAppMsgType::YesNoCancel,
				EAppReturnType::Cancel,
				FText::FromString("Do you want to delete its references from the graph?"),
				&Title);
		
			if (DialogResult == EAppReturnType::Cancel)
			{
				return;
			}

			bDeleteNodes = DialogResult == EAppReturnType::Yes;
		}
	}

	{
		const FVoxelTransaction Transaction(MetaGraph, "Delete parameter");
		Parameters.RemoveAt(Index);

		if (bDeleteNodes)
		{
			DeleteParameterNodes(ParameterAction->ParameterGuid);
		}
	}

	SelectParameterDetailsView(nullptr);
	Refresh();
}

void SMembers::OnDuplicateAction()
{
	TArray<TSharedPtr<FEdGraphSchemaAction>> SelectedActions;
	MembersMenu->GetSelectedActions(SelectedActions);

	if (SelectedActions.IsEmpty())
	{
		return;
	}

	const FGuid TargetGuid = FGuid::NewGuid();

	const TSharedPtr<FVoxelMetaGraphMemberSchemaAction> ParameterAction = StaticCastSharedPtr<FVoxelMetaGraphMemberSchemaAction>(SelectedActions[0]);
	TArray<FVoxelMetaGraphParameter>& Parameters = ParameterAction->MetaGraph->Parameters;

	const FVoxelMetaGraphParameter* Parameter = Parameters.FindByKey(ParameterAction->ParameterGuid);
	if (!Parameter)
	{
		return;
	}

	FVoxelMetaGraphParameter NewParameter = *Parameter;
	NewParameter.Guid = TargetGuid;
	
	const FVoxelTransaction Transaction(MetaGraph, "Duplicate parameter");
	Parameters.Add(NewParameter);

	Refresh();
	SelectAndRequestRename(TargetGuid, GetSection(SelectedActions[0]->SectionID));
}

void SMembers::OnRequestRenameOnActionNode()
{
	MembersMenu->OnRequestRenameOnActionNode();
}

bool SMembers::CanRequestRenameOnActionNode() const
{
	return MembersMenu->CanRequestRenameOnActionNode();
}

void SMembers::OnCopy()
{
	TArray<TSharedPtr<FEdGraphSchemaAction> > SelectedActions;
	MembersMenu->GetSelectedActions(SelectedActions);

	if (SelectedActions.IsEmpty())
	{
		return;
	}

	const TSharedPtr<FVoxelMetaGraphMemberSchemaAction> ParameterAction = StaticCastSharedPtr<FVoxelMetaGraphMemberSchemaAction>(SelectedActions[0]);

	const FVoxelMetaGraphParameter* Parameter = ParameterAction->GetParameter();
	if (!Parameter)
	{
		return;
	}

	FString OutputString;
	FVoxelMetaGraphParameter::StaticStruct()->ExportText(OutputString, Parameter, Parameter, nullptr, 0, nullptr);

	OutputString = CopyParameterNames[SelectedActions[0]->SectionID] + OutputString;

	FPlatformApplicationMisc::ClipboardCopy(OutputString.GetCharArray().GetData());
}

void SMembers::OnCut()
{
	OnCopy();
	OnDeleteEntry();
}

void SMembers::OnPaste()
{
	FString ClipboardText;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardText);

	EMembersNodeSection Section = EMembersNodeSection::MacroInputs;

	if (!GetPasteSectionType(ClipboardText, Section))
	{
		ensure(false);
		return;
	}

	FVoxelMetaGraphParameter NewParameter;
	FStringOutputDevice Errors;
	const TCHAR* Import = ClipboardText.GetCharArray().GetData() + CopyParameterNames[GetSectionId(Section)].Len();
	FVoxelMetaGraphParameter::StaticStruct()->ImportText(Import, &NewParameter, nullptr, 0, &Errors, FVoxelMetaGraphParameter::StaticStruct()->GetName());
	if (!Errors.IsEmpty())
	{
		return;
	}

	NewParameter.Guid = FGuid::NewGuid();
	NewParameter.Category = GetPasteCategory();

	const FVoxelTransaction Transaction(MetaGraph, "Paste parameter");
	MetaGraph->Parameters.Add(NewParameter);

	Refresh();
}

bool SMembers::CanPaste()
{
	FString ClipboardText;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardText);
	
	EMembersNodeSection Section = EMembersNodeSection::MacroInputs;

	if (!GetPasteSectionType(ClipboardText, Section))
	{
		return false;
	}

	FVoxelMetaGraphParameter Parameter;
	FStringOutputDevice Errors;
	const TCHAR* Import = ClipboardText.GetCharArray().GetData() + CopyParameterNames[GetSectionId(Section)].Len();
	FVoxelMetaGraphParameter::StaticStruct()->ImportText(Import, &Parameter, nullptr, 0, &Errors, FVoxelMetaGraphParameter::StaticStruct()->GetName());

	return Errors.IsEmpty();
}

bool SMembers::GetPasteSectionType(const FString& ImportText, EMembersNodeSection& Section) const
{
	if (ImportText.StartsWith(CopyParameterNames[GetSectionId(EMembersNodeSection::Parameters)], ESearchCase::CaseSensitive))
	{
		Section = EMembersNodeSection::Parameters;
		return true;
	}
	else if (ImportText.StartsWith(CopyParameterNames[GetSectionId(EMembersNodeSection::MacroInputs)], ESearchCase::CaseSensitive))
	{
		Section = EMembersNodeSection::MacroInputs;
		return true;
	}
	else if (ImportText.StartsWith(CopyParameterNames[GetSectionId(EMembersNodeSection::MacroOutputs)], ESearchCase::CaseSensitive))
	{
		Section = EMembersNodeSection::MacroOutputs;
		return true;
	}
	else if (ImportText.StartsWith(CopyParameterNames[GetSectionId(EMembersNodeSection::LocalVariables)], ESearchCase::CaseSensitive))
	{
		Section = EMembersNodeSection::LocalVariables;
		return true;
	}
	else
	{
		return false;
	}

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


TSharedRef<SWidget> SMembers::CreateAddButton(int32 InSectionID, FText AddNewText, FName MetaDataTag)
{
	return
		SNew(SButton)
		.ButtonStyle(FEditorAppStyle::Get(), "SimpleButton")
		.OnClicked_Lambda([this, InSectionID]
		{
			OnAddNewParameter(GetSection(InSectionID));
			return FReply::Handled();
		})
		.ContentPadding(FMargin(1, 0))
		.AddMetaData<FTagMetaData>(FTagMetaData(MetaDataTag))
		.ToolTipText(AddNewText)
		[
			SNew(SImage)
			.Image(FAppStyle::Get().GetBrush("Icons.PlusCircle"))
			.ColorAndOpacity(FSlateColor::UseForeground())
		];
}

void SMembers::OnFilterTextChanged(const FText& InFilterText) const
{
	MembersMenu->GenerateFilteredItems(false);
}

void SMembers::OnAddNewParameter(EMembersNodeSection Section)
{
	const TSharedPtr<FVoxelMetaGraphEditorToolkit> Toolkit = WeakToolkit.Pin();
	if (!ensure(Toolkit))
	{
		return;
	}

	FVoxelMetaGraphSchemaAction_NewParameter Action;
	Action.ParameterCategory = GetPasteCategory();

	switch (Section)
	{
	default: check(false);
	case EMembersNodeSection::Parameters: Action.ParameterType = EVoxelMetaGraphParameterType::Parameter; break;
	case EMembersNodeSection::MacroInputs: Action.ParameterType = EVoxelMetaGraphParameterType::MacroInput; break;
	case EMembersNodeSection::MacroOutputs: Action.ParameterType = EVoxelMetaGraphParameterType::MacroOutput; break;
	case EMembersNodeSection::LocalVariables: Action.ParameterType = EVoxelMetaGraphParameterType::LocalVariable; break;
	}

	Action.PerformAction(Toolkit->GetActiveGraph(), nullptr, Toolkit->FindLocationInGraph());
}

void SMembers::SelectParameterDetailsView(const FVoxelMetaGraphParameter* Parameter, bool bCategorySelected) const
{
	const TSharedPtr<FVoxelMetaGraphEditorToolkit> Toolkit = WeakToolkit.Pin();
	if (!Toolkit)
	{
		return;
	}

	if (Parameter == nullptr)
	{
		if (!bCategorySelected)
		{
			MembersMenu->SelectItemByName("");
		}

		Toolkit->SelectParameter(FGuid(), false);
		return;
	}

	Toolkit->SelectParameter(Parameter->Guid, false);
}

bool SMembers::IsParameterUsed(const FGuid& ParameterId) const
{
	const TSharedPtr<FVoxelMetaGraphEditorToolkit> Toolkit = WeakToolkit.Pin();
	if (!Toolkit)
	{
		return false;
	}

	const UEdGraph* ActiveGraph = Toolkit->GetActiveGraph();
	if (!ActiveGraph)
	{
		return false;
	}

	TArray<UVoxelMetaGraphParameterNodeBase*> ParameterNodes;
	ActiveGraph->GetNodesOfClass<UVoxelMetaGraphParameterNodeBase>(ParameterNodes);

	for (const UVoxelMetaGraphParameterNodeBase* Node : ParameterNodes)
	{
		if (Node->Guid == ParameterId)
		{
			return true;
		}
	}

	return false;
}

void SMembers::DeleteParameterNodes(const FGuid& ParameterId) const
{
	const TSharedPtr<FVoxelMetaGraphEditorToolkit> Toolkit = WeakToolkit.Pin();
	if (!Toolkit)
	{
		return;
	}

	UEdGraph* ActiveGraph = Toolkit->GetActiveGraph();
	if (!ActiveGraph)
	{
		return;
	}

	TArray<UVoxelMetaGraphParameterNodeBase*> ParameterNodes;
	ActiveGraph->GetNodesOfClass<UVoxelMetaGraphParameterNodeBase>(ParameterNodes);

	for (UVoxelMetaGraphParameterNodeBase* Node : ParameterNodes)
	{
		if (Node->Guid == ParameterId)
		{
			ActiveGraph->RemoveNode(Node);
		}
	}
}

END_VOXEL_NAMESPACE(MetaGraph)
