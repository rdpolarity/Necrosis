// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLandmassHeightmapBrush.h"
#include "VoxelLandmassHeightmapActor.generated.h"

class UVoxelMeshComponent;
struct FVoxelLandmassHeightmapPreviewMesh;

UCLASS(meta = (VoxelPlaceableItem, PlaceableSubMenu = "Landmass"))
class VOXELLANDMASS_API AVoxelLandmassHeightmapActor : public AVoxelBrushActor
{
	GENERATED_BODY()

public:
	AVoxelLandmassHeightmapActor();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config", meta = (ShowOnlyInnerProperties))
	FVoxelLandmassHeightmapBrush Brush;

public:
	UPROPERTY(Transient)
	TObjectPtr<UVoxelMeshComponent> MeshComponent;

	void UpdatePreviewMesh();
};