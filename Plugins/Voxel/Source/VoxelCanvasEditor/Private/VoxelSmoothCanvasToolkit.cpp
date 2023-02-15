// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelSmoothCanvasToolkit.h"
#include "VoxelSmoothCanvasEdMode.h"

void FVoxelSmoothCanvasToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);
}

FName FVoxelSmoothCanvasToolkit::GetToolkitFName() const
{
	return GetDefault<UVoxelSmoothCanvasEdMode>()->GetModeInfo().ID;
}

FText FVoxelSmoothCanvasToolkit::GetBaseToolkitName() const
{
	return GetDefault<UVoxelSmoothCanvasEdMode>()->GetModeInfo().Name;
}

FText FVoxelSmoothCanvasToolkit::GetActiveToolDisplayName() const 
{
	return VOXEL_LOCTEXT("FVoxelSmoothCanvasToolkit::GetActiveToolDisplayName");
}

FText FVoxelSmoothCanvasToolkit::GetActiveToolMessage() const 
{ 
	return VOXEL_LOCTEXT("FVoxelSmoothCanvasToolkit::GetActiveToolMessage");
}