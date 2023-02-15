// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMetaGraphParameterNodeBase.h"
#include "VoxelMetaGraphParameterNode.generated.h"

UCLASS()
class UVoxelMetaGraphParameterNode : public UVoxelMetaGraphParameterNodeBase
{
	GENERATED_BODY()

private:
	UPROPERTY()
	bool bIsBuffer = false;

public:
	//~ Begin UVoxelGraphNode Interface
	virtual void PostPasteNode() override;
	virtual bool CanPasteHere(const UEdGraph* TargetGraph) const override;

	virtual void AllocateParameterPins(const FVoxelMetaGraphParameter& Parameter) override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;

	virtual bool CanPromotePin(const UEdGraphPin& Pin, FVoxelPinTypeSet& OutTypes) const override;
	virtual void PromotePin(UEdGraphPin& Pin, const FVoxelPinType& NewType) override;
	//~ End UVoxelGraphNode Interface
};