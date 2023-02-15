// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelExecNode.h"
#include "VoxelCollision/VoxelCollider.h"
#include "VoxelCollision/VoxelCollisionProcessor.h"
#include "VoxelCollisionNodes.generated.h"

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelChunkExecObject_CreateCollisionComponent : public FVoxelChunkExecObject
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	FVector3d Position = FVector3d::ZeroVector;
	TSharedPtr<const FBodyInstance> BodyInstance;
	TSharedPtr<const FVoxelCollider> Collider;

	virtual void Create(FVoxelRuntime& Runtime) const override;
	virtual void Destroy(FVoxelRuntime& Runtime) const override;

private:
	mutable FVoxelCollisionProcessorId CollisionId;
};

USTRUCT(Category = "Mesh")
struct VOXELMETAGRAPH_API FVoxelChunkExecNode_CreateCollisionComponent : public FVoxelChunkExecNode_Default
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_INPUT_PIN(FVoxelCollider, Collider, nullptr);
	VOXEL_INPUT_PIN(FBodyInstance, BodyInstance, nullptr);

	virtual TValue<FVoxelChunkExecObject> Execute(const FVoxelQuery& Query) const override;
};