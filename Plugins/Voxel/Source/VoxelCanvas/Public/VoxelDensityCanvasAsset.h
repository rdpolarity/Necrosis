// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelDensityCanvasData.h"
#include "VoxelDensityCanvasAsset.generated.h"

UCLASS(BlueprintType, meta = (VoxelAssetType, AssetSubMenu=Canvas, AssetColor=Red))
class VOXELCANVAS_API UVoxelDensityCanvasAsset : public UObject
{
	GENERATED_BODY()
	VOXEL_USE_NAMESPACE_TYPES(DensityCanvas, FData);

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	float VoxelSize = 100.f;

	UPROPERTY(EditAnywhere, Category = "Config")
	bool bCompress = false;

	// Clears all stored data
	UFUNCTION(BlueprintCallable, Category = "Voxel|Canvas")
	void ClearData();
	
	UFUNCTION(BlueprintCallable, Category = "Voxel|Canvas")
	void AddSphere(
		FTransform VoxelActorTransform,
		FVector Position,
		float Radius = 500.f);
	
	UFUNCTION(BlueprintCallable, Category = "Voxel|Canvas")
	void RemoveSphere(
		FTransform VoxelActorTransform,
		FVector Position,
		float Radius = 500.f);
	
	UFUNCTION(BlueprintCallable, Category = "Voxel|Canvas")
	void SmoothAdd(
		FTransform VoxelActorTransform,
		FVector Position,
		float Radius = 500.f,
		float Falloff = 0.1f,
		float Strength = 0.1f);
	
	UFUNCTION(BlueprintCallable, Category = "Voxel|Canvas")
	void SmoothRemove(
		FTransform VoxelActorTransform,
		FVector Position,
		float Radius = 500.f,
		float Falloff = 0.1f,
		float Strength = 0.1f);

	UFUNCTION(BlueprintCallable, Category = "Voxel|Canvas")
	static UVoxelDensityCanvasAsset* MakeDensityCanvasAsset(float CanvasVoxelSize = 100.f);

public:
	//~ Begin UObject Interface
	virtual void PostLoad() override;
	virtual void Serialize(FArchive& Ar) override;
	//~ End UObject Interface

	TSharedRef<FData> GetData() const;

private:
	FByteBulkData BulkData;
	TSharedPtr<FData> Data;
};