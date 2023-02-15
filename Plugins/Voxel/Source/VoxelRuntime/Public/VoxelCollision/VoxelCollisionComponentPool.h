// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelRuntime/VoxelSubsystem.h"
#include "VoxelCollisionComponentPool.generated.h"

class UVoxelCollisionComponent;

UCLASS()
class VOXELRUNTIME_API UVoxelCollisionComponentPoolProxy : public UVoxelSubsystemProxy
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(FVoxelCollisionComponentPool);
};

class VOXELRUNTIME_API FVoxelCollisionComponentPool : public IVoxelSubsystem
{
public:
	GENERATED_VOXEL_SUBSYSTEM_BODY(UVoxelCollisionComponentPoolProxy);

	UVoxelCollisionComponent* CreateComponent(const FVector3d& Position);
	void DestroyComponent(UVoxelCollisionComponent* Mesh);

private:
	TArray<TWeakObjectPtr<UVoxelCollisionComponent>> MeshPool;
};