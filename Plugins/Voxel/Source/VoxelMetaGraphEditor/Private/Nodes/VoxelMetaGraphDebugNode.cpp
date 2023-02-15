// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphDebugNode.h"
#include "VoxelMetaGraph.h"
#include "VoxelBuffer.h"
#include "VoxelMetaGraphToolkit.h"

void UVoxelMetaGraphDebugNode::AllocateDefaultPins()
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	if (!Node)
	{
		return;
	}

	PinMap.Reset();
	for (const FPin& Pin : Node->GetPins())
	{
		UEdGraphPin* GraphPin = CreatePin(
			Pin.Direction == EPinDirection::Input ? EGPD_Input : EGPD_Output,
			Pin.Type.GetEdGraphPinType(),
			Pin.Name);

		GraphPin->PinToolTip = "Name=" + Pin.Name.ToString() + "\nType=" + Pin.Type.ToString();

		PinMap.Add(&Pin, GraphPin);
	}

	Super::AllocateDefaultPins();
}

FText UVoxelMetaGraphDebugNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	if (!Node)
	{
		return {};
	}

	switch (Node->Type)
	{
	default: check(false);
	case ENodeType::Struct: return FText::FromName(Node->Struct().GetScriptStruct()->GetFName());
	case ENodeType::Macro: return FText::FromName(Node->Macro().Graph->GetFName());
	case ENodeType::Parameter: return FText::FromName(Node->Parameter().Name);
	case ENodeType::MacroInput: return FText::FromName(Node->MacroInput().Name);
	case ENodeType::MacroOutput: return FText::FromName(Node->MacroOutput().Name);
	case ENodeType::Passthrough: return VOXEL_LOCTEXT("Passthrough");
	}
}


FLinearColor UVoxelMetaGraphDebugNode::GetNodeTitleColor() const
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	if (!Node)
	{
		return {};
	}

	switch (Node->Type)
	{
	default: check(false);
	case ENodeType::Struct: return FLinearColor::Black;
	case ENodeType::Macro: return FLinearColor::Black;
	case ENodeType::Parameter: return FLinearColor::Red;
	case ENodeType::MacroInput: return FLinearColor::Red;
	case ENodeType::MacroOutput: return FLinearColor::Red;
	case ENodeType::Passthrough: return FLinearColor::Black;
	}
}

FText UVoxelMetaGraphDebugNode::GetTooltipText() const
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	if (!Node)
	{
		return {};
	}

	switch (Node->Type)
	{
	default: check(false);
	case ENodeType::Struct: return FText::FromString(Node->Struct().GetScriptStruct()->GetName());
	case ENodeType::Macro: return FText::FromString(Node->Macro().Graph->GetPathName());
	case ENodeType::Parameter: return FText::FromString("Guid=" + Node->Parameter().Guid.ToString());
	case ENodeType::MacroInput: return FText::FromString("Guid=" + Node->MacroInput().Guid.ToString());
	case ENodeType::MacroOutput: return FText::FromString("Guid=" + Node->MacroOutput().Guid.ToString());
	case ENodeType::Passthrough: return VOXEL_LOCTEXT("Passthrough");
	}
}

bool UVoxelMetaGraphDebugNode::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const
{
	return false;
}

bool UVoxelMetaGraphDebugNode::CanJumpToDefinition() const
{
	return Node && ensure(Node->Source.GraphNodes.Num() > 0);
}

void UVoxelMetaGraphDebugNode::JumpToDefinition() const
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	TArray<UEdGraphNode*> Nodes;

	ensure(Node->Source.GraphNodes.Num() > 0);
	for (const TWeakObjectPtr<UEdGraphNode> GraphNode : Node->Source.GraphNodes)
	{
		if (!ensure(GraphNode.IsValid()))
		{
			continue;
		}

		Nodes.Add(GraphNode.Get());
	}

	if (Nodes.Num() == 0)
	{
		return;
	}

	FocusOnNodes(Nodes);
}