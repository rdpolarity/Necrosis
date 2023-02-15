// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMaterialLayerAssetThumbnailRenderer.h"
#include "VoxelMaterialLayerAsset.h"

DEFINE_VOXEL_THUMBNAIL_RENDERER(UVoxelMaterialLayerAssetThumbnailRenderer, UVoxelMaterialLayerAsset);

UStaticMesh* UVoxelMaterialLayerAssetThumbnailRenderer::GetStaticMesh(UObject* Object, TArray<UMaterialInterface*>& OutMaterialOverrides) const
{
	const UVoxelMaterialLayerAsset* Asset = CastChecked<UVoxelMaterialLayerAsset>(Object);
	OutMaterialOverrides.Add(Asset->GetPreviewMaterial());
	return Asset->GetPreviewMesh();
}