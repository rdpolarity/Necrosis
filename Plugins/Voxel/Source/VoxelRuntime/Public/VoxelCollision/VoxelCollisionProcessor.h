// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelRuntime/VoxelSubsystem.h"
#include "VoxelCollisionProcessor.generated.h"

struct FVoxelCollider;
class UVoxelCollisionComponent;

DECLARE_UNIQUE_VOXEL_ID(FVoxelCollisionProcessorId);

UCLASS()
class VOXELRUNTIME_API UVoxelCollisionProcessorProxy : public UVoxelSubsystemProxy
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(FVoxelCollisionProcessor);
};

class VOXELRUNTIME_API FVoxelCollisionProcessor : public IVoxelSubsystem
{
public:
	GENERATED_VOXEL_SUBSYSTEM_BODY(UVoxelCollisionProcessorProxy);

	FVoxelCollisionProcessorId CreateCollision(
		const FVector3d& Position,
		const FBodyInstance& BodyInstance,
		const TSharedRef<const FVoxelCollider>& Collider);

	void DestroyCollision(FVoxelCollisionProcessorId Id);

private:
	TMap<FVoxelCollisionProcessorId, TWeakObjectPtr<UVoxelCollisionComponent>> Components;
};