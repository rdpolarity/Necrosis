// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

DECLARE_VOXEL_MEMORY_STAT(VOXELCANVAS_API, STAT_VoxelHeightmapCanvasMemory, "Voxel Heightmap Canvas Memory");

class VOXELCANVAS_API FVoxelHeightmapCanvasData
{
public:
	FVoxelHeightmapCanvasData() = default;

	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelHeightmapCanvasMemory);

	int64 GetAllocatedSize() const
	{
		return Data.GetAllocatedSize();
	}

	FORCEINLINE const FIntPoint& GetSize() const
	{
		return Size;
	}
	FORCEINLINE TVoxelArrayView<float> GetData()
	{
		return Data;
	}
	FORCEINLINE TConstVoxelArrayView<float> GetData() const
	{
		return Data;
	}

	void Initialize(const FIntPoint& NewSize);
	void Serialize(FArchive& Ar);

private:
	FIntPoint Size = FIntPoint::ZeroValue;
	TVoxelArray<float> Data;
};