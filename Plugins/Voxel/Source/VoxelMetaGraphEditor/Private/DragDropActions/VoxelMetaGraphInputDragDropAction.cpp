// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphInputDragDropAction.h"
#include "VoxelMetaGraph.h"
#include "VoxelMetaGraphSchema.h"
#include "VoxelMetaGraphVisuals.h"
#include "Nodes/VoxelMetaGraphLocalVariableNode.h"
#include "Nodes/VoxelMetaGraphMacroParameterNode.h"
#include "Styling/SlateIconFinder.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

void FInputDragDropAction::HoverTargetChanged()
{
	const FVoxelMetaGraphParameter* Parameter = MetaGraph->FindParameterByGuid(ParameterId);
	const UEdGraphPin* PinUnderCursor = GetHoveredPin();
	UEdGraphNode* NodeUnderCursor = GetHoveredNode();

	if (PinUnderCursor &&
		Parameter->ParameterType == EVoxelMetaGraphParameterType::Parameter)
	{
		if (PinUnderCursor->bOrphanedPin)
		{
			SetFeedbackMessageError("Cannot make connection to orphaned pin " + PinUnderCursor->PinName.ToString() + "");
			return;
		}
		else
		{
			if (PinUnderCursor->Direction != EGPD_Input)
			{
				SetFeedbackMessageError("Input parameter '" + Parameter->Name.ToString() + "' cannot be connected to output pin");
				return;
			}

			if (PinUnderCursor->PinType == Parameter->Type &&
				!PinUnderCursor->bNotConnectable)
			{
				SetFeedbackMessageOK("Make " + PinUnderCursor->PinName.ToString() + " = " + Parameter->Name.ToString() + "");
			}
			else
			{
				SetFeedbackMessageError("The type of '" + Parameter->Name.ToString() + "' parameter is not compatible with " + PinUnderCursor->PinName.ToString() + " pin");
			}
			return;
		}
	}

	if (NodeUnderCursor &&
		Parameter->ParameterType == EVoxelMetaGraphParameterType::Parameter)
	{
		if (UVoxelMetaGraphParameterNodeBase* ParameterNode = Cast<UVoxelMetaGraphParameterNodeBase>(NodeUnderCursor))
		{
			const FVoxelMetaGraphParameter& NodeParameter = ParameterNode->GetParameterSafe();

			bool bCanChangeNode = true;
			if (const UVoxelMetaGraphMacroParameterNode* MacroParameterNode = Cast<UVoxelMetaGraphMacroParameterNode>(ParameterNode))
			{
				bCanChangeNode = MacroParameterNode->Type == EVoxelMetaGraphParameterType::MacroInput;
			}
			else if (Cast<UVoxelMetaGraphLocalVariableDeclarationNode>(ParameterNode))
			{
				bCanChangeNode = false;
			}

			if (bCanChangeNode)
			{
				bool bBreakLinks = false;
				if (Parameter->Type != NodeParameter.Type)
				{
					for (const UEdGraphPin* Pin : ParameterNode->GetAllPins())
					{
						if (Pin->LinkedTo.Num() > 0)
						{
							bBreakLinks = true;
							break;
						}
					}
				}

				SetFeedbackMessageOK("Change node to read '" + Parameter->Name.ToString() + "'" + (bBreakLinks ? ". WARNING: This will break links!" : ""));
				return;
			}
		}
	}

	if (SourceAction.IsValid())
	{
		if (!HoveredCategoryName.IsEmpty())
		{
			if (HoveredCategoryName.ToString() == SourceAction->GetCategory().ToString())
			{
				SetFeedbackMessageError("'" + SourceAction->GetMenuDescription().ToString() + "' is already in category '" + HoveredCategoryName.ToString() + "'");
			}
			else
			{
				SetFeedbackMessageOK("Move '" + SourceAction->GetMenuDescription().ToString() + "' to category '" + HoveredCategoryName.ToString() + "'");
			}
			return;
		}
		else if (HoveredAction.IsValid())
		{
			const TSharedPtr<FEdGraphSchemaAction> HoveredActionPtr = HoveredAction.Pin();

			if (HoveredActionPtr->SectionID == SourceAction->SectionID)
			{
				if (SourceAction->GetPersistentItemDefiningObject() == HoveredActionPtr->GetPersistentItemDefiningObject())
				{
					const int32 MovingItemIndex = SourceAction->GetReorderIndexInContainer();
					const int32 TargetVarIndex = HoveredActionPtr->GetReorderIndexInContainer();

					if (MovingItemIndex == INDEX_NONE)
					{
						SetFeedbackMessageError("Cannot reorder '" + SourceAction->GetMenuDescription().ToString() + "'.");
					}
					else if (TargetVarIndex == INDEX_NONE)
					{
						SetFeedbackMessageError("Cannot reorder '" + SourceAction->GetMenuDescription().ToString() + "' before '" + HoveredActionPtr->GetMenuDescription().ToString() + "'.");
					}
					else if (HoveredActionPtr == SourceAction)
					{
						SetFeedbackMessageError("Cannot reorder '" + SourceAction->GetMenuDescription().ToString() + "' before itself.");
					}
					else
					{
						SetFeedbackMessageOK("Reorder '" + SourceAction->GetMenuDescription().ToString() + "' before '" + HoveredActionPtr->GetMenuDescription().ToString() + "'");
					}
				}
				else
				{
					SetFeedbackMessageError("Cannot reorder '" + SourceAction->GetMenuDescription().ToString() + "' into a different scope.");
				}
			}
			else
			{
				SetFeedbackMessageError("Cannot reorder '" + SourceAction->GetMenuDescription().ToString() + "' into a different section.");
			}

			return;
		}
	}
	
	if (Parameter->ParameterType == EVoxelMetaGraphParameterType::MacroOutput)
	{
		SetFeedbackMessageError("Output parameter '" + Parameter->Name.ToString() + "' is already used in graph");
		return;
	}
	
	if (Parameter->ParameterType == EVoxelMetaGraphParameterType::MacroInput)
	{
		SetFeedbackMessageError("Input parameter '" + Parameter->Name.ToString() + "' is already used in graph");
		return;
	}
	
	FGraphSchemaActionDragDropAction::HoverTargetChanged();
}

