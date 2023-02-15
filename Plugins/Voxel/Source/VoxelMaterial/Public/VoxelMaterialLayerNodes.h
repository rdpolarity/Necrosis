// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMaterialLayer.h"
#include "Nodes/VoxelDetailTextureNodes.h"
#include "VoxelMaterialLayerNodes.generated.h"

USTRUCT(Category = "Detail Textures")
struct VOXELMATERIAL_API FVoxelNode_MakeMaterialLayerDetailTexture : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_CALL_PARAM(TSharedPtr<uint8>, OutClass);

	VOXEL_INPUT_PIN(FVoxelMaterialLayerBuffer, Layer, nullptr);
	VOXEL_INPUT_PIN(FName, Name, "MaterialLayer");
	VOXEL_OUTPUT_PIN(FVoxelDetailTexture, DetailTexture);
};

USTRUCT(Category = "Material")
struct VOXELMATERIAL_API FVoxelNode_CreateLayeredMaterial : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_INPUT_PIN(int32, TextureSizeBias, 2);
	VOXEL_INPUT_PIN(FVoxelMaterialLayerBuffer, Layer, nullptr);
	VOXEL_INPUT_PIN_ARRAY(FVoxelDetailTexture, DetailTextures, nullptr, 1);
	VOXEL_OUTPUT_PIN(FVoxelMeshMaterial, Material);
};