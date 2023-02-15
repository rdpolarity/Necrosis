// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphParameterNode.h"
#include "VoxelBuffer.h"
#include "VoxelTransaction.h"
#include "VoxelMetaGraphSchema.h"
#include "VoxelMetaGraphToolkit.h"

void UVoxelMetaGraphParameterNode::PostPasteNode()
{
	Super::PostPasteNode();

	UVoxelMetaGraph* MetaGraph = GetTypedOuter<UVoxelMetaGraph>();
	if (!ensure(MetaGraph))
	{
		return;
	}

	if (MetaGraph->bIsMacroGraph)
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
		ParameterByName->ParameterType == EVoxelMetaGraphParameterType::Parameter)
	{
		// Update Guid
		Guid = ParameterByName->Guid;
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

bool UVoxelMetaGraphParameterNode::CanPasteHere(const UEdGraph* TargetGraph) const
{
	if (!Super::CanPasteHere(TargetGraph))
	{
		return false;
	}

	const UVoxelMetaGraph& MetaGraph = UVoxelMetaGraphSchema::GetToolkit(TargetGraph)->GetAssetAs<UVoxelMetaGraph>();
	return !MetaGraph.bIsMacroGraph;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelMetaGraphParameterNode::AllocateParameterPins(const FVoxelMetaGraphParameter& Parameter)
{
	FVoxelPinType Type = Parameter.Type;
	if (bIsBuffer)
	{
		Type = Type.GetBufferType();
	}

	UEdGraphPin* Pin = CreatePin(
		EGPD_Output,
		Type.GetEdGraphPinType(),
		STATIC_FNAME("Value"));

	Pin->PinFriendlyName = FText::FromName(Parameter.Name);
}

FText UVoxelMetaGraphParameterNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType != ENodeTitleType::FullTitle)
	{
		return {};
	}

	return FText::FromName(GetParameterSafe().Name);
}

FLinearColor UVoxelMetaGraphParameterNode::GetNodeTitleColor() const
{
	return GetSchema()->GetPinTypeColor(GetParameterSafe().Type.GetEdGraphPinType());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool UVoxelMetaGraphParameterNode::CanPromotePin(const UEdGraphPin& Pin, FVoxelPinTypeSet& OutTypes) const
{
	const FVoxelMetaGraphParameter* Parameter = GetParameter();
	if (!Parameter)
	{
		return false;
	}

	const FVoxelPinType InnerType = Parameter->Type.GetInnerType();
	const FVoxelPinType BufferType = Parameter->Type.GetBufferType();

	if (!BufferType.IsBuffer())
	{
		return false;
	}

	OutTypes.Add(InnerType);
	OutTypes.Add(BufferType);

	return true;
}

void UVoxelMetaGraphParameterNode::PromotePin(UEdGraphPin& Pin, const FVoxelPinType& NewType)
{
	Modify();

	bIsBuffer = NewType.IsBuffer();

	ReconstructNode(false);
}