// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelMetaGraph.h"

class SGraphActionMenu;
class FVoxelMetaGraphEditorToolkit;
struct FGraphActionNode;
struct FEdGraphEditAction;
struct FEdGraphSchemaAction;
struct FVoxelMetaGraphParameter;
struct FCreateWidgetForActionData;
struct FGraphActionListBuilderBase;

BEGIN_VOXEL_NAMESPACE(MetaGraph)

enum class EMembersNodeSection
{
	Parameters,
	MacroInputs,
	MacroOutputs,
	LocalVariables
};

FORCEINLINE int32 GetSectionId(EMembersNodeSection Type)
{
	switch (Type)
	{
	default: check(false);
	case EMembersNodeSection::Parameters: return 1;
	case EMembersNodeSection::MacroInputs: return 2;
	case EMembersNodeSection::MacroOutputs: return 3;
	case EMembersNodeSection::LocalVariables: return 4;
	}
}
FORCEINLINE EMembersNodeSection GetSection(int32 SectionId)
{
	if (SectionId == 1)
	{
		return EMembersNodeSection::Parameters;
	}
	else if (SectionId == 2)
	{
		return EMembersNodeSection::MacroInputs;
	}
	else if (SectionId == 3)
	{
		return EMembersNodeSection::MacroOutputs;
	}
	else
	{
		ensure(SectionId == 4);
		return EMembersNodeSection::LocalVariables;
	}
}

FORCEINLINE EMembersNodeSection GetSection(const EVoxelMetaGraphParameterType Type)
{
	switch (Type)
	{
	default: ensure(false);
	case EVoxelMetaGraphParameterType::Parameter: return EMembersNodeSection::Parameters;
	case EVoxelMetaGraphParameterType::MacroInput: return EMembersNodeSection::MacroInputs;
	case EVoxelMetaGraphParameterType::MacroOutput: return EMembersNodeSection::MacroOutputs;
	case EVoxelMetaGraphParameterType::LocalVariable: return EMembersNodeSection::LocalVariables;
	}
}
FORCEINLINE int32 GetSectionId(const EVoxelMetaGraphParameterType Type)
{
	return GetSectionId(GetSection(Type));
}

FORCEINLINE EVoxelMetaGraphParameterType GetParameterType(const EMembersNodeSection Type)
{
	switch (Type)
	{
	default: ensure(false);
	case EMembersNodeSection::Parameters: return EVoxelMetaGraphParameterType::Parameter;
	case EMembersNodeSection::MacroInputs: return EVoxelMetaGraphParameterType::MacroInput;
	case EMembersNodeSection::MacroOutputs: return EVoxelMetaGraphParameterType::MacroOutput;
	case EMembersNodeSection::LocalVariables: return EVoxelMetaGraphParameterType::LocalVariable;
	}
}

class FMembersColumnSizeData
{
public:
	DECLARE_DELEGATE_ThreeParams(FOnMembersSlotResized, float, int32, float)
	DECLARE_DELEGATE_RetVal_TwoParams(float, FGetMembersColumnWidth, int32, float)

public:
	
	FMembersColumnSizeData()
	{
		ValueColumnWidthValue = 0.4f;
		HoveredSplitterIndexValue = INDEX_NONE;
		IndentationValue = 10.f;

		NameColumnWidth.BindRaw(this, &FMembersColumnSizeData::GetNameColumnWidth);
		ValueColumnWidth.BindRaw(this, &FMembersColumnSizeData::GetValueColumnWidth);
		HoveredSplitterIndex.BindRaw(this, &FMembersColumnSizeData::GetHoveredSplitterIndex);
		OnValueColumnResized.BindRaw(this, &FMembersColumnSizeData::SetValueColumnWidth);
		OnSplitterHandleHovered.BindRaw(this, &FMembersColumnSizeData::OnSetHoveredSplitterIndex);

		OnNameColumnResized.BindLambda([](float) {}); 
	}

	FGetMembersColumnWidth NameColumnWidth;
	FGetMembersColumnWidth ValueColumnWidth;
	TAttribute<int32> HoveredSplitterIndex;
	SSplitter::FOnSlotResized OnNameColumnResized; 
	FOnMembersSlotResized OnValueColumnResized;
	SSplitter::FOnHandleHovered OnSplitterHandleHovered;

	void SetValueColumnWidth(float NewWidth, int32 Indentation, float CurrentSize)
	{
		if (CurrentSize != 0.f &&
			Indentation != 0)
		{
			NewWidth /= (CurrentSize + Indentation * IndentationValue) / CurrentSize;
		}

		ensureAlways(NewWidth <= 1.0f);
		ValueColumnWidthValue = NewWidth;
	}

private:
	float ValueColumnWidthValue;
	int32 HoveredSplitterIndexValue;
	float IndentationValue;

