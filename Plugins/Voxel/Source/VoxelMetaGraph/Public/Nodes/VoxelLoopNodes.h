// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelLoopNodes.generated.h"

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelWhileLoopQueryData : public FVoxelQueryData
{
	GENERATED_BODY()
	GENERATED_VOXEL_QUERY_DATA_BODY()
		
	int32 Index = 0;
	FVoxelSharedPinValue Value;

	uint64 GetHash() const
	{
		return FVoxelUtilities::MurmurHash(Index) ^ Value.GetHash();
	}
	bool Identical(const FVoxelWhileLoopQueryData & Other) const
	{
		return
			Index == Other.Index &&
			Value == Other.Value;
	}
};

USTRUCT(Category = "Flow Control")
struct VOXELMETAGRAPH_API FVoxelNode_GetWhileLoopIndex : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_OUTPUT_PIN(int32, Index);
};

USTRUCT(Category = "Flow Control")
struct VOXELMETAGRAPH_API FVoxelNode_GetWhileLoopPreviousValue : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_GENERIC_OUTPUT_PIN(Value);

	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;
};

USTRUCT(Category = "Flow Control")
struct VOXELMETAGRAPH_API FVoxelNode_WhileLoop : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_GENERIC_INPUT_PIN(FirstValue);
	VOXEL_GENERIC_INPUT_PIN(NextValue);
	VOXEL_INPUT_PIN(bool, Continue, false);

	VOXEL_GENERIC_OUTPUT_PIN(Result);

	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;
};