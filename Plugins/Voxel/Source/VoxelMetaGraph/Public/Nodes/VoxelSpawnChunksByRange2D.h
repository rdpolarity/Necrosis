// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelExecNode.h"
#include "VoxelChunkManager.h"
#include "VoxelSpawnChunksByRange2D.generated.h"

USTRUCT(Category = "Chunk")
struct VOXELMETAGRAPH_API FVoxelExecNode_SpawnChunksByRange2D : public FVoxelExecNode_Default
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_OUTPUT_PIN(FVoxelChunkExec, OnChunkSpawned);
	VOXEL_OUTPUT_PIN(FVoxelExec, OnChunksComplete);

	VOXEL_INPUT_PIN(float, ChunkSize, 3200.f);
	VOXEL_INPUT_PIN(int32, RenderDistanceInChunks, 5);

	virtual TValue<FVoxelExecObject> Execute(const FVoxelQuery& Query) const override;
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelExecObject_SpawnChunksByRange2D : public FVoxelExecObject
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	float ChunkSize = 0;
	int32 RenderDistanceInChunks = 0;

	//~ Begin FVoxelExecObject Interface
	virtual void Tick(FVoxelRuntime& Runtime) override;
	//~ End FVoxelExecObject Interface

private:
	bool bUpdateInProgress = false;
	FVector LastCameraPosition = FVector::ZeroVector;

	struct FVisibleChunks
	{
		FIntPoint Start = FIntPoint::ZeroValue;
		int32 Size = 0;
		FVoxelBitArray32 VisibleChunks;
	};
	TSharedPtr<const FVisibleChunks> VisibleChunks;

	FVoxelCriticalSection CriticalSection;
	TVoxelIntPointMap<TSharedPtr<FVoxelChunkRef>> Chunks;
};