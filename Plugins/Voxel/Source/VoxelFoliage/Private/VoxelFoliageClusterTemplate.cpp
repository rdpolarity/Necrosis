// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelFoliageClusterTemplate.h"

DEFINE_VOXEL_FACTORY(UVoxelFoliageClusterTemplate);

UStaticMesh* UVoxelFoliageClusterEntry::GetFirstInstanceMesh() const
{
	if (!Instance)
	{
		return nullptr;
	}

	if (Instance->Meshes.Num() == 0)
	{
		return nullptr;
	}

	for (const UVoxelFoliageMesh_New* FoliageMesh : Instance->Meshes)
	{
		if (FoliageMesh->StaticMesh)
		{
			return FoliageMesh->StaticMesh;
		}
	}

	return nullptr;
}