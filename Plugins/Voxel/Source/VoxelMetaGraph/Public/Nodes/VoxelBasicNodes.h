// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelBasicNodes.generated.h"

USTRUCT(meta = (Internal))
struct VOXELMETAGRAPH_API FVoxelNode_ToArray : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_GENERIC_INPUT_PIN(Value);
	VOXEL_GENERIC_OUTPUT_PIN(OutValue);

	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;
};

USTRUCT(Category = "Misc")
struct VOXELMETAGRAPH_API FVoxelNode_GetLOD : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_OUTPUT_PIN(int32, LOD);

	virtual bool IsPureNode() const override
	{
		return true;
	}
};

USTRUCT(Category = "Misc")
struct VOXELMETAGRAPH_API FVoxelNode_IsInBounds : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelVectorBuffer, Position, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelBoolBuffer, Result);
};

USTRUCT(Category = "Random")
struct VOXELMETAGRAPH_API FVoxelNode_RandFloat : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelVectorBuffer, Position, nullptr);
	VOXEL_INPUT_PIN(int32, Seed, nullptr, SeedPin);
	VOXEL_INPUT_PIN(float, Min, 0.f);
	VOXEL_INPUT_PIN(float, Max, 1.f);
	VOXEL_INPUT_PIN(int32, RoundingDecimals, 3);

	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Result);
};

USTRUCT(Category = "Random")
struct VOXELMETAGRAPH_API FVoxelNode_RandUnitVector : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelVectorBuffer, Position, nullptr);
	VOXEL_INPUT_PIN(int32, Seed, nullptr, SeedPin);
	VOXEL_INPUT_PIN(int32, RoundingDecimals, 3);
	VOXEL_OUTPUT_PIN(FVoxelVectorBuffer, Result);
};

USTRUCT(Category = "Misc")
struct VOXELMETAGRAPH_API FVoxelNode_RunOnCPU : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_GENERIC_INPUT_PIN(Data);
	VOXEL_GENERIC_OUTPUT_PIN(OutData);

	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;
};

USTRUCT(Category = "Misc")
struct VOXELMETAGRAPH_API FVoxelNode_RunOnGPU : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_GENERIC_INPUT_PIN(Data);
	VOXEL_GENERIC_OUTPUT_PIN(OutData);

	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;
};