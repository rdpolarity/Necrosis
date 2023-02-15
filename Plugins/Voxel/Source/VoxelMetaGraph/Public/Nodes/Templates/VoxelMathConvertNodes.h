// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Nodes/Templates/VoxelTemplateNode.h"
#include "VoxelMathConvertNodes.generated.h"

USTRUCT(meta = (Abstract))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_AbstractMathConvert : public FVoxelTemplateNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_GENERIC_INPUT_PIN(Value);
	VOXEL_GENERIC_OUTPUT_PIN(Result);

public:
	virtual bool IsPureNode() const override
	{
		return true;
	}

	virtual FPin* ExpandPins(FNode& Node, TArray<FPin*> Pins, const TArray<FPin*>& AllPins) const override;

	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;

	virtual UScriptStruct* GetInt32InnerNode() const VOXEL_PURE_VIRTUAL({});
	virtual UScriptStruct* GetFloatInnerNode() const VOXEL_PURE_VIRTUAL({});
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_Ceil : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(int32, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = (int)ceil({Value})";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_CeilToFloat : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = ceil({Value})";
	}
};

USTRUCT(Category = "Math|Operators")
struct VOXELMETAGRAPH_API FVoxelTemplateNode_Ceil : public FVoxelTemplateNode_AbstractMathConvert
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetInt32InnerNode() const override
	{
		return FVoxelNode_Ceil::StaticStruct();
	}
	virtual UScriptStruct* GetFloatInnerNode() const override
	{
		return FVoxelNode_CeilToFloat::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_Round : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(int32, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = (int)round({Value})";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_RoundToFloat : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = round({Value})";
	}
};

USTRUCT(Category = "Math|Operators")
struct VOXELMETAGRAPH_API FVoxelTemplateNode_Round : public FVoxelTemplateNode_AbstractMathConvert
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetInt32InnerNode() const override
	{
		return FVoxelNode_Round::StaticStruct();
	}
	virtual UScriptStruct* GetFloatInnerNode() const override
	{
		return FVoxelNode_RoundToFloat::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_Floor : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(int32, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = (int)floor({Value})";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_FloorToFloat : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = floor({Value})";
	}
};

USTRUCT(Category = "Math|Operators")
struct VOXELMETAGRAPH_API FVoxelTemplateNode_Floor : public FVoxelTemplateNode_AbstractMathConvert
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetInt32InnerNode() const override
	{
		return FVoxelNode_Floor::StaticStruct();
	}
	virtual UScriptStruct* GetFloatInnerNode() const override
	{
		return FVoxelNode_FloorToFloat::StaticStruct();
	}
};