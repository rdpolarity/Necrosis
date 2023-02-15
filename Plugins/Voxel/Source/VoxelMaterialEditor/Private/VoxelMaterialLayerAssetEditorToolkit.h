// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMaterialLayerAsset.h"
#include "Toolkits/VoxelSimpleAssetEditorToolkit.h"

class FVoxelMaterialLayerAssetEditorToolkit : public TVoxelSimpleAssetEditorToolkit<UVoxelMaterialLayerAsset>
{
public:
	FVoxelMaterialLayerAssetEditorToolkit() = default;

	//~ Begin FVoxelSimpleAssetEditorToolkit Interface
	virtual bool ShowFloor() const override { return false; }
	virtual void PostChange() override;
	virtual void SetupPreview() override;
	virtual void UpdatePreview() override;
	virtual FRotator GetInitialViewRotation() const override;
	virtual TOptional<float> GetInitialViewDistance() const override { return 500; }
	//~ End FVoxelSimpleAssetEditorToolkit Interface

private:
	TWeakObjectPtr<UStaticMeshComponent> StaticMeshComponent;
};