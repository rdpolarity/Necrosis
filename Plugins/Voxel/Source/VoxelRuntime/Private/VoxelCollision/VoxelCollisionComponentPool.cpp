// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelCollision/VoxelCollisionComponentPool.h"
#include "VoxelCollision/VoxelCollisionComponent.h"
#include "VoxelComponentSubsystem.h"

DEFINE_VOXEL_SUBSYSTEM(FVoxelCollisionComponentPool);

UVoxelCollisionComponent* FVoxelCollisionComponentPool::CreateComponent(const FVector3d& Position)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	UVoxelCollisionComponent* NewMesh = nullptr;
	while (MeshPool.Num() > 0 && !NewMesh)
	{
		NewMesh = MeshPool.Pop(false).Get();
		ensure(NewMesh != nullptr);
	}

	FVoxelComponentSubsystem& ComponentSubsystem = GetSubsystem<FVoxelComponentSubsystem>();

	if (!NewMesh)
	{
		NewMesh = ComponentSubsystem.CreateComponent<UVoxelCollisionComponent>();
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

void FVoxelCollisionComponentPool::DestroyComponent(UVoxelCollisionComponent* Mesh)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	if (!ensure(Mesh))
	{
		return;
	}

	Mesh->SetCollider({});

	MeshPool.Add(Mesh);
}