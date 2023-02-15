// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelLandmassBrush.h"
#include "VoxelLandmassNodes.generated.h"

USTRUCT()
struct FVoxelLandmassBrushes
{
	GENERATED_BODY()

	TVoxelArray<TSharedPtr<const FVoxelLandmassBrushImpl>> Brushes;
};

USTRUCT(Category = "Landmass")
struct FVoxelNode_FindLandmassBrushes : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_INPUT_PIN(FName, LayerName, "Main");
	VOXEL_OUTPUT_PIN(FVoxelLandmassBrushes, Brushes);
};

USTRUCT(Category = "Landmass")
struct FVoxelNode_SampleLandmassBrushes : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelLandmassBrushes, Brushes, nullptr);
	VOXEL_INPUT_PIN(FVoxelVectorBuffer, Position, nullptr);
	VOXEL_INPUT_PIN(float, MaxDistance, 100.f);
	VOXEL_INPUT_PIN(float, Smoothness, 0.f);
	VOXEL_INPUT_PIN(bool, HermiteInterpolation, true);

	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Distance, DensityPin);
};