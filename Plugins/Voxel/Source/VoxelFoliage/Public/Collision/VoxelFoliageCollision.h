// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFoliage.h"
#include "VoxelRuntime/VoxelSubsystem.h"
#include "VoxelFoliageCollision.generated.h"

class UVoxelFoliageCollisionComponent;

DECLARE_UNIQUE_VOXEL_ID(FVoxelFoliageCollisionId);

UCLASS()
class VOXELFOLIAGE_API UVoxelFoliageCollisionProxy : public UVoxelSubsystemProxy
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(FVoxelFoliageCollision);
};

class VOXELFOLIAGE_API FVoxelFoliageCollision : public IVoxelSubsystem
{
public:
	GENERATED_VOXEL_SUBSYSTEM_BODY(UVoxelFoliageCollisionProxy);

	FVoxelFoliageCollisionId CreateMesh(
		const FVector3d& Position,
		UStaticMesh* Mesh,
		const FBodyInstance& BodyInstance,
		const TSharedRef<const FVoxelFoliageData>& FoliageData);

	void DestroyMesh(FVoxelFoliageCollisionId Id);

private:
	int32 NumInstances = 0;
	TMap<FVoxelFoliageCollisionId, TWeakObjectPtr<UVoxelFoliageCollisionComponent>> Components;
};