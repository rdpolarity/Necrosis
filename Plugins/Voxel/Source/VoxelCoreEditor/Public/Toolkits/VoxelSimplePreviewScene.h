// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "AdvancedPreviewScene.h"

class VOXELCOREEDITOR_API FVoxelSimplePreviewScene : public FAdvancedPreviewScene
{
public:
	using FAdvancedPreviewScene::FAdvancedPreviewScene;

	virtual void AddComponent(UActorComponent* Component, const FTransform& LocalToWorld = FTransform::Identity, bool bAttachToRoot = false) override;
	virtual void RemoveComponent(UActorComponent* Component) override;

	FBox GetComponentsBounds() const;

private:
	TSet<UActorComponent*> VoxelComponents;
};