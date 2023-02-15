// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMaterialLayer.h"
#include "VoxelMaterialLayerRenderer.h"

TSharedPtr<FVoxelMaterialLayer> FVoxelMaterialLayerPinType::Compute(const TSoftObjectPtr<UVoxelMaterialLayerAsset>& Value) const
{
	FVoxelMaterialLayer Layer;
	if (const UVoxelMaterialLayerAsset* Asset = Value.LoadSynchronous())
	{
		Layer = GetSubsystem<FVoxelMaterialLayerRenderer>().RegisterLayer_GameThread(Asset);
	}
	return MakeSharedCopy(Layer);
}