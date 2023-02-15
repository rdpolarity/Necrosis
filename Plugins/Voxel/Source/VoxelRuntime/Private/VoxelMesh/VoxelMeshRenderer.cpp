// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMesh/VoxelMeshRenderer.h"
#include "VoxelMesh/VoxelMeshComponent.h"
#include "VoxelMesh/VoxelMeshComponentPool.h"

DEFINE_UNIQUE_VOXEL_ID(FVoxelMeshRendererId);
DEFINE_VOXEL_SUBSYSTEM(FVoxelMeshRenderer);

FVoxelMeshRendererId FVoxelMeshRenderer::CreateMesh(
	const FVector3d& Position,
	const TSharedRef<const FVoxelMesh>& Mesh)
{
	const FVoxelMeshRendererId Id = FVoxelMeshRendererId::New();

	TWeakObjectPtr<UVoxelMeshComponent>& Component = Components.Add(Id);
	Component = GetSubsystem<FVoxelMeshComponentPool>().CreateMesh(Position);
	Component->SetMesh(Mesh);

	return Id;
}

void FVoxelMeshRenderer::DestroyMesh(FVoxelMeshRendererId Id)
{
	TWeakObjectPtr<UVoxelMeshComponent> Component;
	if (IsDestroyed() ||
		!ensure(Components.RemoveAndCopyValue(Id, Component)) ||
		!Component.IsValid())
	{
		return;
	}

	GetSubsystem<FVoxelMeshComponentPool>().DestroyMesh(Component.Get());
}