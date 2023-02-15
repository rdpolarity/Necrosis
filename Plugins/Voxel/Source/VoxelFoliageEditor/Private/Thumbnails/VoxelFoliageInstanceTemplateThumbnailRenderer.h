// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "ThumbnailHelpers.h"
#include "VoxelFoliageInstanceTemplateThumbnailRenderer.generated.h"

class UVoxelFoliageInstanceTemplate;
class FVoxelFoliageInstanceTemplateThumbnailScene;

UCLASS()
class UVoxelFoliageInstanceTemplateThumbnailRenderer : public UVoxelThumbnailRenderer
{
	GENERATED_BODY()

public:
	//~ Begin UVoxelCustomThumbnailRenderer Interface
	virtual TSharedPtr<FThumbnailPreviewScene> CreateScene() override;
	virtual void InitializeScene(UObject* Object) override;
	virtual void ClearScene(UObject* Object) override;
	//~ End UVoxelCustomThumbnailRenderer Interface
};

class FVoxelFoliageInstanceTemplateThumbnailScene : public FVoxelThumbnailScene
{
public:
	void SetInstanceTemplate(const UVoxelFoliageInstanceTemplate* InstanceTemplate);

protected:
	//~ Begin FVoxelThumbnailScene Interface
	virtual FBoxSphereBounds GetBounds() const override;
	//~ End FVoxelThumbnailScene Interface

private:
	TArray<TWeakObjectPtr<UStaticMeshComponent>> StaticMeshComponents;
};