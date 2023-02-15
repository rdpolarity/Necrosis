// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelHeightmapCanvasBrush.h"
#include "VoxelHeightmapCanvasAsset.h"

#if 0 // TODO
DEFINE_VOXEL_BRUSH(FVoxelHeightmapCanvasBrush);

void FVoxelHeightmapCanvasBrush::CacheData_GameThread()
{
	if (!Canvas)
	{
		return;
	}

	CanvasData = Canvas->GetData();
	Canvas = nullptr;
}
#endif