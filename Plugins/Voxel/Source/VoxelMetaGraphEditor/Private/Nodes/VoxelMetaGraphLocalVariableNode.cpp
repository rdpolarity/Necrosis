// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphLocalVariableNode.h"
#include "VoxelGraphEditorToolkit.h"
#include "EdGraph/EdGraph.h"

void UVoxelMetaGraphLocalVariableNode::PostPasteNode()
{
	Super::PostPasteNode();

	UVoxelMetaGraph* MetaGraph = GetTypedOuter<UVoxelMetaGraph>();
	if (!ensure(MetaGraph))
	{
		return;
	}

	if (MetaGraph->Parameters.FindByKey(Guid))
	{
		return;
	}

	const FVoxelMetaGraphParameter* ParameterByName = MetaGraph->Parameters.FindByKey(CachedParameter.Name);
	if (ParameterByName &&
		ParameterByName->Type == CachedParameter.Type &&
		ParameterByName->ParameterType == EVoxelMetaGraphParameterType::LocalVariable)
	{
		// Update Guid
		Guid = ParameterByName->Guid;
		return;
	}

	const FVoxelTransaction Transaction(MetaGraph);

	// Add new local variable
	// Regenerate guid to be safe
	Guid = FGuid::NewGuid();
	CachedParameter.Guid = Guid;

	MetaGraph->Parameters.Add(CachedParameter);

	ensure(MetaGraph->Parameters.Last().Guid == CachedParameter.Guid);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelMetaGraphLocalVariableDeclarationNode::AllocateParameterPins(const FVoxelMetaGraphParameter& Parameter)
{
	{
		UEdGraphPin* Pin = CreatePin(EGPD_Input, Parameter.Type.GetEdGraphPinType(), FName("InputPin"));
		Pin->bAllowFriendlyName = true;
		Pin->PinFriendlyName = FText::FromName(Parameter.Name);

		Parameter.DefaultValue.ApplyToPinDefaultValue(*Pin);
	}

	{
		UEdGraphPin* Pin = CreatePin(EGPD_Output, Parameter.Type.GetEdGraphPinType(), FName("OutputPin"));
		Pin->bAllowFriendlyName = true;
		Pin->PinFriendlyName = VOXEL_LOCTEXT(" ");
	}
}

void UVoxelMetaGraphLocalVariableDeclarationNode::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	ON_SCOPE_EXIT
	{
		Super::PinDefaultValueChanged(Pin);
	};

	if (Pin->Direction != EGPD_Input)
	{
		return;
	}

	FVoxelMetaGraphParameter* Parameter = GetParameter();
	if (!ensure(Parameter))
	{
		return;
	}

	{
		UVoxelMetaGraph& MetaGraph = *GetTypedOuter<UVoxelMetaGraph>();
		const FVoxelTransaction Transaction(MetaGraph);

		Parameter->DefaultValue = FVoxelPinValue::MakeFromPinDefaultValue(*Pin);

		// Update others
		for (UEdGraphNode* Node : GetGraph()->Nodes)
		{
			const UVoxelMetaGraphLocalVariableDeclarationNode* ParameterNode = Cast<UVoxelMetaGraphLocalVariableDeclarationNode>(Node);
			if (!ParameterNode ||
				ParameterNode->Guid != Guid)
			{
				continue;
			}

			UEdGraphPin* OtherPin = ParameterNode->GetInputPin(0);
			OtherPin->DefaultValue = Pin->DefaultValue;
			OtherPin->DefaultObject = Pin->DefaultObject;
		}
	}
	
	Pin = GetInputPin(0);
}

FText UVoxelMetaGraphLocalVariableDeclarationNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType != ENodeTitleType::FullTitle)
	{
		return VOXEL_LOCTEXT("LOCAL");
	}

	return FText::FromName(GetParameterSafe().Name);
}

UVoxelGraphNode* UVoxelMetaGraphLocalVariableDeclarationNode::IsInLoop()
{
	TSet<UVoxelGraphNode*> VisitedNodes;
	TArray<UVoxelGraphNode*> NodesToProcess;
	NodesToProcess.Add(this);
	while (NodesToProcess.Num() > 0)
	{
		UVoxelGraphNode* CurrentNode = NodesToProcess.Pop(false);

		if (VisitedNodes.Contains(CurrentNode))
		{
			continue;
		}
		VisitedNodes.Add(CurrentNode);

		if (CurrentNode->GetInputPins().IsEmpty())
		{
			continue;
		}

		for (UEdGraphPin* InputPin : CurrentNode->GetInputPins())
		{
			for (const UEdGraphPin* LinkedPin : InputPin->LinkedTo)
			{
				if (UVoxelMetaGraphLocalVariableNode* LocalVariableNode = Cast<UVoxelMetaGraphLocalVariableNode>(LinkedPin->GetOwningNode()))
				{
					if (LocalVariableNode->Guid == Guid)
					{
						return LocalVariableNode;
					}

					if (const UVoxelMetaGraphLocalVariableUsageNode* UsageNode = Cast<UVoxelMetaGraphLocalVariableUsageNode>(LocalVariableNode))
					{
						if (UVoxelMetaGraphLocalVariableDeclarationNode* Declaration = UsageNode->FindDeclaration())
						{
							NodesToProcess.Add(Declaration);
						}
					}
					else if (UVoxelMetaGraphLocalVariableDeclarationNode* DeclarationNode = Cast<UVoxelMetaGraphLocalVariableDeclarationNode>(LocalVariableNode))
					{
						NodesToProcess.Add(DeclarationNode);
					}
				}
				else if (UVoxelGraphNode* VoxelGraphNode = Cast<UVoxelGraphNode>(LinkedPin->GetOwningNode()))
				{
					if (!VoxelGraphNode->GetInputPins().IsEmpty())
					{
						NodesToProcess.Add(VoxelGraphNode);
					}
				}
				else
				{
					ensure(false);
				}
			}
		}
	}

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelMetaGraphLocalVariableUsageNode::AllocateParameterPins(const FVoxelMetaGraphParameter& Parameter)
{
	UEdGraphPin* Pin = CreatePin(EGPD_Output, Parameter.Type.GetEdGraphPinType(), FName("OutputPin"));
	Pin->PinFriendlyName = FText::FromName(Parameter.Name);
}

FText UVoxelMetaGraphLocalVariableUsageNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType != ENodeTitleType::FullTitle)
	{
		return {};
	}

	return FText::FromName(GetParameterSafe().Name);
}

void UVoxelMetaGraphLocalVariableUsageNode::JumpToDefinition() const
{
	UVoxelMetaGraphLocalVariableDeclarationNode* DeclarationNode = FindDeclaration();
	if (!DeclarationNode)
	{
		return;
	}

	const TSharedPtr<SGraphEditor> ActiveGraphEditor = GetToolkit()->GetActiveGraphEditor();
	ActiveGraphEditor->ClearSelectionSet();
	ActiveGraphEditor->SetNodeSelection(DeclarationNode, true);
	ActiveGraphEditor->ZoomToFit(true);
}

UVoxelMetaGraphLocalVariableDeclarationNode* UVoxelMetaGraphLocalVariableUsageNode::FindDeclaration() const
{
	TArray<UVoxelMetaGraphLocalVariableDeclarationNode*> DeclarationNodes;
	GetGraph()->GetNodesOfClass<UVoxelMetaGraphLocalVariableDeclarationNode>(DeclarationNodes);

	for (UVoxelMetaGraphLocalVariableDeclarationNode* Node : DeclarationNodes)
	{
		if (Node->Guid == Guid)
		{
			return Node;
		}
	}

	return nullptr;
}