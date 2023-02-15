// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "Nodes/VoxelFoliageNodes.h"
#include "VoxelFoliageClusterTemplate.h"
#include "VoxelFoliageClusterAssetNode.generated.h"

USTRUCT(DisplayName = "Foliage Cluster Template")
struct VOXELFOLIAGE_API FVoxelFoliageClusterTemplateData
{
	GENERATED_BODY()

	TSharedPtr<const FVoxelFoliageClusterTemplateProxy> TemplateData;
};

USTRUCT()
struct VOXELFOLIAGE_API FVoxelFoliageClusterTemplateDataPinType : public FVoxelExposedPinType
{
	GENERATED_BODY()

	DEFINE_VOXEL_EXPOSED_PIN_TYPE(FVoxelFoliageClusterTemplateData, TSoftObjectPtr<UVoxelFoliageClusterTemplate>)
	{
		const UVoxelFoliageClusterTemplate* Template = Value.LoadSynchronous();
		if (!Template)
		{
			return {};
		}

		const TSharedRef<FVoxelFoliageClusterTemplateData> TemplateData = MakeShared<FVoxelFoliageClusterTemplateData>();
		TemplateData->TemplateData = MakeShared<FVoxelFoliageClusterTemplateProxy>(*Template);
		return TemplateData;
	}
};

USTRUCT(Category = "Foliage")
struct VOXELFOLIAGE_API FVoxelNode_GenerateClusterAssetFoliageData : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelFoliageClusterTemplateData, Asset, nullptr);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Height, nullptr);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Mask, 1.f);

	// Distance between points multiplier
	VOXEL_INPUT_PIN(float, DistanceMultiplier, 1.f);
	VOXEL_INPUT_PIN(float, MaxHeightDifferenceMultiplier, 1.f);
	VOXEL_INPUT_PIN(float, MaskOccupancyMultiplier, 1.f);
	VOXEL_INPUT_PIN(float, GradientStep, 100.f);
	VOXEL_INPUT_PIN(int32, Seed, nullptr, SeedPin);

	VOXEL_OUTPUT_PIN(FVoxelFoliageChunkData, ChunkData);

public:
	bool CalculateTotalValues(const FVoxelFoliageClusterTemplateData& Asset, TSharedPtr<TVoxelArray<float>>& Values) const;

	int32 CalculateInstancesCount(
		const FVoxelFoliageClusterTemplateData& Asset,
		const uint64 SeedHash,
		const FVoxelVectorBufferView& ClustersPositionView,
		const FVoxelVectorBufferView& ClustersGradient,
		const FVoxelFloatBufferView& ClustersMaskView) const;
};