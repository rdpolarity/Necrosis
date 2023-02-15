// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"

class UVoxelFoliageMesh_New;
class UVoxelFoliageInstanceTemplate;
class FVoxelSimpleAssetEditorToolkit;
struct FVoxelFoliageMeshWrapper;

class SVoxelFoliageInstanceMeshesList : public SCompoundWidget
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_ARGUMENT(TWeakObjectPtr<UVoxelFoliageInstanceTemplate>, Asset)
		SLATE_ARGUMENT(TWeakPtr<FVoxelSimpleAssetEditorToolkit>, Toolkit)
	};

	void Construct(const FArguments& InArgs);

public:
	UVoxelFoliageMesh_New* GetSelectedMesh() const;

	void DeleteSelectedMesh();
	void DuplicateSelectedMesh();

	void PostChange();

private:
	TSharedRef<ITableRow> MakeWidgetFromOption(TSharedPtr<FVoxelFoliageMeshWrapper> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedPtr<SWidget> OnContextMenuOpening();

	void AddMesh(const UVoxelFoliageMesh_New* FoliageMeshToCopy, UStaticMesh* DefaultMesh);

private:
	TSharedPtr<IDetailsView> MeshDetailsView;
	TSharedPtr<SListView<TSharedPtr<FVoxelFoliageMeshWrapper>>> MeshesListView;

private:
	TArray<TSharedPtr<FVoxelFoliageMeshWrapper>> MeshesList;
	TWeakObjectPtr<UVoxelFoliageInstanceTemplate> WeakInstance;
	TWeakPtr<FVoxelSimpleAssetEditorToolkit> WeakToolkit;
};
