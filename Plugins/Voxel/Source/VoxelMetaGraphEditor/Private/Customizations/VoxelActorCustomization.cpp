// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "VoxelActor.h"

VOXEL_CUSTOMIZE_CLASS(AVoxelActor)(IDetailLayoutBuilder& DetailLayout)
{
	VOXEL_FUNCTION_COUNTER_LLM();

	FVoxelEditorUtilities::EnableRealtime();
}