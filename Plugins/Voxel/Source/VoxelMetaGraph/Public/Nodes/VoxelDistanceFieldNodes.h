// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelDistanceFieldNodes.generated.h"

struct FVoxelNode_MakeDistanceField;

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelDistanceField : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	bool bIsHeightmap = false;
	FVoxelBox Bounds;
	float Smoothness = 0.f;
	bool bIsSubtractive = false;
	int32 Priority = 0;

	virtual TVoxelFutureValue<FVoxelFloatBuffer> GetDistances(const FVoxelQuery& Query) const VOXEL_PURE_VIRTUAL({});
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelDistanceFieldArray : public FVoxelDistanceField
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	TArray<TSharedRef<const FVoxelDistanceField>> DistanceFields;
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelDistanceFieldCustom : public FVoxelDistanceField
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	TVoxelArray<TSharedPtr<const FVoxelDistanceField>> DistanceFields;

	const FVoxelNode_MakeDistanceField* Node = nullptr;
	TSharedPtr<IVoxelNodeOuter> Outer;

	virtual TVoxelFutureValue<FVoxelFloatBuffer> GetDistances(const FVoxelQuery& Query) const override;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(Category = "Distance Field")
struct VOXELMETAGRAPH_API FVoxelNode_MakeDistanceField : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Density, nullptr, DensityPin);
	VOXEL_INPUT_PIN(float, Smoothness, 0.f);
	VOXEL_INPUT_PIN(bool, Subtractive, false);
	VOXEL_INPUT_PIN(int32, Priority, 0);
	VOXEL_OUTPUT_PIN(FVoxelDistanceField, DistanceField);
};

USTRUCT(Category = "Distance Field")
struct FVoxelNode_MakeDistanceFieldFromBrushes : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_INPUT_PIN(FName, LayerName, "Main");
	VOXEL_OUTPUT_PIN(FVoxelDistanceField, DistanceField);
};

USTRUCT(Category = "Distance Field")
struct VOXELMETAGRAPH_API FVoxelNode_CombineDistanceFields : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_INPUT_PIN_ARRAY(FVoxelDistanceField, DistanceFields, nullptr, 1);
	VOXEL_OUTPUT_PIN(FVoxelDistanceField, DistanceField);
};

USTRUCT(Category = "Distance Field")
struct VOXELMETAGRAPH_API FVoxelNode_ComputeDensityFromDistanceField : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_INPUT_PIN(FVoxelDistanceField, DistanceField, nullptr);
	VOXEL_INPUT_PIN(float, MinExactDistance, 0.f);
	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Density, DensityPin);
};