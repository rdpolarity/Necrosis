// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Toolkits/VoxelSimplePreviewScene.h"

void FVoxelSimplePreviewScene::AddComponent(UActorComponent* Component, const FTransform& LocalToWorld, bool bAttachToRoot)
{
	FAdvancedPreviewScene::AddComponent(Component, LocalToWorld, bAttachToRoot);

	VoxelComponents.Add(Component);
}

void FVoxelSimplePreviewScene::RemoveComponent(UActorComponent* Component)
{
	FAdvancedPreviewScene::RemoveComponent(Component);

	VoxelComponents.Remove(Component);
}

FBox FVoxelSimplePreviewScene::GetComponentsBounds() const
{
	FBox Bounds(ForceInit);
	for (const UActorComponent* Component : VoxelComponents)
	{
		if (const USceneComponent* SceneComponent = Cast<USceneComponent>(Component))
		{
			// Force a CalcBounds for ISMs when there hasn't been any tick yet
			Bounds += SceneComponent->CalcBounds(SceneComponent->GetComponentToWorld()).GetBox();
		}
	}
	return Bounds;
}