// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#if 0 // TODO
#include "VoxelMinimal.h"
#include "VoxelBlockNodes.h"
#include "VoxelBlockStructureNodes.generated.h"

class FVoxelBlockDataChunkProvider
{
public:
	static constexpr int32 ChunkSize = 16;

	const TVoxelInputPin<FVoxelBlockDataBuffer>& Pin;

	FVoxelBlockDataChunkProvider(
		const FVoxelQuery& Query,
		const FVoxelBlockChunkQueryData& BlockChunkQueryData,
		const TVoxelInputPin<FVoxelBlockDataBuffer>& Pin)
		: Pin(Pin)
		, ChunkQuery(Query)
		, BlockChunkQueryData(ChunkQuery.Add<FVoxelBlockChunkQueryData>())
	{
	}

	FORCEINLINE FVoxelBlockData* RESTRICT GetChunk(const FIntVector& ChunkKey)
	{
		if (ChunkKey == LastChunkKey)
		{
			checkVoxelSlow(LastChunk);
			return LastChunk;
		}

		FVoxelBlockData* RESTRICT Chunk = GetChunk_SkipCache(ChunkKey);
		LastChunkKey = ChunkKey;
		LastChunk = Chunk;
		return Chunk;
	}
	FORCENOINLINE FVoxelBlockData* RESTRICT GetChunk_SkipCache(const FIntVector& ChunkKey)
	{
		TUniquePtr<FVoxelBlockDataBuffer>& Chunk = Chunks.FindOrAdd(ChunkKey);
		if (!Chunk)
		{
			BlockChunkQueryData.Initialize(FVoxelIntBox(ChunkKey * ChunkSize, (ChunkKey + 1) * ChunkSize));
			Chunk = Pin.GetCopy(ChunkQuery);
			BlockChunkQueryData = {};

			if (Chunk->IsConstant())
			{
				const FVoxelBlockData Value = Chunk->GetTypedConstant();

				Chunk = MakeUnique<FVoxelBlockDataBuffer>();
				Chunk->Allocate(FMath::Cube(ChunkSize));

				FVoxelUtilities::SetAll(Chunk->GetWriteView(), Value);
			}
		}

		checkVoxelSlow(Chunk->GetWriteView().Num() == FMath::Cube(ChunkSize));
		return Chunk->GetWriteView().GetData();
	}

	FORCEINLINE FVoxelBlockData& Get(const FIntVector& Position)
	{
		const FIntVector ChunkKey = FVoxelUtilities::DivideFloor(Position, ChunkSize);
		return GetChunk(ChunkKey)[FVoxelUtilities::Get3DIndex(ChunkSize, Position - ChunkKey * ChunkSize)];
	}
	FORCEINLINE FVoxelBlockData& Get(int32 X, int32 Y, int32 Z)
	{
		return Get(FIntVector(X, Y, Z));
	}

	TUniquePtr<FVoxelBlockDataBuffer> GetBlockDataBuffer(const FVoxelIntBox& Bounds)
	{
		VOXEL_FUNCTION_COUNTER();

		if (Bounds.Min % ChunkSize == 0 &&
			Bounds.Max == Bounds.Min + ChunkSize)
		{
			const FIntVector ChunkKey = Bounds.Min / ChunkSize;
			(void)GetChunk_SkipCache(ChunkKey);
			return MoveTemp(Chunks[ChunkKey]);
		}

		const int32 Num = Bounds.Count_SmallBox();

		TUniquePtr<FVoxelBlockDataBuffer> Buffer = MakeUnique<FVoxelBlockDataBuffer>();
		Buffer->Allocate(Num);

		VOXEL_SLOW_SCOPE_COUNTER("Copy");

		int32 Index = 0;
		for (int32 Z = Bounds.Min.Z; Z < Bounds.Max.Z; Z++) 
		{
			for (int32 Y = Bounds.Min.Y; Y < Bounds.Max.Y; Y++)
			{
				for (int32 X = Bounds.Min.X; X < Bounds.Max.X; X++)
				{
					Buffer->SetTyped(Index++, Get(X, Y, Z));
				}
			}
		}
		check(Index == Num);

		return Buffer;
	}

private:
	FIntVector LastChunkKey = FIntVector(MAX_int32);
	FVoxelBlockData* RESTRICT LastChunk = nullptr;

	FVoxelQuery ChunkQuery;
	FVoxelBlockChunkQueryData& BlockChunkQueryData;

	TVoxelIntVectorMap<TUniquePtr<FVoxelBlockDataBuffer>> Chunks;
};

USTRUCT()
struct FVoxelBlockStructurePositions
{
	GENERATED_BODY()

	TVoxelArray<FIntVector> Positions;
};

USTRUCT(Category = "Block")
struct VOXELBLOCK_API FVoxelNode_ApplyBlockTree : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelBlockDataBuffer, Block, nullptr);
	VOXEL_INPUT_PIN(FVoxelBlockStructurePositions, Positions, nullptr);
	VOXEL_INPUT_PIN(FVoxelBlockData, TrunkBlock, nullptr);
	VOXEL_INPUT_PIN(FVoxelBlockData, FoliageBlock, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelBlockDataBuffer, Block);
};

USTRUCT(Category = "Block")
struct VOXELBLOCK_API FVoxelNode_GetTreePositions : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(float, DistanceBetweenTrees, 5.f);
	VOXEL_INPUT_PIN(FVoxelInt32Buffer, Height, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelBlockStructurePositions, Positions);
};
#endif