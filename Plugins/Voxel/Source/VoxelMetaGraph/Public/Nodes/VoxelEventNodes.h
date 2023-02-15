// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelExecNode.h"
#include "VoxelEventNodes.generated.h"

USTRUCT(meta = (NodeIcon = "Event"))
struct VOXELMETAGRAPH_API FVoxelExecNode_OnConstruct : public FVoxelExecNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_OUTPUT_PIN(FVoxelExec, Then);

	virtual bool HasSideEffects() const override
	{
		return true;
	}

	virtual void Execute(TArray<TValue<FVoxelExecObject>>& OutObjects) const override;
};

USTRUCT(meta = (NodeIcon = "Event"))
struct VOXELMETAGRAPH_API FVoxelExecNode_BroadcastEvent : public FVoxelExecNode_Default
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FName, EventName, "Complete");

	virtual TValue<FVoxelExecObject> Execute(const FVoxelQuery& Query) const override;
};