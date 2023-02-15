// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelExposedPinType.h"
#include "VoxelMaterialLayerAsset.h"
#include "VoxelMaterialLayer.generated.h"

USTRUCT(DisplayName = "Material Layer")
struct VOXELMATERIAL_API FVoxelMaterialLayer
{
	GENERATED_BODY()

	uint8 Class = 0;
	uint8 Layer = 0;
};

USTRUCT()
struct VOXELMATERIAL_API FVoxelMaterialLayerPinType : public FVoxelExposedPinType
{
	GENERATED_BODY()

	DEFINE_VOXEL_EXPOSED_PIN_TYPE(FVoxelMaterialLayer, TSoftObjectPtr<UVoxelMaterialLayerAsset>);
};

DECLARE_VOXEL_TERMINAL_BUFFER(FVoxelMaterialLayerBufferView, FVoxelMaterialLayerBuffer, FVoxelMaterialLayer, PF_R8G8_UINT);

USTRUCT()
struct VOXELMATERIAL_API FVoxelMaterialLayerBufferView : public FVoxelTerminalBufferView
{
	GENERATED_BODY()
	GENERATED_VOXEL_TERMINAL_BUFFER_VIEW_BODY(FVoxelMaterialLayer);
};

USTRUCT(DisplayName = "Material Layer Buffer")
struct VOXELMATERIAL_API FVoxelMaterialLayerBuffer : public FVoxelTerminalBuffer
{
	GENERATED_BODY()
	GENERATED_VOXEL_TERMINAL_BUFFER_BODY(FVoxelMaterialLayer);
};