// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Render/VoxelFoliageRenderer.h"
#include "Render/VoxelFoliageComponent.h"
#include "VoxelFoliageComponentPool.h"

VOXEL_CONSOLE_VARIABLE(
	VOXELFOLIAGE_API, int32, GVoxelFoliageMaxInstances, 10 * 1000 * 1000,
	"voxel.foliage.MaxInstances",
	"");

DEFINE_UNIQUE_VOXEL_ID(FVoxelFoliageRendererId);
DEFINE_VOXEL_SUBSYSTEM(FVoxelFoliageRenderer);

FVoxelFoliageRendererId FVoxelFoliageRenderer::CreateMesh(
	const FVector3d& Position,
	UStaticMesh* Mesh,
	const FVoxelFoliageSettings& FoliageSettings,
	const TSharedRef<const FVoxelFoliageData>& FoliageData)
{
	VOXEL_FUNCTION_COUNTER();

	FoliageData->UpdateStats();

	const FVoxelFoliageRendererId Id = FVoxelFoliageRendererId::New();

	TWeakObjectPtr<UVoxelFoliageComponent>& Component = Components.Add(Id);
	Component = GetSubsystem<FVoxelFoliageComponentPool>().CreateComponent(Position);
	Component->SetStaticMesh(Mesh);

	FoliageSettings.ApplyToComponent(*Component);

	if (NumInstances + FoliageData->Transforms->Num() > GVoxelFoliageMaxInstances)
	{
		VOXEL_MESSAGE(Error, "More than {0} foliage instances being rendered, aborting", GVoxelFoliageMaxInstances);
	}
	else
	{
		NumInstances += FoliageData->Transforms->Num();
		Component->FoliageData = FoliageData;
		Component->StartTreeBuild();
	}

	return Id;
}

void FVoxelFoliageRenderer::DestroyMesh(FVoxelFoliageRendererId Id)
{
	TWeakObjectPtr<UVoxelFoliageComponent> Component;
	if (IsDestroyed() ||
		!ensure(Components.RemoveAndCopyValue(Id, Component)) ||
		!Component.IsValid())
	{
		return;
	}

	if (ensure(Component->FoliageData))
	{
		NumInstances -= Component->FoliageData->Transforms->Num();
		ensure(NumInstances >= 0);
	}

	GetSubsystem<FVoxelFoliageComponentPool>().DestroyComponent(Component.Get());
}