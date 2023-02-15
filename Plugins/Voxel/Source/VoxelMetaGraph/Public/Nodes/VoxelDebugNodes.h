// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelExecNode.h"
#include "VoxelDebugDrawSubsystem.h"
#include "VoxelDebugNodes.generated.h"

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelChunkExecObject_DrawDebugPoints : public FVoxelChunkExecObject
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	TVoxelArray<FBatchedPoint> Points;

	virtual void Create(FVoxelRuntime& Runtime) const override;
	virtual void Destroy(FVoxelRuntime& Runtime) const override;

private:
	mutable FVoxelDebugDrawId DebugDrawId;
};

USTRUCT(Category = "Debug")
struct VOXELMETAGRAPH_API FVoxelChunkExecNode_DrawDebugPoints : public FVoxelChunkExecNode_Default
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelVectorBuffer, Position, nullptr);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Size, 10.f);
	VOXEL_INPUT_PIN(FVoxelLinearColorBuffer, Color, FLinearColor::Red);

	virtual TValue<FVoxelChunkExecObject> Execute(const FVoxelQuery& Query) const override;
};