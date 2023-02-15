// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMaterialLayerAsset.generated.h"

UCLASS(BlueprintType, Blueprintable, meta = (VoxelAssetType, AssetColor=LightBlue))
class VOXELMATERIAL_API UVoxelMaterialLayerAsset : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Voxel")
	TObjectPtr<UMaterialInterface> Material;
	
	//~ Begin UObject Interface
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	//~ End UObject Interface
public:
	static constexpr const TCHAR* PrimaryAssetType = TEXT("VoxelMaterialLayerAsset");

	static UStaticMesh* GetPreviewMesh();
	UMaterialInterface* GetPreviewMaterial(bool bRebuild = false) const
	{
		// TODO
		return PreviewMaterial;
	}

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PreviewMaterial;
};