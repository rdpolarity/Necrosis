// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphLocalVariableDragDropAction.h"
#include "VoxelNode.h"
#include "VoxelExposedPinType.h"
#include "VoxelMetaGraph.h"
#include "VoxelMetaGraphSchema.h"
#include "VoxelMetaGraphVisuals.h"
#include "Nodes/VoxelMetaGraphLocalVariableNode.h"
#include "Nodes/VoxelMetaGraphMacroParameterNode.h"
#include "Styling/SlateIconFinder.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

void FLocalVariableDragDropAction::HoverTargetChanged()
{
	const FVoxelMetaGraphParameter* Parameter = MetaGraph->FindParameterByGuid(ParameterId);
	const UEdGraphPin* PinUnderCursor = GetHoveredPin();
	UEdGraphNode* NodeUnderCursor = GetHoveredNode();

	if (PinUnderCursor &&
		Parameter->ParameterType == EVoxelMetaGraphParameterType::LocalVariable)
	{
		if (PinUnderCursor->bOrphanedPin)
		{
			SetFeedbackMessageError("Cannot make connection to orphaned pin " + PinUnderCursor->PinName.ToString() + "");
			return;
		}
		else
		{
			if (PinUnderCursor->PinType == Parameter->Type &&
				!PinUnderCursor->bNotConnectable)
			{
				if (PinUnderCursor->Direction == EGPD_Input)
				{
					SetFeedbackMessageOK("Make " + PinUnderCursor->PinName.ToString() + " = " + Parameter->Name.ToString() + "");
				}
				else
				{
					SetFeedbackMessageOK("Make " + Parameter->Name.ToString() + " = " + PinUnderCursor->PinName.ToString() + "");
				}
			}
			else
			{
				SetFeedbackMessageError("The type of '" + Parameter->Name.ToString() + "' local variable is not compatible with " + PinUnderCursor->PinName.ToString() + " pin");
			}
			return;
		}
	}

	if (NodeUnderCursor &&
		Parameter->ParameterType == EVoxelMetaGraphParameterType::LocalVariable)
	{
		if (UVoxelMetaGraphParameterNodeBase* ParameterNode = Cast<UVoxelMetaGraphParameterNodeBase>(NodeUnderCursor))
		{
			const FVoxelMetaGraphParameter& NodeParameter = ParameterNode->GetParameterSafe();

			bool bWrite = false;
			if (const UVoxelMetaGraphMacroParameterNode* MacroParameterNode = Cast<UVoxelMetaGraphMacroParameterNode>(ParameterNode))
			{
				bWrite = MacroParameterNode->Type == EVoxelMetaGraphParameterType::MacroOutput;
			}
			else if (Cast<UVoxelMetaGraphLocalVariableDeclarationNode>(ParameterNode))
			{
				bWrite = true;
			}

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

			SetFeedbackMessageOK("Change node to " + FString(bWrite ? "write" : "read") + " '" + Parameter->Name.ToString() + "'" + (bBreakLinks ? ". WARNING: This will break links!" : ""));
			return;
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

	FGraphSchemaActionDragDropAction::HoverTargetChanged();
}

FReply FLocalVariableDragDropAction::DroppedOnPin(FVector2D ScreenPosition, FVector2D GraphPosition)
{
	const FVoxelMetaGraphParameter* Parameter = MetaGraph->FindParameterByGuid(ParameterId);

	if (!ensure(Parameter))
	{
		return FReply::Unhandled();
	}

	if (Parameter->ParameterType != EVoxelMetaGraphParameterType::LocalVariable)
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
		TargetPin->bNotConnectable)
	{
		return FReply::Handled();
	}
	
	FVoxelMetaGraphSchemaAction_NewParameterUsage Action;
	Action.Guid = ParameterId;
	Action.ParameterType = Parameter->ParameterType;
	Action.bDeclaration = TargetPin->Direction == EGPD_Output;
	Action.PerformAction(TargetPin->GetOwningNode()->GetGraph(), TargetPin, GraphPosition, true);
	
	return FReply::Handled();
}

FReply FLocalVariableDragDropAction::DroppedOnNode(FVector2D ScreenPosition, FVector2D GraphPosition)
{
	const FVoxelMetaGraphParameter* Parameter = MetaGraph->FindParameterByGuid(ParameterId);

	if (!ensure(Parameter))
	{
		return FReply::Unhandled();
	}

	if (Parameter->ParameterType != EVoxelMetaGraphParameterType::LocalVariable)
	{
		return FReply::Handled();
	}

	UEdGraphNode* TargetNode = GetHoveredNode();
	if (!TargetNode)
	{
		return FReply::Unhandled();
	}

	if (UVoxelMetaGraphParameterNodeBase* ParameterNode = Cast<UVoxelMetaGraphParameterNodeBase>(TargetNode))
	{
		const FVoxelMetaGraphParameter& NodeParameter = ParameterNode->GetParameterSafe();

		bool bWrite = false;
		if (const UVoxelMetaGraphMacroParameterNode* MacroParameterNode = Cast<UVoxelMetaGraphMacroParameterNode>(ParameterNode))
		{
			bWrite = MacroParameterNode->Type == EVoxelMetaGraphParameterType::MacroOutput;
		}
		else if (Cast<UVoxelMetaGraphLocalVariableDeclarationNode>(ParameterNode))
		{
			bWrite = true;
		}

		const FVoxelTransaction Transaction(MetaGraph, "Replace variable");

		FVoxelMetaGraphSchemaAction_NewParameterUsage Action;
		Action.Guid = ParameterId;
		Action.ParameterType = EVoxelMetaGraphParameterType::LocalVariable;
		Action.bDeclaration = bWrite;
		const UVoxelGraphNode* NewNode = Cast<UVoxelGraphNode>(Action.PerformAction(TargetNode->GetGraph(), nullptr, FVector2D(TargetNode->NodePosX, TargetNode->NodePosY), true));

		if (Parameter->Type == NodeParameter.Type)
		{
			for (int32 Index = 0; Index < FMath::Min(ParameterNode->GetInputPins().Num(), NewNode->GetInputPins().Num()); Index++)
			{
				NewNode->GetInputPin(Index)->CopyPersistentDataFromOldPin(*ParameterNode->GetInputPin(Index));
			}
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

FReply FLocalVariableDragDropAction::DroppedOnPanel(const TSharedRef<SWidget>& Panel, FVector2D ScreenPosition, FVector2D GraphPosition, UEdGraph& Graph)
{
	const FVoxelMetaGraphParameter* Parameter = MetaGraph->FindParameterByGuid(ParameterId);

	if (!ensure(Parameter))
	{
		return FReply::Handled();
	}

	if (Parameter->ParameterType != EVoxelMetaGraphParameterType::LocalVariable)
	{
		return FReply::Handled();
	}

	const FModifierKeysState ModifierKeys = FSlateApplication::Get().GetModifierKeys();
	const bool bModifiedKeysActive = ModifierKeys.IsControlDown() || ModifierKeys.IsAltDown();
	const bool bAutoCreateGetter = bModifiedKeysActive ? ModifierKeys.IsControlDown() : bControlDrag;
	const bool bAutoCreateSetter = bModifiedKeysActive ? ModifierKeys.IsAltDown() : bAltDrag;

	if (bAutoCreateGetter)
	{
		CreateVariable(true, ParameterId, &Graph, GraphPosition, Parameter->ParameterType);
	}
	else if (bAutoCreateSetter)
	{
		CreateVariable(false, ParameterId, &Graph, GraphPosition, Parameter->ParameterType);
	}
	else
	{
		FMenuBuilder MenuBuilder(true, nullptr);

		MenuBuilder.BeginSection("BPVariableDroppedOn", FText::FromName(Parameter->Name) );
		{
			MenuBuilder.AddMenuEntry(
				FText::FromString("Get " + Parameter->Name.ToString()),
				FText::FromString("Create Getter for variable '" + Parameter->Name.ToString() + "'\n(Ctrl-drag to automatically create a getter)"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FLocalVariableDragDropAction::CreateVariable, true, ParameterId, &Graph, GraphPosition, Parameter->ParameterType), FCanExecuteAction())
			);

			MenuBuilder.AddMenuEntry(
				FText::FromString("Set " + Parameter->Name.ToString()),
				FText::FromString("Create Setter for variable '" + Parameter->Name.ToString() + "'\n(Alt-drag to automatically create a setter)"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FLocalVariableDragDropAction::CreateVariable, false, ParameterId, &Graph, GraphPosition, Parameter->ParameterType), FCanExecuteAction())
			);
		}
		MenuBuilder.EndSection();

		FSlateApplication::Get().PushMenu(
			Panel,
			FWidgetPath(),
			MenuBuilder.MakeWidget(),
			ScreenPosition,
			FPopupTransitionEffect( FPopupTransitionEffect::ContextMenu)
		);
	}

	return FReply::Handled();
}

FReply FLocalVariableDragDropAction::DroppedOnAction(TSharedRef<FEdGraphSchemaAction> Action)
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

FReply FLocalVariableDragDropAction::DroppedOnCategory(FText Category)
{
	if (SourceAction.IsValid())
	{
		SourceAction->MovePersistentItemToCategory(Category);
	}
	
	return FReply::Handled();
}

void FLocalVariableDragDropAction::GetDefaultStatusSymbol(const FSlateBrush*& PrimaryBrushOut, FSlateColor& IconColorOut, FSlateBrush const*& SecondaryBrushOut, FSlateColor& SecondaryColorOut) const
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

void FLocalVariableDragDropAction::CreateVariable(bool bGetter, const FGuid ParameterId, UEdGraph* Graph, const FVector2D GraphPosition, const EVoxelMetaGraphParameterType ParameterType)
{
	FVoxelMetaGraphSchemaAction_NewParameterUsage Action;
	Action.Guid = ParameterId;
	Action.ParameterType = ParameterType;
	Action.bDeclaration = !bGetter;
	Action.PerformAction(Graph, nullptr, GraphPosition, true);
}

void FLocalVariableDragDropAction::SetFeedbackMessageError(const FString& Message)
{
	const FSlateBrush* StatusSymbol = FEditorAppStyle::GetBrush("Graph.ConnectorFeedback.Error");
	SetSimpleFeedbackMessage(StatusSymbol, FLinearColor::White, FText::FromString(Message));
}

void FLocalVariableDragDropAction::SetFeedbackMessageOK(const FString& Message)
{
	const FSlateBrush* StatusSymbol = FEditorAppStyle::GetBrush("Graph.ConnectorFeedback.OK");
	SetSimpleFeedbackMessage(StatusSymbol, FLinearColor::White, FText::FromString(Message));
}

END_VOXEL_NAMESPACE(MetaGraph)