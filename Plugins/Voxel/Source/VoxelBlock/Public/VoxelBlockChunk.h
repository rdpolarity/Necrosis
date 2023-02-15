// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBlockTypes.h"

struct VOXELBLOCK_API FVoxelBlockChunk
{
	TVoxelStaticArray<FVoxelBlockData, FMath::Cube(BlockChunkSize)> Blocks{ NoInit };
};

struct VOXELBLOCK_API FVoxelBlockChunkArray
{
	FIntVector Start;
	FIntVector Size;
	TVoxelArray<FVoxelBlockChunk*> Chunks;

	FVoxelBlockData& Get(int32 X, int32 Y, int32 Z)
	{
		const FIntVector ChunkKey = FVoxelUtilities::DivideFloor(FIntVector(X, Y, Z), BlockChunkSize);
		if (ChunkKey.X < Start.X || Start.X + Size.X <= ChunkKey.X ||
			ChunkKey.Y < Start.Y || Start.Y + Size.Y <= ChunkKey.Y ||
			ChunkKey.Z < Start.Z || Start.Z + Size.Z <= ChunkKey.Z)
		{
			Generate(ChunkKey);
		}

		FVoxelBlockChunk* Chunk = Chunks[FVoxelUtilities::Get3DIndex(Size, ChunkKey, Start)];
		checkVoxelSlow(Chunk);
		return Chunk->Blocks[FVoxelUtilities::Get3DIndex(BlockChunkSize, FIntVector(X, Y, Z) - ChunkKey * BlockChunkSize)];
	}

	FORCENOINLINE void Generate(const FIntVector& ChunkKey)
	{
		
	}
};