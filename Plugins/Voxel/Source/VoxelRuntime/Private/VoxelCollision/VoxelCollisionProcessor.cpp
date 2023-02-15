// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelCollision/VoxelCollisionProcessor.h"
#include "VoxelCollision/VoxelCollisionComponent.h"
#include "VoxelCollision/VoxelCollisionComponentPool.h"

DEFINE_UNIQUE_VOXEL_ID(FVoxelCollisionProcessorId);
DEFINE_VOXEL_SUBSYSTEM(FVoxelCollisionProcessor);

FVoxelCollisionProcessorId FVoxelCollisionProcessor::CreateCollision(
	const FVector3d& Position,
	const FBodyInstance& BodyInstance,
	const TSharedRef<const FVoxelCollider>& Collider)
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelCollisionProcessorId Id = FVoxelCollisionProcessorId::New();

	TWeakObjectPtr<UVoxelCollisionComponent>& Component = Components.Add(Id);
	Component = GetSubsystem<FVoxelCollisionComponentPool>().CreateComponent(Position);
	Component->BodyInstance.CopyRuntimeBodyInstancePropertiesFrom(&BodyInstance);
	Component->BodyInstance.SetObjectType(BodyInstance.GetObjectType());
	Component->SetCollider(Collider);

	return Id;
}

void FVoxelCollisionProcessor::DestroyCollision(FVoxelCollisionProcessorId Id)
{
	TWeakObjectPtr<UVoxelCollisionComponent> Component;
	if (IsDestroyed() ||
		!ensure(Components.RemoveAndCopyValue(Id, Component)) ||
		!Component.IsValid())
	{
		return;
	}

	GetSubsystem<FVoxelCollisionComponentPool>().DestroyComponent(Component.Get());
}