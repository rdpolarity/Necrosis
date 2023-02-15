// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMetaGraph.h"
#include "VoxelMetaGraphNode.h"
#include "VoxelMetaGraphParameterNodeBase.generated.h"

UCLASS()
class UVoxelMetaGraphParameterNodeBase : public UVoxelMetaGraphNode
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FGuid Guid;

	UPROPERTY()
	FVoxelMetaGraphParameter CachedParameter;

	FVoxelMetaGraphParameter* GetParameter() const;
	const FVoxelMetaGraphParameter& GetParameterSafe() const;

	//~ Begin UVoxelGraphNode Interface
	virtual void AllocateDefaultPins() override final;
	virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const override;
	virtual void PrepareForCopying() override;

	virtual bool CanSplitPin(const UEdGraphPin& Pin) const override;
	virtual void SplitPin(UEdGraphPin& Pin) override;
	//~ End UVoxelGraphNode Interface

protected:
	virtual void AllocateParameterPins(const FVoxelMetaGraphParameter& Parameter) VOXEL_PURE_VIRTUAL();
};