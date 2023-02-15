// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMesh/VoxelMesh.h"

DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelMeshMemory);

FVoxelMesh::~FVoxelMesh()
{
	ensure(bIsDestroyed_RenderThread || !bIsInitialized_RenderThread);
}

void FVoxelMesh::CallInitialize_GameThread() const
{
	check(IsInGameThread());

	if (bIsInitialized_GameThread)
	{
		return;
	}
	bIsInitialized_GameThread = true;

	VOXEL_CONST_CAST(this)->Initialize_GameThread();
}

void FVoxelMesh::CallInitialize_RenderThread(ERHIFeatureLevel::Type FeatureLevel) const
{
	check(IsInRenderingThread());

	if (bIsInitialized_RenderThread)
	{
		return;
	}
	bIsInitialized_RenderThread = true;

	VOXEL_CONST_CAST(this)->Initialize_RenderThread(FeatureLevel);
}

TSharedRef<FVoxelMaterialRef> FVoxelMesh::GetMaterialSafe() const
{
	TSharedPtr<FVoxelMaterialRef> Material = GetMaterial();
	if (!Material)
	{
		Material = FVoxelMaterialRef::Default();
	}
	return Material.ToSharedRef();
}