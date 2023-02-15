// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMesh/VoxelMeshComponentPool.h"
#include "VoxelMesh/VoxelMeshComponent.h"
#include "VoxelComponentSubsystem.h"

DEFINE_VOXEL_SUBSYSTEM(FVoxelMeshComponentPool);

UVoxelMeshComponent* FVoxelMeshComponentPool::CreateMesh(const FVector3d& Position)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	UVoxelMeshComponent* NewMesh = nullptr;
	while (MeshPool.Num() > 0 && !NewMesh)
	{
		NewMesh = MeshPool.Pop(false).Get();
		ensure(NewMesh != nullptr);
	}

	FVoxelComponentSubsystem& ComponentSubsystem = GetSubsystem<FVoxelComponentSubsystem>();

	if (!NewMesh)
	{
		NewMesh = ComponentSubsystem.CreateComponent<UVoxelMeshComponent>();
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

void FVoxelMeshComponentPool::DestroyMesh(UVoxelMeshComponent* Mesh)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	if (!ensure(Mesh))
	{
		return;
	}

	Mesh->SetMesh({});

	MeshPool.Add(Mesh);
}