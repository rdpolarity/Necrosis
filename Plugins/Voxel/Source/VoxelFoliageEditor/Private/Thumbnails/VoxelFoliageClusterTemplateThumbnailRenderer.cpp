// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelFoliageClusterTemplateThumbnailRenderer.h"
#include "VoxelFoliageEditorUtilities.h"
#include "VoxelFoliageClusterTemplate.h"

DEFINE_VOXEL_THUMBNAIL_RENDERER(UVoxelFoliageClusterTemplateThumbnailRenderer, UVoxelFoliageClusterTemplate);

TSharedPtr<FThumbnailPreviewScene> UVoxelFoliageClusterTemplateThumbnailRenderer::CreateScene()
{
	return MakeShared<FVoxelFoliageClusterTemplateThumbnailScene>();
}

void UVoxelFoliageClusterTemplateThumbnailRenderer::InitializeScene(UObject* Object)
{
	const UVoxelFoliageClusterTemplate* ClusterTemplate = Cast<UVoxelFoliageClusterTemplate>(Object);
	if (!IsValid(ClusterTemplate))
	{
		return;
	}

	const TSharedRef<FVoxelFoliageClusterTemplateThumbnailScene> ClusterTemplateScene = GetScene<FVoxelFoliageClusterTemplateThumbnailScene>();

	ClusterTemplateScene->SetClusterTemplate(ClusterTemplate);
}

void UVoxelFoliageClusterTemplateThumbnailRenderer::ClearScene(UObject* Object)
{
	const TSharedRef<FVoxelFoliageClusterTemplateThumbnailScene> ClusterTemplateScene = GetScene<FVoxelFoliageClusterTemplateThumbnailScene>();

	ClusterTemplateScene->SetClusterTemplate(nullptr);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelFoliageClusterTemplateThumbnailScene::SetClusterTemplate(const UVoxelFoliageClusterTemplate* ClusterTemplate)
{
	if (!ClusterTemplate)
	{
		for (const TWeakObjectPtr<UInstancedStaticMeshComponent>& WeakComponent : MeshComponents)
		{
			UInstancedStaticMeshComponent* Component = WeakComponent.Get();
			if (!Component)
			{
				continue;
			}

			Component->SetStaticMesh(nullptr);
			Component->RecreateRenderState_Concurrent();
		}
		return;
	}

	FVoxelFoliageEditorUtilities::FClusterRenderData Data;
	FVoxelFoliageEditorUtilities::RenderClusterTemplate(*ClusterTemplate, *this, Data, MeshComponents);

	for (const TWeakObjectPtr<UInstancedStaticMeshComponent>& WeakComponent : MeshComponents)
	{
		UInstancedStaticMeshComponent* Component = WeakComponent.Get();
		if (!Component)
		{
			continue;
		}

		Component->ForcedLodModel = 1;
		Component->UpdateBounds();

		Component->RecreateRenderState_Concurrent();
	}
}

FBoxSphereBounds FVoxelFoliageClusterTemplateThumbnailScene::GetBounds() const
{
	FBox Bounds(ForceInit);
	for (const TWeakObjectPtr<UStaticMeshComponent> Component : MeshComponents)
	{
		if (!Component.IsValid())
		{
			continue;
		}

		if (const USceneComponent* SceneComponent = Cast<USceneComponent>(Component))
		{
			Bounds += SceneComponent->CalcBounds(SceneComponent->GetComponentToWorld()).GetBox();
		}
	}

	return FBoxSphereBounds(Bounds);
}

float FVoxelFoliageClusterTemplateThumbnailScene::GetBoundsScale() const
{
	return 0.8f;
}