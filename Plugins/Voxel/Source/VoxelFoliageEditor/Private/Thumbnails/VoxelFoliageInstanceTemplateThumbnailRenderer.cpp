// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelFoliageInstanceTemplateThumbnailRenderer.h"
#include "VoxelFoliageEditorUtilities.h"
#include "VoxelFoliageInstanceTemplate.h"

DEFINE_VOXEL_THUMBNAIL_RENDERER(UVoxelFoliageInstanceTemplateThumbnailRenderer, UVoxelFoliageInstanceTemplate);

TSharedPtr<FThumbnailPreviewScene> UVoxelFoliageInstanceTemplateThumbnailRenderer::CreateScene()
{
	return MakeShared<FVoxelFoliageInstanceTemplateThumbnailScene>();
}

void UVoxelFoliageInstanceTemplateThumbnailRenderer::InitializeScene(UObject* Object)
{
	const UVoxelFoliageInstanceTemplate* InstanceTemplate = Cast<UVoxelFoliageInstanceTemplate>(Object);
	if (!IsValid(InstanceTemplate))
	{
		return;
	}

	const TSharedRef<FVoxelFoliageInstanceTemplateThumbnailScene> ClusterTemplateScene = GetScene<FVoxelFoliageInstanceTemplateThumbnailScene>();

	ClusterTemplateScene->SetInstanceTemplate(InstanceTemplate);
}

void UVoxelFoliageInstanceTemplateThumbnailRenderer::ClearScene(UObject* Object)
{
	const TSharedRef<FVoxelFoliageInstanceTemplateThumbnailScene> ClusterTemplateScene = GetScene<FVoxelFoliageInstanceTemplateThumbnailScene>();

	ClusterTemplateScene->SetInstanceTemplate(nullptr);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelFoliageInstanceTemplateThumbnailScene::SetInstanceTemplate(const UVoxelFoliageInstanceTemplate* InstanceTemplate)
{
	if (!InstanceTemplate)
	{
		for (const TWeakObjectPtr<UStaticMeshComponent>& WeakComponent : StaticMeshComponents)
		{
			UStaticMeshComponent* Component = WeakComponent.Get();
			if (!Component)
			{
				continue;
			}

			Component->SetStaticMesh(nullptr);
			Component->RecreateRenderState_Concurrent();
		}
		return;
	}

	FVoxelFoliageEditorUtilities::RenderInstanceTemplate(*InstanceTemplate, *this, StaticMeshComponents);

	for (const TWeakObjectPtr<UStaticMeshComponent>& WeakComponent : StaticMeshComponents)
	{
		UStaticMeshComponent* Component = WeakComponent.Get();
		if (!Component)
		{
			continue;
		}

		Component->ForcedLodModel = 1;
		Component->UpdateBounds();

		Component->RecreateRenderState_Concurrent();
	}
}

FBoxSphereBounds FVoxelFoliageInstanceTemplateThumbnailScene::GetBounds() const
{
	FBox Bounds(ForceInit);
	for (const TWeakObjectPtr<UStaticMeshComponent> Component : StaticMeshComponents)
	{
		if (!Component.IsValid())
		{
			continue;
		}

		if (const USceneComponent* SceneComponent = Cast<USceneComponent>(Component))
		{
			// Force a CalcBounds for ISMs when there hasn't been any tick yet
			Bounds += SceneComponent->CalcBounds(SceneComponent->GetComponentToWorld()).GetBox();
		}
	}

	return FBoxSphereBounds(Bounds);
}