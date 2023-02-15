// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Collision/VoxelFoliageCollision.h"
#include "VoxelFoliageComponentPool.h"
#include "Collision/VoxelFoliageCollisionComponent.h"

VOXEL_CONSOLE_VARIABLE(
	VOXELFOLIAGE_API, int32, GVoxelFoliageMaxCollisionInstances, 10 * 1000 * 1000,
	"voxel.foliage.MaxCollisionInstances",
	"");

DEFINE_UNIQUE_VOXEL_ID(FVoxelFoliageCollisionId);
DEFINE_VOXEL_SUBSYSTEM(FVoxelFoliageCollision);

FVoxelFoliageCollisionId FVoxelFoliageCollision::CreateMesh(
	const FVector3d& Position,
	UStaticMesh* Mesh,
	const FBodyInstance& BodyInstance,
	const TSharedRef<const FVoxelFoliageData>& FoliageData)
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelFoliageCollisionId Id = FVoxelFoliageCollisionId::New();

	TWeakObjectPtr<UVoxelFoliageCollisionComponent>& Component = Components.Add(Id);
	Component = GetSubsystem<FVoxelFoliageComponentPool>().CreateCollisionComponent(Position);
	Component->SetStaticMesh(Mesh);
	Component->BodyInstance.UseExternalCollisionProfile(Component->GetBodySetup());
	Component->BodyInstance.CopyRuntimeBodyInstancePropertiesFrom(&BodyInstance);
	Component->BodyInstance.SetObjectType(BodyInstance.GetObjectType());

	if (NumInstances + FoliageData->Transforms->Num() > GVoxelFoliageMaxCollisionInstances)
	{
		VOXEL_MESSAGE(Error, "More than {0} foliage instances being rendered, aborting", GVoxelFoliageMaxCollisionInstances);
	}
	else
	{
		NumInstances += FoliageData->Transforms->Num();
		Component->AssignInstances(*FoliageData->Transforms);
	}

	Component->CreateAllInstanceBodies();

	return Id;
}

void FVoxelFoliageCollision::DestroyMesh(FVoxelFoliageCollisionId Id)
{
	TWeakObjectPtr<UVoxelFoliageCollisionComponent> Component;
	if (IsDestroyed() ||
		!ensure(Components.RemoveAndCopyValue(Id, Component)) ||
		!Component.IsValid())
	{
		return;
	}

	NumInstances -= Component->GetInstancesCount();
	ensure(NumInstances >= 0);

	GetSubsystem<FVoxelFoliageComponentPool>().DestroyCollisionComponent(Component.Get());
}