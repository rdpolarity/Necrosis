// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelGraphNode.h"
#include "VoxelMetaGraphNodeInterface.h"
#include "VoxelMetaGraphNode.generated.h"

class IVoxelNodeDefinition;

UCLASS(Abstract)
class UVoxelMetaGraphNode : public UVoxelGraphNode, public IVoxelMetaGraphNodeInterface
{
	GENERATED_BODY()

public:
	//~ Begin UVoxelGraphNode Interface
	virtual TSharedRef<SWidget> MakeStatWidget() const override;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
	//~ End UVoxelGraphNode Interface

	virtual bool CanPromotePin(const UEdGraphPin& Pin, FVoxelPinTypeSet& OutTypes) const { return false; }
	virtual void PromotePin(UEdGraphPin& Pin, const FVoxelPinType& NewType) VOXEL_PURE_VIRTUAL();

	virtual TSharedRef<IVoxelNodeDefinition> GetNodeDefinition();

public:
	virtual bool TryMigratePin(UEdGraphPin* OldPin, UEdGraphPin* NewPin) const override;
	virtual bool TryMigrateDefaultValue(const UEdGraphPin* OldPin, UEdGraphPin* NewPin) const override;
	virtual void PostReconstructNode() override;
};