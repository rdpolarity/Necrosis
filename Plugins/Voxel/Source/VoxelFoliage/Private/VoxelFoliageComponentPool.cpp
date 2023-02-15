// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelFoliageComponentPool.h"
#include "Render/VoxelFoliageComponent.h"
#include "Collision/VoxelFoliageCollisionComponent.h"
#include "VoxelComponentSubsystem.h"

DEFINE_VOXEL_SUBSYSTEM(FVoxelFoliageComponentPool);

UVoxelFoliageComponent* FVoxelFoliageComponentPool::CreateComponent(const FVector3d& Position)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	UVoxelFoliageComponent* NewComponent = nullptr;
	while (ComponentPool.Num() > 0 && !NewComponent)
	{
		NewComponent = ComponentPool.Pop(false).Get();
		ensure(NewComponent != nullptr);
	}

	FVoxelComponentSubsystem& ComponentSubsystem = GetSubsystem<FVoxelComponentSubsystem>();

	if (!NewComponent)
	{
		NewComponent = ComponentSubsystem.CreateComponent<UVoxelFoliageComponent>();
		if (!ensure(NewComponent))
		{
			return nullptr;
		}
		
		ComponentSubsystem.SetupSceneComponent(*NewComponent);
		NewComponent->RegisterComponent();
	}
	
	ComponentSubsystem.SetComponentPosition(*NewComponent, Position);

	return NewComponent;
}

void FVoxelFoliageComponentPool::DestroyComponent(UVoxelFoliageComponent* Component)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	if (!ensure(Component))
	{
		return;
	}

	Component->ClearInstances();
	Component->SetStaticMesh(nullptr);

	ComponentPool.Add(Component);
}

UVoxelFoliageCollisionComponent* FVoxelFoliageComponentPool::CreateCollisionComponent(const FVector3d& Position)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	UVoxelFoliageCollisionComponent* NewComponent = nullptr;
	while (CollisionComponentPool.Num() > 0 && !NewComponent)
	{
		NewComponent = CollisionComponentPool.Pop(false).Get();
		ensure(NewComponent != nullptr);
	}

	FVoxelComponentSubsystem& ComponentSubsystem = GetSubsystem<FVoxelComponentSubsystem>();

	if (!NewComponent)
	{
		NewComponent = ComponentSubsystem.CreateComponent<UVoxelFoliageCollisionComponent>();
		if (!ensure(NewComponent))
		{
			return nullptr;
		}
		
		ComponentSubsystem.SetupSceneComponent(*NewComponent);
		NewComponent->RegisterComponent();
	}
	
	ComponentSubsystem.SetComponentPosition(*NewComponent, Position);

	return NewComponent;
}

void FVoxelFoliageComponentPool::DestroyCollisionComponent(UVoxelFoliageCollisionComponent* Component)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	if (!ensure(Component))
	{
		return;
	}

	Component->ClearAllInstanceBodies();
	Component->SetStaticMesh(nullptr);

	CollisionComponentPool.Add(Component);
}