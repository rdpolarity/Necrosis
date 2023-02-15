// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelExecNode.h"
#include "VoxelChunkManager.h"
#include "VoxelSpawnChunksByScreenSize.generated.h"

USTRUCT(Category = "Chunk")
struct VOXELMETAGRAPH_API FVoxelExecNode_SpawnChunksByScreenSize : public FVoxelExecNode_Default
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(float, WorldSize, 1.e6f);
	VOXEL_INPUT_PIN(float, ChunkSize, 3200.f);
	VOXEL_INPUT_PIN(float, ChunkScreenSize, 1.f);
	VOXEL_INPUT_PIN(int32, MaxChunks, 100000);

	VOXEL_OUTPUT_PIN(FVoxelChunkExec, OnChunkSpawned);
	VOXEL_OUTPUT_PIN(FVoxelExec, OnChunksComplete);

	virtual TValue<FVoxelExecObject> Execute(const FVoxelQuery& Query) const override;
};

BEGIN_VOXEL_NAMESPACE(SpawnChunksByScreenSize)

class FOctree;

DECLARE_UNIQUE_VOXEL_ID(FChunkId);

END_VOXEL_NAMESPACE(SpawnChunksByScreenSize)

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelExecObject_SpawnChunksByScreenSize : public FVoxelExecObject
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	float WorldSize = 0;
	float ChunkSize = 0;
	float ChunkScreenSize = 0;
	int32 MaxChunks = 0;

	//~ Begin FVoxelExecObject Interface
	virtual void Tick(FVoxelRuntime& Runtime) override;
	//~ End FVoxelExecObject Interface
	
private:
	VOXEL_USE_NAMESPACE_TYPES(SpawnChunksByScreenSize, FOctree, FChunkId);

	TSharedPtr<const FOctree> Octree;
	bool bTaskInProgress = false;
	FVector LastLocalViewOrigin = FVector(MAX_dbl);
	
	struct FPreviousChunks
	{
		FPreviousChunks() = default;
		virtual ~FPreviousChunks() = default;

		TArray<TSharedPtr<FPreviousChunks>> Children;
	};
	struct FPreviousChunksLeaf : FPreviousChunks
	{
		TSharedPtr<FVoxelChunkRef> ChunkRef;
	};
	struct FChunk
	{
		TSharedPtr<FVoxelChunkRef> ChunkRef;
		TSharedPtr<FPreviousChunks> PreviousChunks;
	};

	FVoxelCriticalSection CriticalSection;
	TMap<FChunkId, TSharedPtr<FChunk>> Chunks;

	void UpdateTree(FVoxelRuntime& Runtime, const FVector& LocalViewOrigin);
};

BEGIN_VOXEL_NAMESPACE(SpawnChunksByScreenSize)

struct FNodeData
{
	bool bIsRendered = false;
	uint8 TransitionMask = 0;
	FChunkId ChunkId = FChunkId::New();
};

struct FChunkInfo
{
	FVoxelBox ChunkBounds;
	int32 LOD = 0;
	uint8 TransitionMask = 0;
	FVoxelIntBox NodeBounds;
};

class FOctree : public TVoxelFlatOctree<FNodeData>
{
public:
	const FVector LocalViewOrigin;
	const FVoxelExecObject_SpawnChunksByScreenSize& Object;

	static constexpr int32 ChunkSize = 8;

	FOctree(
		const int32 Depth,
		const FVector& LocalViewOrigin,
		const FVoxelExecObject_SpawnChunksByScreenSize& Object)
		: TVoxelFlatOctree<FNodeData>(ChunkSize, Depth)
		, LocalViewOrigin(LocalViewOrigin)
		, Object(Object)
	{
	}

	FORCEINLINE FVoxelBox GetChunkBounds(FNode Node) const
	{
		return GetNodeBounds(Node).ToVoxelBox().Scale(Object.ChunkSize / ChunkSize);
	}

	void Update(
		TMap<FChunkId, FChunkInfo>& ChunkInfos,
		TSet<FChunkId>& ChunksToAdd,
		TSet<FChunkId>& ChunksToRemove,
		TSet<FChunkId>& ChunksToUpdate);

	bool AdjacentNodeHasHigherHeight(FNode Node, int32 Direction, int32 Height) const;
};

END_VOXEL_NAMESPACE(SpawnChunksByScreenSize)