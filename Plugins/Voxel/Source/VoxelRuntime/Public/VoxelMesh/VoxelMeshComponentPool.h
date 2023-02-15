// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelRuntime/VoxelSubsystem.h"
#include "VoxelMeshComponentPool.generated.h"

class UVoxelMeshComponent;

UCLASS()
class VOXELRUNTIME_API UVoxelMeshComponentPoolProxy : public UVoxelSubsystemProxy
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(FVoxelMeshComponentPool);
};

class VOXELRUNTIME_API FVoxelMeshComponentPool : public IVoxelSubsystem
{
public:
	GENERATED_VOXEL_SUBSYSTEM_BODY(UVoxelMeshComponentPoolProxy);
	
	UVoxelMeshComponent* CreateMesh(const FVector3d& Position);
	void DestroyMesh(UVoxelMeshComponent* Mesh);

private:
	TArray<TWeakObjectPtr<UVoxelMeshComponent>> MeshPool;
};