// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelNavmesh/VoxelNavmeshComponentPool.h"
#include "VoxelNavmesh/VoxelNavmeshComponent.h"
#include "VoxelComponentSubsystem.h"

DEFINE_VOXEL_SUBSYSTEM(FVoxelNavmeshComponentPool);

UVoxelNavmeshComponent* FVoxelNavmeshComponentPool::CreateComponent(const FVector3d& Position)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	UVoxelNavmeshComponent* NewMesh = nullptr;
	while (MeshPool.Num() > 0 && !NewMesh)
	{
		NewMesh = MeshPool.Pop(false).Get();
		ensure(NewMesh != nullptr);
	}

	FVoxelComponentSubsystem& ComponentSubsystem = GetSubsystem<FVoxelComponentSubsystem>();

	if (!NewMesh)
	{
		NewMesh = ComponentSubsystem.CreateComponent<UVoxelNavmeshComponent>();
		if (!ensure(NewMesh))
		{
			return nullptr;
		}
		
		ComponentSubsystem.SetupSceneComponent(*NewMesh);

		NewMesh->RegisterComponent();
	}
	
	ComponentSubsystem.SetComponentPosition(*NewMesh, Position);

	return NewMesh;
}

void FVoxelNavmeshComponentPool::DestroyComponent(UVoxelNavmeshComponent* Mesh)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	if (!ensure(Mesh))
	{
		return;
	}

	Mesh->SetNavigationMesh({});

	MeshPool.Add(Mesh);
}