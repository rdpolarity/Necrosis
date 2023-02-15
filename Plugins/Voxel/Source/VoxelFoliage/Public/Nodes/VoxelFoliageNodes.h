// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelNodeTypes.h"
#include "VoxelFoliage.h"
#include "VoxelFoliageNodes.generated.h"

struct FVoxelFoliageChunkMeshData
{
	TWeakObjectPtr<UStaticMesh> StaticMesh;
	FVoxelFoliageSettings FoliageSettings;
	FBodyInstance BodyInstance;
	TSharedPtr<TVoxelArray<FTransform3f>> Transforms;
};

USTRUCT()
struct VOXELFOLIAGE_API FVoxelFoliageChunkData : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	FVector ChunkPosition = FVector::Zero();
	int32 InstancesCount = 0;
	TArray<TSharedPtr<FVoxelFoliageChunkMeshData>> Data;
};

USTRUCT(Category = "Foliage")
struct VOXELFOLIAGE_API FVoxelNode_GenerateManualFoliageData : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelStaticMesh, StaticMesh, nullptr);
	VOXEL_INPUT_PIN(FVoxelFoliageSettings, FoliageSettings, nullptr);
	VOXEL_INPUT_PIN(FBodyInstance, BodyInstance, nullptr);
	VOXEL_INPUT_PIN(FVoxelVectorBuffer, Positions, nullptr);
	VOXEL_INPUT_PIN(FVoxelQuaternionBuffer, Rotations, nullptr);
	VOXEL_INPUT_PIN(FVoxelVectorBuffer, Scales, FVector::OneVector);

	VOXEL_OUTPUT_PIN(FVoxelFoliageChunkData, ChunkData);
};

USTRUCT(Category = "Foliage")
struct VOXELFOLIAGE_API FVoxelNode_GenerateHaltonPositions : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(int32, Seed, nullptr, SeedPin);
	VOXEL_INPUT_PIN(float, DistanceBetweenPositions, nullptr);

	VOXEL_OUTPUT_PIN(FVoxelVector2DBuffer, Positions);
};