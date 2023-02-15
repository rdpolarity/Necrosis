// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMaterialLayerAssetEditorToolkit.h"

DEFINE_VOXEL_TOOLKIT(FVoxelMaterialLayerAssetEditorToolkit);

void FVoxelMaterialLayerAssetEditorToolkit::PostChange()
{
	// Raise the errors right away, otherwise they're raised by the thumbnail preview
	(void)GetAsset().GetPreviewMaterial(true);
}

void FVoxelMaterialLayerAssetEditorToolkit::SetupPreview()
{
	StaticMeshComponent = NewObject<UStaticMeshComponent>();
	StaticMeshComponent->SetStaticMesh(UVoxelMaterialLayerAsset::GetPreviewMesh());
	GetPreviewScene().AddComponent(StaticMeshComponent.Get());

	// Force a refresh in case the materials are out of date
	(void)GetAsset().GetPreviewMaterial(true);
}

void FVoxelMaterialLayerAssetEditorToolkit::UpdatePreview()
{
	StaticMeshComponent->SetMaterial(0, GetAsset().GetPreviewMaterial());
}

FRotator FVoxelMaterialLayerAssetEditorToolkit::GetInitialViewRotation() const
{
	return FRotator(-20.0f, 180 + 45.0f, 0.f);
}