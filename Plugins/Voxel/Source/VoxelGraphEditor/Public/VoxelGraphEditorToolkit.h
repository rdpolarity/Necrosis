// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Toolkits/VoxelBaseEditorToolkit.h"
#include "GraphEditor.h"

struct VOXELGRAPHEDITOR_API FVoxelGraphDelayOnGraphChangedScope
{
	FVoxelGraphDelayOnGraphChangedScope();
	~FVoxelGraphDelayOnGraphChangedScope();
};

class VOXELGRAPHEDITOR_API FVoxelGraphEditorToolkit : public FVoxelBaseEditorToolkit
{
public:
	using FVoxelBaseEditorToolkit::FVoxelBaseEditorToolkit;

	virtual TArray<TSharedPtr<SGraphEditor>> GetGraphEditors() const = 0;
	virtual TSharedPtr<SGraphEditor> GetActiveGraphEditor() const = 0;
	virtual void OnSelectedNodesChanged(const TSet<UObject*>& NewSelection) {}
	virtual void OnSpawnGraphNodeByShortcut(const FInputChord& Chord, const FVector2D& Position) {}
	virtual void OnGraphChangedImpl();
	virtual void FixupGraphProperties() {}

	void OnGraphChanged();

public:
	FUICommandList& GetGraphEditorCommands() const
	{
		return *GraphEditorCommands;
	}

protected:
	TSharedRef<SGraphEditor> CreateGraphEditor(UEdGraph* Graph);

	TSharedPtr<FUICommandList> GraphEditorCommands;

public:
	//~ Begin FVoxelBaseEditorToolkit Interface
	virtual void BindToolkitCommands() override;
	virtual void CreateInternalWidgets() override;
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;
	//~ End FVoxelBaseEditorToolkit Interface

public:
	//~ Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }
	//~ End FEditorUndoClient Interface

private:
	void OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged) const;
	void OnNodeDoubleClicked(UEdGraphNode* Node);
	FActionMenuContent OnCreateGraphActionMenu(UEdGraph* InGraph, const FVector2D& InNodePosition, const TArray<UEdGraphPin*>& InDraggedPins, bool bAutoExpand, SGraphEditor::FActionMenuClosed InOnMenuClosed) const;

public:
	TSet<UEdGraphNode*> GetSelectedNodes() const;

	void CreateComment() const;
	void DeleteNodes(const TArray<UEdGraphNode*>& Nodes);

	void OnSplitPin();
	void OnRecombinePin();
	
	void OnResetPinToDefaultValue();
	bool CanResetPinToDefaultValue() const;

	void DeleteSelectedNodes();
	bool CanDeleteNodes() const;

	void CutSelectedNodes();
	bool CanCutNodes() const;

	void CopySelectedNodes();
	bool CanCopyNodes() const;

	void PasteNodes();
	void PasteNodesHere(const FVector2D& Location);
	bool CanPasteNodes() const;

	void DuplicateNodes();
	bool CanDuplicateNodes() const;

public:
	bool bDisableOnGraphChanged = false;

private:
	bool bOnGraphChangedCalled = false;
};