FReply FInputDragDropAction::DroppedOnPin(FVector2D ScreenPosition, FVector2D GraphPosition)
{
	const FVoxelMetaGraphParameter* Parameter = MetaGraph->FindParameterByGuid(ParameterId);

	if (!ensure(Parameter))
	{
		return FReply::Unhandled();
	}

	if (Parameter->ParameterType != EVoxelMetaGraphParameterType::Parameter)
	{
		return FReply::Handled();
	}
	
	UEdGraphPin* TargetPin = GetHoveredPin();
	if (!TargetPin ||
		TargetPin->bOrphanedPin)
	{
		return FReply::Handled();
	}

	if (TargetPin->PinType != Parameter->Type ||
		TargetPin->Direction == EGPD_Output ||
		TargetPin->bNotConnectable)
	{
		return FReply::Handled();
	}
	
	FVoxelMetaGraphSchemaAction_NewParameterUsage Action;
	Action.Guid = ParameterId;
	Action.ParameterType = Parameter->ParameterType;
	Action.PerformAction(TargetPin->GetOwningNode()->GetGraph(), TargetPin, GraphPosition, true);
	
	return FReply::Handled();
}

FReply FInputDragDropAction::DroppedOnNode(FVector2D ScreenPosition, FVector2D GraphPosition)
{
	const FVoxelMetaGraphParameter* Parameter = MetaGraph->FindParameterByGuid(ParameterId);

	if (!ensure(Parameter))
	{
		return FReply::Unhandled();
	}

	if (Parameter->ParameterType != EVoxelMetaGraphParameterType::Parameter)
	{
		return FReply::Unhandled();
	}

	UEdGraphNode* TargetNode = GetHoveredNode();
	if (!TargetNode)
	{
		return FReply::Unhandled();
	}

	if (UVoxelMetaGraphParameterNodeBase* ParameterNode = Cast<UVoxelMetaGraphParameterNodeBase>(TargetNode))
	{
		const FVoxelMetaGraphParameter& NodeParameter = ParameterNode->GetParameterSafe();

		if (const UVoxelMetaGraphMacroParameterNode* MacroParameterNode = Cast<UVoxelMetaGraphMacroParameterNode>(ParameterNode))
		{
			if (MacroParameterNode->Type != EVoxelMetaGraphParameterType::MacroInput)
			{
				return FReply::Unhandled();
			}
		}
		else if (Cast<UVoxelMetaGraphLocalVariableDeclarationNode>(ParameterNode))
		{
			return FReply::Unhandled();
		}

		const FVoxelTransaction Transaction(MetaGraph, "Replace variable");

		FVoxelMetaGraphSchemaAction_NewParameterUsage Action;
		Action.Guid = ParameterId;
		Action.ParameterType = EVoxelMetaGraphParameterType::Parameter;
		const UVoxelGraphNode* NewNode = Cast<UVoxelGraphNode>(Action.PerformAction(TargetNode->GetGraph(), nullptr, FVector2D(TargetNode->NodePosX, TargetNode->NodePosY), true));

		if (Parameter->Type == NodeParameter.Type)
		{
			for (int32 Index = 0; Index < FMath::Min(ParameterNode->GetOutputPins().Num(), NewNode->GetOutputPins().Num()); Index++)
			{
				NewNode->GetOutputPin(Index)->CopyPersistentDataFromOldPin(*ParameterNode->GetOutputPin(Index));
			}
		}

		TargetNode->GetGraph()->RemoveNode(TargetNode);

		return FReply::Handled();
	}

	return FGraphSchemaActionDragDropAction::DroppedOnNode(ScreenPosition, GraphPosition);
}

