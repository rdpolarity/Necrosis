// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelExecNode.h"
#include "Nodes/VoxelFoliageNodes.h"
#include "Collision/VoxelFoliageCollision.h"
#include "VoxelFoliageCollisionNode.generated.h"

struct FVoxelFoliageCollisionData
{
	TWeakObjectPtr<UStaticMesh> StaticMesh;
	FBodyInstance BodyInstance;
	TSharedPtr<const FVoxelFoliageData> FoliageData;
};

USTRUCT()
struct VOXELFOLIAGE_API FVoxelChunkExecObject_CreateFoliageCollision : public FVoxelChunkExecObject
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	FVector Position = FVector::Zero();
	TArray<FVoxelFoliageCollisionData> TemplatesData;

	virtual void Create(FVoxelRuntime& Runtime) const override;
	virtual void Destroy(FVoxelRuntime& Runtime) const override;

private:
	mutable TSet<FVoxelFoliageCollisionId> CollisionIds;
};

USTRUCT(Category = "Foliage", DisplayName = "Create Foliage Collision Component")
struct VOXELFOLIAGE_API FVoxelChunkExecNode_CreateFoliageCollision : public FVoxelChunkExecNode_Default
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelFoliageChunkData, ChunkData, nullptr);

	virtual TValue<FVoxelChunkExecObject> Execute(const FVoxelQuery& Query) const override;
};