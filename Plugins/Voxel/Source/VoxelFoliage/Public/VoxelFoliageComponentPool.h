// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelRuntime/VoxelSubsystem.h"
#include "VoxelFoliageComponentPool.generated.h"

class UVoxelFoliageComponent;
class UVoxelFoliageCollisionComponent;

UCLASS()
class VOXELFOLIAGE_API UVoxelFoliageComponentPoolProxy : public UVoxelSubsystemProxy
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(FVoxelFoliageComponentPool);
};

class VOXELFOLIAGE_API FVoxelFoliageComponentPool : public IVoxelSubsystem
{
public:
	GENERATED_VOXEL_SUBSYSTEM_BODY(UVoxelFoliageComponentPoolProxy);

	UVoxelFoliageComponent* CreateComponent(const FVector3d& Position);
	void DestroyComponent(UVoxelFoliageComponent* Component);

	UVoxelFoliageCollisionComponent* CreateCollisionComponent(const FVector3d& Position);
	void DestroyCollisionComponent(UVoxelFoliageCollisionComponent* Component);

private:
	TArray<TWeakObjectPtr<UVoxelFoliageComponent>> ComponentPool;
	TArray<TWeakObjectPtr<UVoxelFoliageCollisionComponent>> CollisionComponentPool;
};