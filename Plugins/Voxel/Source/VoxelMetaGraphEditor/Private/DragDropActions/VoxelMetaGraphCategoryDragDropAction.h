// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "GraphEditorDragDropAction.h"
#include "Widgets/SVoxelMetaGraphMembers.h"

class UVoxelMetaGraph;

BEGIN_VOXEL_NAMESPACE(MetaGraph)

class VOXELMETAGRAPHEDITOR_API FCategoryDragDropAction : public FGraphEditorDragDropAction
{
public:
	DRAG_DROP_OPERATOR_TYPE(FCategoryDragDropAction, FGraphEditorDragDropAction)

	static TSharedRef<FCategoryDragDropAction> New(const FString& InCategory, TWeakObjectPtr<UVoxelMetaGraph> MetaGraph, EMembersNodeSection Section)
	{
		const TSharedRef<FCategoryDragDropAction> Operation = MakeShared<FCategoryDragDropAction>();
		Operation->DraggedCategory = InCategory;
		Operation->Section = Section;
		Operation->MetaGraph = MetaGraph;
		Operation->Construct();
		return Operation;
	}

	//~ Begin FGraphSchemaActionDragDropAction Interface
	virtual void HoverTargetChanged() override;
	virtual FReply DroppedOnCategory(FText Category) override;
	//~ End FGraphSchemaActionDragDropAction Interface

private:
	EMembersNodeSection Section = EMembersNodeSection::MacroInputs;
	FString DraggedCategory;
	TWeakObjectPtr<UVoxelMetaGraph> MetaGraph;
	
	void SetFeedbackMessageError(const FString& Message);
	void SetFeedbackMessageOK(const FString& Message);
};

END_VOXEL_NAMESPACE(MetaGraph)