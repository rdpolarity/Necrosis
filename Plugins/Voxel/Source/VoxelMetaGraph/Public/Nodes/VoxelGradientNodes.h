// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelGradientNodes.generated.h"

USTRUCT(Category = "Gradient", meta = (Abstract))
struct VOXELMETAGRAPH_API FVoxelNode_GetGradientBase : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Value, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Gradient);

	virtual EVoxelAxis GetAxis() const VOXEL_PURE_VIRTUAL({});
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelNode_GetGradientX : public FVoxelNode_GetGradientBase
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual EVoxelAxis GetAxis() const override { return EVoxelAxis::X; }
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelNode_GetGradientY : public FVoxelNode_GetGradientBase
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual EVoxelAxis GetAxis() const override { return EVoxelAxis::Y; }
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelNode_GetGradientZ : public FVoxelNode_GetGradientBase
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual EVoxelAxis GetAxis() const override { return EVoxelAxis::Z; }
};