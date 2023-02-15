// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelBlockAssetThumbnailRenderer.h"
#include "VoxelBlockAsset.h"

DEFINE_VOXEL_THUMBNAIL_RENDERER(UVoxelBlockAssetThumbnailRenderer, UVoxelBlockAsset);

UStaticMesh* UVoxelBlockAssetThumbnailRenderer::GetStaticMesh(UObject* Object, TArray<UMaterialInterface*>& OutMaterialOverrides) const
{
	UVoxelBlockAsset* Asset = CastChecked<UVoxelBlockAsset>(Object);
	OutMaterialOverrides = Asset->GetPreviewMaterials();
	return Asset->GetPreviewMesh();
}