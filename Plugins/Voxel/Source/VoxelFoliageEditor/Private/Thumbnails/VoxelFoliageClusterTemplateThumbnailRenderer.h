// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "ThumbnailHelpers.h"
#include "VoxelFoliageClusterTemplateThumbnailRenderer.generated.h"

class UVoxelFoliageClusterTemplate;
class FVoxelFoliageClusterTemplateThumbnailScene;

UCLASS()
class VOXELFOLIAGEEDITOR_API UVoxelFoliageClusterTemplateThumbnailRenderer : public UVoxelThumbnailRenderer
{
	GENERATED_BODY()

public:
	//~ Begin UVoxelCustomThumbnailRenderer Interface
	virtual TSharedPtr<FThumbnailPreviewScene> CreateScene() override;
	virtual void InitializeScene(UObject* Object) override;
	virtual void ClearScene(UObject* Object) override;
	//~ End UVoxelCustomThumbnailRenderer Interface
};

class FVoxelFoliageClusterTemplateThumbnailScene : public FVoxelThumbnailScene
{
public:
	void SetClusterTemplate(const UVoxelFoliageClusterTemplate* ClusterTemplate);

protected:
	//~ Begin FVoxelThumbnailScene Interface
	virtual FBoxSphereBounds GetBounds() const override;
	virtual float GetBoundsScale() const override;
	//~ End FVoxelThumbnailScene Interface

private:
	TArray<TWeakObjectPtr<UInstancedStaticMeshComponent>> MeshComponents;
};