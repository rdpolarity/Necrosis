// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelExecNode.h"
#include "VoxelNavmesh/VoxelNavmesh.h"
#include "VoxelNavmesh/VoxelNavmeshProcessor.h"
#include "VoxelNavmeshNodes.generated.h"

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelChunkExecObject_CreateNavmeshComponent : public FVoxelChunkExecObject
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	FVector3d Position = FVector3d::ZeroVector;
	TSharedPtr<const FVoxelNavmesh> Navmesh;

	virtual void Create(FVoxelRuntime& Runtime) const override;
	virtual void Destroy(FVoxelRuntime& Runtime) const override;

private:
	mutable FVoxelNavmeshProcessorId NavmeshId;
};

USTRUCT(Category = "Mesh")
struct VOXELMETAGRAPH_API FVoxelChunkExecNode_CreateNavmeshComponent : public FVoxelChunkExecNode_Default
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelNavmesh, Navmesh, nullptr);

	virtual TValue<FVoxelChunkExecObject> Execute(const FVoxelQuery& Query) const override;
};