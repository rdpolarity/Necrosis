// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFoliageInstanceTemplate.h"
#include "Toolkits/VoxelSimpleAssetEditorToolkit.h"

class SVoxelFoliageInstanceMeshesList;

class FVoxelFoliageInstanceTemplateAssetEditorToolkit : public TVoxelSimpleAssetEditorToolkit<UVoxelFoliageInstanceTemplate>
{
public:
	FVoxelFoliageInstanceTemplateAssetEditorToolkit() = default;

	//~ Begin FVoxelSimpleAssetEditorToolkit Interface
	virtual void BindToolkitCommands() override;
	virtual void CreateInternalWidgets() override;
	virtual TSharedRef<FTabManager::FLayout> GetLayout() const override;
	virtual void RegisterTabs(FRegisterTab RegisterTab) override;

	virtual void UpdatePreview() override;
	virtual void DrawPreviewCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas) override;
	virtual void PostChange() override;

	virtual FRotator GetInitialViewRotation() const override { return FRotator(0, -90, 0); }
	//~ End FVoxelSimpleAssetEditorToolkit Interface

private:
	static const FName MeshesTabId;

	TSharedPtr<SVoxelFoliageInstanceMeshesList> MeshesList;

	TArray<TWeakObjectPtr<UStaticMeshComponent>> StaticMeshComponents;
};