FReply FInputDragDropAction::DroppedOnPanel(const TSharedRef<SWidget>& Panel, FVector2D ScreenPosition, FVector2D GraphPosition, UEdGraph& Graph)
{
	const FVoxelMetaGraphParameter* Parameter = MetaGraph->FindParameterByGuid(ParameterId);

	if (!ensure(Parameter))
	{
		return FReply::Handled();
	}

	if (Parameter->ParameterType != EVoxelMetaGraphParameterType::Parameter)
	{
		return FReply::Handled();
	}

	FVoxelMetaGraphSchemaAction_NewParameterUsage Action;
	Action.Guid = ParameterId;
	Action.ParameterType = Parameter->ParameterType;
	Action.PerformAction(&Graph, nullptr, GraphPosition, true);
	
	return FReply::Handled();
}

FReply FInputDragDropAction::DroppedOnAction(TSharedRef<FEdGraphSchemaAction> Action)
{
	if (!SourceAction.IsValid() ||
		SourceAction->GetTypeId() != Action->GetTypeId() ||
		SourceAction->GetPersistentItemDefiningObject() != Action->GetPersistentItemDefiningObject())
	{
		return FReply::Unhandled();
	}

	SourceAction->ReorderToBeforeAction(Action);
	return FReply::Handled();
}

FReply FInputDragDropAction::DroppedOnCategory(FText Category)
{
	if (SourceAction.IsValid())
	{
		SourceAction->MovePersistentItemToCategory(Category);
	}
	
	return FReply::Handled();
}

void FInputDragDropAction::GetDefaultStatusSymbol(const FSlateBrush*& PrimaryBrushOut, FSlateColor& IconColorOut, FSlateBrush const*& SecondaryBrushOut, FSlateColor& SecondaryColorOut) const
{
	FGraphSchemaActionDragDropAction::GetDefaultStatusSymbol(PrimaryBrushOut, IconColorOut, SecondaryBrushOut, SecondaryColorOut);

	const FVoxelMetaGraphParameter* Parameter = MetaGraph->FindParameterByGuid(ParameterId);
	if (!ensure(Parameter))
	{
		return;
	}

	PrimaryBrushOut = FVoxelMetaGraphVisuals::GetPinIcon(Parameter->Type).GetIcon();
	IconColorOut = FVoxelMetaGraphVisuals::GetPinColor(Parameter->Type);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FInputDragDropAction::SetFeedbackMessageError(const FString& Message)
{
	const FSlateBrush* StatusSymbol = FEditorAppStyle::GetBrush("Graph.ConnectorFeedback.Error");
	SetSimpleFeedbackMessage(StatusSymbol, FLinearColor::White, FText::FromString(Message));
}

void FInputDragDropAction::SetFeedbackMessageOK(const FString& Message)
{
	const FSlateBrush* StatusSymbol = FEditorAppStyle::GetBrush("Graph.ConnectorFeedback.OK");
	SetSimpleFeedbackMessage(StatusSymbol, FLinearColor::White, FText::FromString(Message));
}

END_VOXEL_NAMESPACE(MetaGraph)