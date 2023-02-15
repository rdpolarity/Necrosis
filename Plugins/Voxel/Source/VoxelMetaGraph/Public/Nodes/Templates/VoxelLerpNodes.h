// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Nodes/Templates/VoxelTemplateNode.h"
#include "VoxelLerpNodes.generated.h"

USTRUCT(meta = (Abstract))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_LerpBase : public FVoxelTemplateNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_GENERIC_INPUT_PIN(A);
	VOXEL_GENERIC_INPUT_PIN(B);
	VOXEL_GENERIC_INPUT_PIN(Alpha);
	VOXEL_GENERIC_OUTPUT_PIN(Result);

public:
	virtual bool IsPureNode() const override
	{
		return true;
	}

	virtual FPin* ExpandPins(FNode& Node, TArray<FPin*> Pins, const TArray<FPin*>& AllPins) const override;

	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;

	virtual UScriptStruct* GetInnerNode() const VOXEL_PURE_VIRTUAL({});
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_Lerp : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, A, nullptr);
	VOXEL_MATH_INPUT_PIN(float, B, nullptr);
	VOXEL_MATH_INPUT_PIN(float, Alpha, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = lerp({A}, {B}, {Alpha})";
	}
};

USTRUCT(Category = "Math|Misc")
struct VOXELMETAGRAPH_API FVoxelTemplateNode_Lerp : public FVoxelTemplateNode_LerpBase
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetInnerNode() const override
	{
		return FVoxelNode_Lerp::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_SafeLerp : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, A, nullptr);
	VOXEL_MATH_INPUT_PIN(float, B, nullptr);
	VOXEL_MATH_INPUT_PIN(float, Alpha, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = lerp({A}, {B}, clamp({Alpha}, 0.f, 1.f))";
	}
};

USTRUCT(Category = "Math|Misc")
struct VOXELMETAGRAPH_API FVoxelTemplateNode_SafeLerp : public FVoxelTemplateNode_LerpBase
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetInnerNode() const override
	{
		return FVoxelNode_SafeLerp::StaticStruct();
	}
};