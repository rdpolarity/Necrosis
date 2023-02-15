// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelAdvancedNoiseNode.generated.h"

UENUM(BlueprintType, DisplayName = "Advanced Noise Octave Type")
enum class EVoxelAdvancedNoiseOctaveType : uint8
{
	SmoothPerlin UMETA(ToolTip = "Perlin2D"),
	BillowyPerlin UMETA(ToolTip = "abs(Perlin2D) * 2 - 1"),
	RidgedPerlin UMETA(ToolTip = "(1 - abs(Perlin2D)) * 2 - 1"),

	SmoothCellular UMETA(ToolTip = "Cellular2D"),
	BillowyCellular UMETA(ToolTip = "abs(Cellular2D) * 2 - 1"),
	RidgedCellular UMETA(ToolTip = "(1 - abs(Cellular2D)) * 2 - 1"),
};

USTRUCT(Category = "Noise")
struct VOXELMETAGRAPH_API FVoxelNode_AdvancedNoise2D : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	VOXEL_INPUT_PIN(FVoxelVector2DBuffer, Position, nullptr);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Amplitude, 10000);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, FeatureScale, 100000);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Lacunarity, 2.f);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Gain, 0.5f);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, CellularJitter, 0.9f);
	VOXEL_INPUT_PIN(int32, NumOctaves, 10);
	VOXEL_INPUT_PIN(int32, Seed, nullptr, SeedPin);
	VOXEL_INPUT_PIN(uint8, DefaultOctaveType, nullptr, EnumPin<EVoxelAdvancedNoiseOctaveType>, PropertyBind);
	VOXEL_INPUT_PIN_ARRAY(uint8, OctaveType, nullptr, 1, EnumPin<EVoxelAdvancedNoiseOctaveType>);
	VOXEL_INPUT_PIN_ARRAY(FVoxelFloatBuffer, OctaveStrength, 1.f, 1);

	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Value);
};