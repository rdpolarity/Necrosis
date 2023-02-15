// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphMacroParameterNode.h"
#include "VoxelTransaction.h"
#include "VoxelMetaGraphSchema.h"
#include "VoxelMetaGraphToolkit.h"

void UVoxelMetaGraphMacroParameterNode::PostPasteNode()
{
	Super::PostPasteNode();

	UVoxelMetaGraph* MetaGraph = GetTypedOuter<UVoxelMetaGraph>();
	if (!ensure(MetaGraph))
	{
		return;
	}

	const FVoxelTransaction Transaction(MetaGraph);

	// Add new parameter
	// Regenerate guid to be safe
	Guid = FGuid::NewGuid();
	CachedParameter.Guid = Guid;

	MetaGraph->Parameters.Add(CachedParameter);

	ensure(MetaGraph->Parameters.Last().Guid == CachedParameter.Guid);
}

bool UVoxelMetaGraphMacroParameterNode::CanPasteHere(const UEdGraph* TargetGraph) const
{
	if (!Super::CanPasteHere(TargetGraph))
	{
		return false;
	}

	const UVoxelMetaGraph& MetaGraph = UVoxelMetaGraphSchema::GetToolkit(TargetGraph)->GetAssetAs<UVoxelMetaGraph>();
	return MetaGraph.bIsMacroGraph;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelMetaGraphMacroParameterNode::AllocateParameterPins(const FVoxelMetaGraphParameter& Parameter)
{
	UEdGraphPin* Pin = CreatePin(
		Type == EVoxelMetaGraphParameterType::MacroOutput ? EGPD_Input : EGPD_Output,
		Parameter.Type.GetEdGraphPinType(),
		STATIC_FNAME("Value"));

	Pin->PinFriendlyName = FText::FromName(GetParameter()->Name);

	if (Type == EVoxelMetaGraphParameterType::MacroInput)
	{
		CreatePin(
			EGPD_Input,
			Parameter.Type.GetEdGraphPinType(),
			STATIC_FNAME("Preview"));
	}
}

FText UVoxelMetaGraphMacroParameterNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	switch (Type)
	{
	default: check(false);
	case EVoxelMetaGraphParameterType::MacroInput: return VOXEL_LOCTEXT("INPUT");
	case EVoxelMetaGraphParameterType::MacroOutput: return VOXEL_LOCTEXT("OUTPUT");
	}
}

FLinearColor UVoxelMetaGraphMacroParameterNode::GetNodeTitleColor() const
{
	return GetSchema()->GetPinTypeColor(GetParameterSafe().Type.GetEdGraphPinType());
}

void UVoxelMetaGraphMacroParameterNode::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);

	if (Type == EVoxelMetaGraphParameterType::MacroInput &&
		Pin->Direction == EGPD_Input)
	{
		GetTypedOuter<UVoxelMetaGraph>()->OnParametersChanged.Broadcast();
	}
}

void UVoxelMetaGraphMacroParameterNode::PinConnectionListChanged(UEdGraphPin* Pin)
{
	ON_SCOPE_EXIT
	{
		Super::PinConnectionListChanged(Pin);
	};

	if (Type != EVoxelMetaGraphParameterType::MacroInput ||
		Pin->Direction != EGPD_Input)
	{
		return;
	}

	FVoxelSystemUtilities::DelayedCall([MetaGraph = GetTypedOuter<UVoxelMetaGraph>()]
	{
		MetaGraph->OnParametersChanged.Broadcast();
	});
}