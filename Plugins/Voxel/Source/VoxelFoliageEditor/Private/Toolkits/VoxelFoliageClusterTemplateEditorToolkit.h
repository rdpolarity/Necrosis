// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFoliageClusterTemplate.h"
#include "Toolkits/VoxelSimpleAssetEditorToolkit.h"

class SVoxelFoliageClusterEntriesList;

enum class EPreviewInstancesCount : uint8
{
	Min,
	Random,
	Max
};
ENUM_RANGE_BY_FIRST_AND_LAST(EPreviewInstancesCount, EPreviewInstancesCount::Min, EPreviewInstancesCount::Max);

class FVoxelFoliageClusterTemplateEditorToolkit : public TVoxelSimpleAssetEditorToolkit<UVoxelFoliageClusterTemplate>
{
public:
	FVoxelFoliageClusterTemplateEditorToolkit() = default;

	//~ Begin FVoxelSimpleAssetEditorToolkit Interface
	virtual void BindToolkitCommands() override;
	virtual void CreateInternalWidgets() override;
	virtual TSharedRef<FTabManager::FLayout> GetLayout() const override;
	virtual void RegisterTabs(FRegisterTab RegisterTab) override;

	virtual void UpdatePreview() override;
	virtual void PostChange() override;

	virtual void PopulateToolBar(const TSharedPtr<SHorizontalBox>& ToolbarBox, const TSharedPtr<SViewportToolBar>& ParentToolBarPtr) override;

	virtual FRotator GetInitialViewRotation() const override { return FRotator(0, -90, 0); }
	//~ End FVoxelSimpleAssetEditorToolkit Interface

private:
	static const FName EntriesTabId;

	TSharedPtr<SVoxelFoliageClusterEntriesList> EntriesList;

	TArray<TWeakObjectPtr<UInstancedStaticMeshComponent>> MeshComponents;
	int32 PreviewSeed = 0;

	EPreviewInstancesCount PreviewInstancesCount = EPreviewInstancesCount::Random;
};