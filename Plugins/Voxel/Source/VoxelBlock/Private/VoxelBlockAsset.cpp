// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelBlockAsset.h"
#include "VoxelBlockUtilities.h"

DEFINE_VOXEL_BLUEPRINT_FACTORY(UVoxelBlockAsset);

FPrimaryAssetId UVoxelBlockAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}

#if WITH_EDITOR
void UVoxelBlockAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	bDirtyPreviewMaterials = true;
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
UStaticMesh* UVoxelBlockAsset::GetPreviewMesh()
{
	return LoadObject<UStaticMesh>(nullptr, TEXT("StaticMesh'/VoxelPlugin/EditorAssets/SM_CubeWithFaces.SM_CubeWithFaces'"));
}

TArray<UMaterialInterface*> UVoxelBlockAsset::GetPreviewMaterials()
{
	if (PreviewMaterials.Num() == 6 && 
		!bDirtyPreviewMaterials && 
		!PreviewMaterials.Contains(nullptr))
	{
		return TArray<UMaterialInterface*>(PreviewMaterials);
	}
	
	PreviewMaterials.SetNum(6);
	bDirtyPreviewMaterials = false;

	TMap<FVoxelBlockId, int32> ScalarIds;
	TMap<FVoxelBlockId, FVoxelBlockFaceIds> FacesIds;
	FVoxelBlockUtilities::BuildTextures({ { FVoxelBlockId(0), this } }, ScalarIds, FacesIds, Textures, TextureArrays);
	ensure(FacesIds.Num() == 0 || FacesIds.Num() == 1);
	ensure(ScalarIds.Num() == 0 || ScalarIds.Num() == 1);

	for (int32 Index = 0; Index < 6; Index++)
	{
		TObjectPtr<UMaterialInstanceDynamic>& Instance = PreviewMaterials[Index];
		if (!Instance)
		{
			Instance = UMaterialInstanceDynamic::Create(PreviewMaterial, this);
		}
		if (Instance)
		{
			FVoxelBlockUtilities::SetupInstance(Instance, Textures, TextureArrays);
			Instance->SetScalarParameterValue("FaceId", FacesIds.FindRef(FVoxelBlockId(0)).GetId(EVoxelBlockFace(Index)));
			Instance->SetScalarParameterValue("ScalarId", ScalarIds.FindRef(FVoxelBlockId(0)));
		}
	}
	
	return TArray<UMaterialInterface*>(PreviewMaterials);
}

void UVoxelBlockAsset::ClearPreviewMaterials()
{
	PreviewMaterials.Reset();
}
#endif