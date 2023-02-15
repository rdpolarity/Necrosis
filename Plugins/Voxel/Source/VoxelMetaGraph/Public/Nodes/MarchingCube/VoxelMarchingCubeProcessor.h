// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBuffer.h"

namespace EVoxelTransitionMask
{
	enum Type : uint8
	{
		XMin = 0x01,
		XMax = 0x02,
		YMin = 0x04,
		YMax = 0x08,
		ZMin = 0x10,
		ZMax = 0x20
	};
}

struct FVoxelMarchingCubeProcessor
{
	const int32 ChunkSize;
	const int32 DataSize;

	FVoxelMarchingCubeProcessor(const int32 ChunkSize, const int32 DataSize)
		: ChunkSize(ChunkSize)
		, DataSize(DataSize)
	{
	}

	static constexpr int32 EdgeIndexCount = 3;

	FORCEINLINE int32 GetCacheIndex(int32 EdgeIndex, int32 LX, int32 LY) const
	{
		checkVoxelSlow(0 <= LX && LX < ChunkSize);
		checkVoxelSlow(0 <= LY && LY < ChunkSize);
		checkVoxelSlow(0 <= EdgeIndex && EdgeIndex < EdgeIndexCount);
		return EdgeIndex + LX * EdgeIndexCount + LY * EdgeIndexCount * ChunkSize;
	}
	
	void MainPass(
		TConstVoxelArrayView<float> Densities,
		TVoxelArray<FVoxelInt4>& OutCells,
		TVoxelArray<int32>& OutIndices,
		TVoxelArray<float>& OutVerticesX,
		TVoxelArray<float>& OutVerticesY,
		TVoxelArray<float>& OutVerticesZ) const;
};