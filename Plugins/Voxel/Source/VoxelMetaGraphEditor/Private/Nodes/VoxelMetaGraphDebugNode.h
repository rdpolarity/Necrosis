// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelGraphNode.h"
#include "VoxelMetaGraphGraph.h"
#include "VoxelMetaGraphDebugNode.generated.h"

struct FVoxelMetaGraphParameter;

UCLASS()
class UVoxelMetaGraphDebugNode : public UVoxelGraphNode
{
	GENERATED_BODY()

public:
	VOXEL_USE_NAMESPACE_TYPES(MetaGraph, FGraph, FNode, FPin);

	const FNode* Node = nullptr;
	TSharedPtr<const FGraph> Graph;

	TMap<const FPin*, UEdGraphPin*> PinMap;

	//~ Begin UVoxelGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const override;

	virtual bool CanJumpToDefinition() const override;
	virtual void JumpToDefinition() const override;
	//~ End UVoxelGraphNode Interface
};