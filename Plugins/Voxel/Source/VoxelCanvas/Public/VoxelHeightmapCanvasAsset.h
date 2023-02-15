// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelHeightmapCanvasData.h"
#include "VoxelHeightmapCanvasAsset.generated.h"

UENUM(BlueprintType)
enum class EVoxelHeightmapCanvasPaintBehavior : uint8
{
	Overwrite,
	Min,
	Max
};

UCLASS(BlueprintType, meta = (VoxelAssetType, AssetSubMenu=Canvas, AssetColor=Red))
class VOXELCANVAS_API UVoxelHeightmapCanvasAsset : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	FIntPoint Size = FIntPoint(256, 256);

	UPROPERTY(EditAnywhere, Category = "Config")
	bool bCompress = false;

	//~ Begin UObject Interface
	virtual void Serialize(FArchive& Ar) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface

public:
	UFUNCTION(BlueprintCallable, Category = "Voxel|Canvas")
	void PaintCircle(
		FVector2D Center, 
		float Radius = 10.f, 
		float Falloff = 0.5f,
		EVoxelFalloff FalloffType = EVoxelFalloff::Smooth,
		float Strength = 0.1f, 
		float Value = 100.f,
		EVoxelHeightmapCanvasPaintBehavior Behavior = EVoxelHeightmapCanvasPaintBehavior::Overwrite);

	UFUNCTION(BlueprintCallable, Category = "Voxel|Canvas")
	void PaintSquare(
		FVector2D Center,
		float Extent = 10.f, 
		float Value = 100.f,
		EVoxelHeightmapCanvasPaintBehavior Behavior = EVoxelHeightmapCanvasPaintBehavior::Overwrite);

	UFUNCTION(BlueprintCallable, Category = "Voxel|Canvas")
	void PaintRamp(
		FVector2D Start,
		FVector2D End,
		float Width = 10.f,
		float StartValue = 0.f,
		float EndValue = 100.f,
		EVoxelHeightmapCanvasPaintBehavior Behavior = EVoxelHeightmapCanvasPaintBehavior::Overwrite);

public:
	TSharedPtr<FVoxelHeightmapCanvasData> GetData() const
	{
		return Data;
	}

	void Update(const FVoxelBox2D& Bounds) const;

private:
	FByteBulkData BulkData;
	TSharedPtr<FVoxelHeightmapCanvasData> Data;

	void FixupData();

	static FORCEINLINE void ApplyBehavior(float& Value, float NewValue, EVoxelHeightmapCanvasPaintBehavior Behavior)
	{
		switch (Behavior)
		{
		default: VOXEL_ASSUME(false);
		case EVoxelHeightmapCanvasPaintBehavior::Overwrite:
		{
			Value = NewValue;
		}
		break;
		case EVoxelHeightmapCanvasPaintBehavior::Min:
		{
			Value = FMath::Min(Value, NewValue);
		}
		break;
		case EVoxelHeightmapCanvasPaintBehavior::Max:
		{
			Value = FMath::Max(Value, NewValue);
		}
		break;
		}
	}
};