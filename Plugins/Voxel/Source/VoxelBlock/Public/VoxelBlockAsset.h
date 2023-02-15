// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBlockTypes.h"
#include "VoxelBlockAsset.generated.h"

USTRUCT(BlueprintType)
struct VOXELBLOCK_API FVoxelBlockFaceTextures
{
	GENERATED_BODY()
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	TObjectPtr<UTexture2D> Top;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	TObjectPtr<UTexture2D> Bottom;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	TObjectPtr<UTexture2D> Front;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	TObjectPtr<UTexture2D> Back;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	TObjectPtr<UTexture2D> Left;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	TObjectPtr<UTexture2D> Right;

	TObjectPtr<UTexture2D> GetTexture(EVoxelBlockFace Face) const
	{
		switch (Face)
		{
		default: ensure(false);
		case EVoxelBlockFace::Back: return Back;
		case EVoxelBlockFace::Front: return Front;
		case EVoxelBlockFace::Left: return Left;
		case EVoxelBlockFace::Right: return Right;
		case EVoxelBlockFace::Bottom: return Bottom;
		case EVoxelBlockFace::Top: return Top;
		}
	}
};

UCLASS(BlueprintType, Blueprintable, meta = (VoxelAssetType, AssetColor=LightBlue))
class VOXELBLOCK_API UVoxelBlockAsset : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	bool bIsAir = false;

	// If true, neighbors will render their faces
	// Useful for glass/leaves
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	bool bIsMasked = false;

	// If true, will render faces between blocks of the same type
	// If false, will only render faces between blocks of different types
	// In minecraft, this is true for leaves but false for glass
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel", meta = (EditCondition = bIsMasked))
	bool bAlwaysRenderInnerFaces = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel", meta = (EditCondition = bIsMasked))
	bool bIsTwoSided = false;
	
	// Needs to have a "MaterialId" scalar parameter
	UPROPERTY(EditDefaultsOnly, Category = "Voxel")
	TObjectPtr<UMaterialInterface> PreviewMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Voxel")
	int32 TextureSize = 16;

	// If true, will automatically set texture compression to UserInterface2D
	UPROPERTY(EditDefaultsOnly, Category = "Voxel")
	bool bAutomaticallyFixTextures = true;

	UPROPERTY(EditDefaultsOnly, Category = "Voxel")
	bool bDisableTextureFiltering = true;
	
	//~ Begin UObject Interface
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface
public:
	static constexpr const TCHAR* PrimaryAssetType = TEXT("VoxelBlockAsset");

public:
#if WITH_EDITOR
	static UStaticMesh* GetPreviewMesh();
	TArray<UMaterialInterface*> GetPreviewMaterials();
	void ClearPreviewMaterials();
#endif

private:
#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> PreviewMaterials;
	
	UPROPERTY(Transient)
	bool bDirtyPreviewMaterials = true;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UTexture2D>> Textures;
	
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UTexture2DArray>> TextureArrays;
#endif
};