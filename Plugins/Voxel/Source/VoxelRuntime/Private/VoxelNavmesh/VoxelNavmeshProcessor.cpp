// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelNavmesh/VoxelNavmeshProcessor.h"
#include "VoxelNavmesh/VoxelNavmeshComponent.h"
#include "VoxelNavmesh/VoxelNavmeshComponentPool.h"

DEFINE_UNIQUE_VOXEL_ID(FVoxelNavmeshProcessorId);
DEFINE_VOXEL_SUBSYSTEM(FVoxelNavmeshProcessor);

FVoxelNavmeshProcessorId FVoxelNavmeshProcessor::CreateNavmesh(
	const FVector3d& Position,
	const TSharedRef<const FVoxelNavmesh>& NavigationMesh)
{
	const FVoxelNavmeshProcessorId Id = FVoxelNavmeshProcessorId::New();

	TWeakObjectPtr<UVoxelNavmeshComponent>& Component = Components.Add(Id);
	Component = GetSubsystem<FVoxelNavmeshComponentPool>().CreateComponent(Position);
	Component->SetNavigationMesh(NavigationMesh);

	return Id;
}

void FVoxelNavmeshProcessor::DestroyNavmesh(FVoxelNavmeshProcessorId Id)
{
	TWeakObjectPtr<UVoxelNavmeshComponent> Component;
	if (IsDestroyed() ||
		!ensure(Components.RemoveAndCopyValue(Id, Component)) ||
		!Component.IsValid())
	{
		return;
	}

	GetSubsystem<FVoxelNavmeshComponentPool>().DestroyComponent(Component.Get());
}