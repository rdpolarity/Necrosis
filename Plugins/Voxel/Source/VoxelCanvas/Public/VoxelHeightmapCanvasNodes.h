// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelHeightmapCanvasNodes.generated.h"

USTRUCT(DisplayName = "Heightmap Canvas")
struct VOXELCANVAS_API FVoxelHeightmapCanvas
{
	GENERATED_BODY()

	FIntPoint Size = FIntPoint::ZeroValue;
	TVoxelArray<float> Heightmap;

	void Initialize(const FIntPoint& NewSize)
	{
		Size = NewSize;
		FVoxelUtilities::SetNumFast(Heightmap, NewSize.X * NewSize.Y);
	}

	FVoxelVectorBuffer GeneratePositionBuffer() const;
};

USTRUCT(Category = "Heightmap Canvas")
struct VOXELCANVAS_API FVoxelNode_GetBaseHeightmapCanvas : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_OUTPUT_PIN(FVoxelHeightmapCanvas, Canvas);
};

USTRUCT(Category = "Heightmap Canvas")
struct VOXELCANVAS_API FVoxelNode_DisplaceHeightmapCanvas : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_INPUT_PIN(FVoxelHeightmapCanvas, Canvas, nullptr);
	VOXEL_INPUT_PIN(FVoxelVector2DBuffer, Offset, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelHeightmapCanvas, OutCanvas);
};

USTRUCT(Category = "Heightmap Canvas")
struct VOXELCANVAS_API FVoxelNode_BlurHeightmapCanvas : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_INPUT_PIN(FVoxelHeightmapCanvas, Canvas, nullptr);
	VOXEL_INPUT_PIN(float, Strength, 1.f);
	VOXEL_OUTPUT_PIN(FVoxelHeightmapCanvas, OutCanvas);
};

USTRUCT(Category = "Heightmap Canvas")
struct VOXELCANVAS_API FVoxelNode_UpscaleHeightmapCanvas2x : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_INPUT_PIN(FVoxelHeightmapCanvas, Canvas, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelHeightmapCanvas, OutCanvas);
};