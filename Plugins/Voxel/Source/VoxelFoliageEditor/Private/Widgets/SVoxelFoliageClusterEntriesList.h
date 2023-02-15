// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"

class UVoxelFoliageClusterEntry;
class UVoxelFoliageClusterTemplate;
class UVoxelFoliageInstanceTemplate;
class FVoxelSimpleAssetEditorToolkit;
struct FVoxelFoliageClusterEntryWrapper;

class SVoxelFoliageClusterEntriesList : public SCompoundWidget
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_ARGUMENT(TWeakObjectPtr<UVoxelFoliageClusterTemplate>, Asset)
		SLATE_ARGUMENT(TWeakPtr<FVoxelSimpleAssetEditorToolkit>, Toolkit)
	};

	void Construct(const FArguments& InArgs);

public:
	UVoxelFoliageClusterEntry* GetSelectedEntry() const;

	void DeleteSelectedEntry();
	void DuplicateSelectedEntry();

	void PostChange();

private:
	TSharedRef<ITableRow> MakeWidgetFromOption(TSharedPtr<FVoxelFoliageClusterEntryWrapper> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedPtr<SWidget> OnContextMenuOpening();

	void AddEntry(const UVoxelFoliageClusterEntry* EntryToCopy, UVoxelFoliageInstanceTemplate* DefaultTemplate);

private:
	TSharedPtr<IDetailsView> EntryDetailsView;
	TSharedPtr<SListView<TSharedPtr<FVoxelFoliageClusterEntryWrapper>>> EntriesListView;

private:
	TArray<TSharedPtr<FVoxelFoliageClusterEntryWrapper>> EntriesList;
	TWeakObjectPtr<UVoxelFoliageClusterTemplate> WeakCluster;
	TWeakPtr<FVoxelSimpleAssetEditorToolkit> WeakToolkit;
};