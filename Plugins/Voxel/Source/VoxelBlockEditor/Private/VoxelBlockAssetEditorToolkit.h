// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBlockAsset.h"
#include "Toolkits/VoxelSimpleAssetEditorToolkit.h"

class FVoxelBlockAssetEditorToolkit : public TVoxelSimpleAssetEditorToolkit<UVoxelBlockAsset>
{
public:
	FVoxelBlockAssetEditorToolkit() = default;

	//~ Begin FVoxelSimpleAssetEditorToolkit Interface
	virtual bool ShowFloor() const override { return false; }
	virtual void PostChange() override;
	virtual void SetupPreview() override;
	virtual void UpdatePreview() override;
	virtual FRotator GetInitialViewRotation() const override;
	virtual TOptional<float> GetInitialViewDistance() const override { return 256; }
	//~ End FVoxelSimpleAssetEditorToolkit Interface

private:
	TWeakObjectPtr<UStaticMeshComponent> StaticMeshComponent;
};