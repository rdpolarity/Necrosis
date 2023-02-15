// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelLandmassMeshBrushThumbnailRenderer.h"
#include "VoxelLandmassMeshBrush.h"

DEFINE_VOXEL_THUMBNAIL_RENDERER(UVoxelLandmassMeshBrushThumbnailRenderer, UVoxelLandmassMeshBrush);

UStaticMesh* UVoxelLandmassMeshBrushThumbnailRenderer::GetStaticMesh(UObject* Object, TArray<UMaterialInterface*>& OutMaterialOverrides) const
{
	return CastChecked<UVoxelLandmassMeshBrush>(Object)->Mesh.LoadSynchronous();
}