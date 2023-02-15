// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Engine/AssetUserData.h"
#include "VoxelMeshVoxelizerLibrary.h"
#include "VoxelLandmassMeshBrush.generated.h"

class FVoxelLandmassBrushData;

UCLASS(BlueprintType, meta = (VoxelAssetType, AssetColor=Red))
class VOXELLANDMASS_API UVoxelLandmassMeshBrush : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Voxel", AssetRegistrySearchable)
	TSoftObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, Category = "Voxel")
	float VoxelSizeMultiplier = 1.f;

	// Relative to the size
	// Bigger = higher memory usage but more accurate when using smooth min
	UPROPERTY(EditAnywhere, Category = "Voxel", meta = (UIMin = 0, UIMax = 1))
	float MaxSmoothness = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Voxel", meta = (ShowOnlyInnerProperties))
	FVoxelMeshVoxelizerSettings VoxelizerSettings;

	float GetVoxelSize() const;
	TSharedPtr<FVoxelLandmassBrushData> GetBrushData();

public:
#if WITH_EDITOR
	static void OnReimport();
#endif

public:
	//~ Begin UObject Interface
	virtual void Serialize(FArchive& Ar) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface

private:
#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Voxel", AdvancedDisplay)
	FIntVector DataSize;

	UPROPERTY(VisibleAnywhere, Category = "Voxel")
	float MemorySizeInMB = 0.f;
#endif

private:
	TSharedPtr<FVoxelLandmassBrushData> BrushData;

	void TryCreateBrushData();
};

UCLASS()
class VOXELLANDMASS_API UVoxelLandmassMeshBrushAssetUserData : public UAssetUserData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Voxel")
	TSoftObjectPtr<UVoxelLandmassMeshBrush> Brush;
	
	static UVoxelLandmassMeshBrush* GetBrush(UStaticMesh& Mesh);
};