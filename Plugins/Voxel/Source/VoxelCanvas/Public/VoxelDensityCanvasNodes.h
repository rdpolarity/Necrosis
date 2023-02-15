// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelExposedPinType.h"
#include "VoxelDensityCanvasData.h"
#include "VoxelDensityCanvasAsset.h"
#include "VoxelDensityCanvasNodes.generated.h"

USTRUCT()
struct VOXELCANVAS_API FVoxelDensityCanvasDataPinType : public FVoxelExposedPinType
{
	GENERATED_BODY()

	DEFINE_VOXEL_EXPOSED_PIN_TYPE(FVoxelDensityCanvasData, TSoftObjectPtr<UVoxelDensityCanvasAsset>)
	{
		const UVoxelDensityCanvasAsset* Asset = Value.LoadSynchronous();
		if (!Asset)
		{
			return {};
		}

		return MakeSharedCopy(FVoxelDensityCanvasData{ Asset->GetData() });
	}
};

USTRUCT(Category = "Canvas")
struct VOXELCANVAS_API FVoxelNode_ApplyDensityCanvas : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, InDensity, nullptr, DensityPin);
	VOXEL_INPUT_PIN(FVoxelDensityCanvasData, Canvas, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, OutDensity, DensityPin);
};