// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelExecNode.h"
#include "VoxelChunkManager.h"
#include "VoxelSpawnChunksByRange.generated.h"

USTRUCT(Category = "Chunk")
struct VOXELMETAGRAPH_API FVoxelExecNode_SpawnChunksByRange : public FVoxelExecNode_Default
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(float, ChunkSize, 3200.f);
	VOXEL_INPUT_PIN(int32, RenderDistanceInChunks, 5);

	VOXEL_OUTPUT_PIN(FVoxelChunkExec, OnChunkSpawned);
	VOXEL_OUTPUT_PIN(FVoxelExec, OnChunksComplete);

	virtual TValue<FVoxelExecObject> Execute(const FVoxelQuery& Query) const override;
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelExecObject_SpawnChunksByRange : public FVoxelExecObject
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
		FIntVector Start = FIntVector::ZeroValue;
		int32 Size = 0;
		FVoxelBitArray32 VisibleChunks;
	};
	TSharedPtr<const FVisibleChunks> VisibleChunks;

	FVoxelCriticalSection CriticalSection;
	TVoxelIntVectorMap<TSharedPtr<FVoxelChunkRef>> Chunks;
};