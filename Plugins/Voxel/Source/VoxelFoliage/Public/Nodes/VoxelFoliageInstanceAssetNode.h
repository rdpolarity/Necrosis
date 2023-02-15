// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "Nodes/VoxelFoliageNodes.h"
#include "VoxelFoliageInstanceTemplate.h"
#include "VoxelFoliageInstanceAssetNode.generated.h"

USTRUCT(DisplayName = "Foliage Instance Template")
struct VOXELFOLIAGE_API FVoxelFoliageInstanceTemplateData
{
	GENERATED_BODY()

	TSharedPtr<const FVoxelFoliageInstanceTemplateProxy> TemplateData;
};

USTRUCT()
struct VOXELFOLIAGE_API FVoxelFoliageInstanceTemplateDataPinType : public FVoxelExposedPinType
{
	GENERATED_BODY()

	DEFINE_VOXEL_EXPOSED_PIN_TYPE(FVoxelFoliageInstanceTemplateData, TSoftObjectPtr<UVoxelFoliageInstanceTemplate>)
	{
		const UVoxelFoliageInstanceTemplate* Template = Value.LoadSynchronous();
		if (!Template)
		{
			return {};
		}

		const TSharedRef<FVoxelFoliageInstanceTemplateData> TemplateData = MakeShared<FVoxelFoliageInstanceTemplateData>();
		TemplateData->TemplateData = MakeShared<FVoxelFoliageInstanceTemplateProxy>(*Template);
		return TemplateData;
	}
};

USTRUCT(Category = "Foliage")
struct VOXELFOLIAGE_API FVoxelNode_GenerateInstanceAssetFoliageData : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelFoliageInstanceTemplateData, Asset, nullptr);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Height, nullptr);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Mask, 1.f);

	// Distance between points multiplier
	VOXEL_INPUT_PIN(float, DistanceMultiplier, 1.f);
	VOXEL_INPUT_PIN(float, GradientStep, 100.f);
	VOXEL_INPUT_PIN(int32, Seed, nullptr, SeedPin);

	VOXEL_OUTPUT_PIN(FVoxelFoliageChunkData, ChunkData);
};