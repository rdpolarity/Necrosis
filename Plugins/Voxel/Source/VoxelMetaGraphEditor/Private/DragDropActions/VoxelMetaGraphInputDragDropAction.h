// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "GraphEditorDragDropAction.h"

class UVoxelMetaGraph;
struct FVoxelMetaGraphParameter;

BEGIN_VOXEL_NAMESPACE(MetaGraph)

class FInputDragDropAction : public FGraphSchemaActionDragDropAction
{
public:
	DRAG_DROP_OPERATOR_TYPE(FInputDragDropAction, FGraphSchemaActionDragDropAction)

	static TSharedRef<FInputDragDropAction> New(TSharedPtr<FEdGraphSchemaAction> InAction, FGuid ParameterId, TWeakObjectPtr<UVoxelMetaGraph> MetaGraph)
	{
		const TSharedRef<FInputDragDropAction> Operation = MakeShared<FInputDragDropAction>();
		Operation->MetaGraph = MetaGraph;
		Operation->ParameterId = ParameterId;
		Operation->SourceAction = InAction;
		Operation->Construct();
		return Operation;
	}

	//~ Begin FGraphSchemaActionDragDropAction Interface
	virtual void HoverTargetChanged() override;

	virtual FReply DroppedOnPin(FVector2D ScreenPosition, FVector2D GraphPosition) override;
	virtual FReply DroppedOnNode(FVector2D ScreenPosition, FVector2D GraphPosition) override;
	virtual FReply DroppedOnPanel(const TSharedRef<SWidget>& Panel, FVector2D ScreenPosition, FVector2D GraphPosition, UEdGraph& Graph) override;
	virtual FReply DroppedOnAction(TSharedRef<FEdGraphSchemaAction> Action) override;
	virtual FReply DroppedOnCategory(FText Category) override;

	virtual void GetDefaultStatusSymbol(const FSlateBrush*& PrimaryBrushOut, FSlateColor& IconColorOut, FSlateBrush const*& SecondaryBrushOut, FSlateColor& SecondaryColorOut) const override;
	//~ End FGraphSchemaActionDragDropAction Interface

private:
	FGuid ParameterId;
	TWeakObjectPtr<UVoxelMetaGraph> MetaGraph;

	void SetFeedbackMessageError(const FString& Message);
	void SetFeedbackMessageOK(const FString& Message);
};

END_VOXEL_NAMESPACE(MetaGraph)