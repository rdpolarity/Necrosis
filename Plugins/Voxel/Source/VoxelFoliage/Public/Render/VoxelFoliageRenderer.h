// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFoliage.h"
#include "VoxelRuntime/VoxelSubsystem.h"
#include "VoxelFoliageRenderer.generated.h"

class UVoxelFoliageComponent;

DECLARE_UNIQUE_VOXEL_ID(FVoxelFoliageRendererId);

UCLASS()
class VOXELFOLIAGE_API UVoxelFoliageRendererProxy : public UVoxelSubsystemProxy
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(FVoxelFoliageRenderer);
};

class VOXELFOLIAGE_API FVoxelFoliageRenderer : public IVoxelSubsystem
{
public:
	GENERATED_VOXEL_SUBSYSTEM_BODY(UVoxelFoliageRendererProxy);

	FVoxelFoliageRendererId CreateMesh(
		const FVector3d& Position,
		UStaticMesh* Mesh,
		const FVoxelFoliageSettings& FoliageSettings,
		const TSharedRef<const FVoxelFoliageData>& FoliageData);

	void DestroyMesh(FVoxelFoliageRendererId Id);

private:
	int32 NumInstances = 0;
	TMap<FVoxelFoliageRendererId, TWeakObjectPtr<UVoxelFoliageComponent>> Components;
};