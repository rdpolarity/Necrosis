// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelBlockAssetEditorToolkit.h"

DEFINE_VOXEL_TOOLKIT(FVoxelBlockAssetEditorToolkit);

void FVoxelBlockAssetEditorToolkit::PostChange()
{
	// Raise the errors right away, otherwise they're raised by the thumbnail preview
	GetAsset().GetPreviewMaterials();
}

void FVoxelBlockAssetEditorToolkit::SetupPreview()
{
	StaticMeshComponent = NewObject<UStaticMeshComponent>();
	StaticMeshComponent->SetStaticMesh(UVoxelBlockAsset::GetPreviewMesh());
	GetPreviewScene().AddComponent(StaticMeshComponent.Get());

	// Force a refresh in case the materials are out of date
	GetAsset().ClearPreviewMaterials();
}

void FVoxelBlockAssetEditorToolkit::UpdatePreview()
{
	const TArray<UMaterialInterface*> Materials = GetAsset().GetPreviewMaterials();
	for (int32 Index = 0; Index < Materials.Num(); Index++)
	{
		StaticMeshComponent->SetMaterial(Index, Materials[Index]);
	}
}

FRotator FVoxelBlockAssetEditorToolkit::GetInitialViewRotation() const
{
	return FRotator(-20.0f, 180 + 45.0f, 0.f);
}