	float GetNameColumnWidth(int32 Indentation, float CurrentSize) const
	{
		return 1.f - GetValueColumnWidth(Indentation, CurrentSize);
	}

	float GetValueColumnWidth(int32 Indentation, float CurrentSize) const
	{
		if (CurrentSize == 0.f ||
			Indentation == 0)
		{
			return ValueColumnWidthValue;
		}

		return (CurrentSize + Indentation * IndentationValue) / CurrentSize * ValueColumnWidthValue;
	}
	int32 GetHoveredSplitterIndex() const
	{
		return HoveredSplitterIndexValue;
	}

	void OnSetHoveredSplitterIndex(int32 HoveredIndex)
	{
		HoveredSplitterIndexValue = HoveredIndex;
	}
};

class SMembers
	: public SCompoundWidget
	, public FSelfRegisteringEditorUndoClient
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_ARGUMENT(UVoxelMetaGraph*, MetaGraph);
		SLATE_ARGUMENT(TSharedPtr<FVoxelMetaGraphEditorToolkit>, Toolkit);
	};

	void Construct(const FArguments& Args);

	//~ Begin SWidget Interface
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	//~ End SWidget Interface

	void Refresh();
	void SelectAndRequestRename(const FGuid& ItemId, EMembersNodeSection Section);
	void SelectByParameterId(const FGuid& ItemId) const;
	void SelectByName(const FName& Name, ESelectInfo::Type SelectInfo, int32 InSectionID);

	FMembersColumnSizeData& GetColumnSizeData()
	{
		return ColumnSizeData;
	}
	
private:
	bool bNeedsRefresh = false;
	bool bIsRefreshing = false;
	
	TSharedPtr<SSearchBox> FilterBox;
	TSharedPtr<FUICommandList> CommandList;
	TSharedPtr<SGraphActionMenu> MembersMenu;
	
	TWeakObjectPtr<UVoxelMetaGraph> MetaGraph;
	TWeakPtr<FVoxelMetaGraphEditorToolkit> WeakToolkit;

	FMembersColumnSizeData ColumnSizeData;

	//~ Begin Interface FSelfRegisteringEditorUndoClient
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	//~ End Interface FSelfRegisteringEditorUndoClient

	void OnGraphChanged(const FEdGraphEditAction& Action);
	void OnObjectPropertyChanged();
	FString GetPasteCategory() const;

private:
	void CollectStaticSections(TArray<int32>& StaticSectionIDs);
	FText OnGetSectionTitle(int32 InSectionID);
	TSharedRef<SWidget> OnGetMenuSectionWidget(TSharedRef<SWidget> RowWidget, int32 InSectionID);
	
	void CollectAllActions(FGraphActionListBuilderBase& OutAllActions);
	TSharedRef<SWidget> OnCreateWidgetForAction(FCreateWidgetForActionData* const InCreateData);
	void OnActionSelected(const TArray<TSharedPtr<FEdGraphSchemaAction>>& InActions, ESelectInfo::Type InSelectionType);
	bool HandleActionMatchesName(FEdGraphSchemaAction* EdGraphSchemaAction, const FName& Name);
	FReply OnActionDragged(const TArray<TSharedPtr<FEdGraphSchemaAction>>& InActions, const FPointerEvent& MouseEvent);
	void OnMemberActionDoubleClicked(const TArray<TSharedPtr<FEdGraphSchemaAction>>& InActions);
	
	void OnCategoryNameCommitted(const FText& InNewText, ETextCommit::Type InTextCommit, TWeakPtr< FGraphActionNode > InAction);
	FReply OnCategoryDragged(const FText& InCategory, const FPointerEvent& MouseEvent);
	FText GetFilterText() const;
	TSharedPtr<SWidget> OnContextMenuOpening();

private:
	void OnDeleteEntry();
	void OnDuplicateAction();
	
	void OnRequestRenameOnActionNode();
	bool CanRequestRenameOnActionNode() const;
	
	void OnCopy();
	void OnCut();
	
	void OnPaste();
	bool CanPaste();

	bool GetPasteSectionType(const FString& ImportText, EMembersNodeSection& OutSection) const;

private:
	TSharedRef<SWidget> CreateAddButton(int32 InSectionID, FText AddNewText, FName MetaDataTag);

	void OnFilterTextChanged(const FText& InFilterText) const;
	void OnAddNewParameter(EMembersNodeSection Section);
	void SelectParameterDetailsView(const FVoxelMetaGraphParameter* Parameter, bool bCategorySelected = false) const;
	bool IsParameterUsed(const FGuid& ParameterId) const;
	void DeleteParameterNodes(const FGuid& ParameterId) const;
};

END_VOXEL_NAMESPACE(MetaGraph)