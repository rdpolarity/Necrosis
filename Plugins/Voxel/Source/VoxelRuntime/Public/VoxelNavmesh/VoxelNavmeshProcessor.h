// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelRuntime/VoxelSubsystem.h"
#include "VoxelNavmeshProcessor.generated.h"

struct FVoxelNavmesh;
class UVoxelNavmeshComponent;

DECLARE_UNIQUE_VOXEL_ID(FVoxelNavmeshProcessorId);

UCLASS()
class VOXELRUNTIME_API UVoxelNavmeshProcessorProxy : public UVoxelSubsystemProxy
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(FVoxelNavmeshProcessor);
};

class VOXELRUNTIME_API FVoxelNavmeshProcessor : public IVoxelSubsystem
{
public:
	GENERATED_VOXEL_SUBSYSTEM_BODY(UVoxelNavmeshProcessorProxy);

	FVoxelNavmeshProcessorId CreateNavmesh(
		const FVector3d& Position,
		const TSharedRef<const FVoxelNavmesh>& Navmesh);

	void DestroyNavmesh(FVoxelNavmeshProcessorId Id);

private:
	TMap<FVoxelNavmeshProcessorId, TWeakObjectPtr<UVoxelNavmeshComponent>> Components;
};