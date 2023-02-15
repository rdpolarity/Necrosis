// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "GraphEditorDragDropAction.h"

class UVoxelMetaGraph;
struct FVoxelMetaGraphParameter;
enum class EVoxelMetaGraphParameterType;

BEGIN_VOXEL_NAMESPACE(MetaGraph)

class FLocalVariableDragDropAction : public FGraphSchemaActionDragDropAction
{
public:
	DRAG_DROP_OPERATOR_TYPE(FLocalVariableDragDropAction, FGraphSchemaActionDragDropAction)

	static TSharedRef<FLocalVariableDragDropAction> New(TSharedPtr<FEdGraphSchemaAction> InAction, FGuid ParameterId, TWeakObjectPtr<UVoxelMetaGraph> MetaGraph)
	{
		const TSharedRef<FLocalVariableDragDropAction> Operation = MakeShared<FLocalVariableDragDropAction>();
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
	
	void SetAltDrag(bool InIsAltDrag) {	bAltDrag = InIsAltDrag; }
	void SetCtrlDrag(bool InIsCtrlDrag) { bControlDrag = InIsCtrlDrag; }

private:
	FGuid ParameterId;
	TWeakObjectPtr<UVoxelMetaGraph> MetaGraph;
	bool bControlDrag = false;
	bool bAltDrag = false;

	static void CreateVariable(bool bGetter, const FGuid ParameterId, UEdGraph* Graph, const FVector2D GraphPosition, const EVoxelMetaGraphParameterType ParameterType);
	
	void SetFeedbackMessageError(const FString& Message);
	void SetFeedbackMessageOK(const FString& Message);
};

END_VOXEL_NAMESPACE(MetaGraph)