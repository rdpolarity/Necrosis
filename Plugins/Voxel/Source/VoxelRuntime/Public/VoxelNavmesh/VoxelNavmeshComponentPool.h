// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelRuntime/VoxelSubsystem.h"
#include "VoxelNavmeshComponentPool.generated.h"

class UVoxelNavmeshComponent;

UCLASS()
class VOXELRUNTIME_API UVoxelNavmeshComponentPoolProxy : public UVoxelSubsystemProxy
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(FVoxelNavmeshComponentPool);
};

class VOXELRUNTIME_API FVoxelNavmeshComponentPool : public IVoxelSubsystem
{
public:
	GENERATED_VOXEL_SUBSYSTEM_BODY(UVoxelNavmeshComponentPoolProxy);

	UVoxelNavmeshComponent* CreateComponent(const FVector3d& Position);
	void DestroyComponent(UVoxelNavmeshComponent* Mesh);

private:
	TArray<TWeakObjectPtr<UVoxelNavmeshComponent>> MeshPool;
};