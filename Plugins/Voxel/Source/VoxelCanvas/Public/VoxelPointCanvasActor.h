// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelPointCanvasBrush.h"
#include "VoxelPointCanvasActor.generated.h"

DECLARE_UNIQUE_VOXEL_ID(FVoxelLandmassBrushId);

UCLASS()
class VOXELCANVAS_API UVoxelPointCanvasComponent final : public UPrimitiveComponent
{
	GENERATED_BODY()
	
public:
	//~ Begin UPrimitiveComponent Interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual bool ShouldCreatePhysicsState() const override { return false; }
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ End UPrimitiveComponent Interface.
};

UCLASS(meta = (VoxelPlaceableItem, PlaceableSubMenu = "Landmass"))
class VOXELCANVAS_API AVoxelPointCanvasActor : public AVoxelBrushActor
{
	GENERATED_BODY()

public:
	AVoxelPointCanvasActor();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config", meta = (ShowOnlyInnerProperties))
	FVoxelPointCanvasBrush Brush;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config")
	bool bDisplayPoints = false;

private:
	UPROPERTY()
	TObjectPtr<UVoxelPointCanvasComponent> CanvasComponent = nullptr;
};