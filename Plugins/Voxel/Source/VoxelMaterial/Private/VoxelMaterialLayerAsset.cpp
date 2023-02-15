// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMaterialLayerAsset.h"

DEFINE_VOXEL_BLUEPRINT_FACTORY(UVoxelMaterialLayerAsset);

FPrimaryAssetId UVoxelMaterialLayerAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}

UStaticMesh* UVoxelMaterialLayerAsset::GetPreviewMesh()
{
	return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/EditorMeshes/EditorSphere.EditorSphere"));
}