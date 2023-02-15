// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Nodes/Templates/VoxelTemplateNode.h"
#include "VoxelClampNodes.generated.h"

USTRUCT(meta = (Abstract))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_AbstractClampBase : public FVoxelTemplateNode
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

public:
	virtual UScriptStruct* GetFloatInnerNode() const
	{
		return nullptr;
	}
	virtual UScriptStruct* GetInt32InnerNode() const
	{
		return nullptr;
	}
};

USTRUCT(meta = (Abstract))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_ClampBase : public FVoxelTemplateNode_AbstractClampBase
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_GENERIC_INPUT_PIN(Min);
	VOXEL_GENERIC_INPUT_PIN(Max);
};

USTRUCT(meta = (Abstract))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_MappedClampBase : public FVoxelTemplateNode_AbstractClampBase
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_GENERIC_INPUT_PIN(InMin);
	VOXEL_GENERIC_INPUT_PIN(InMax);
	VOXEL_GENERIC_INPUT_PIN(OutMin);
	VOXEL_GENERIC_INPUT_PIN(OutMax);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_Clamp : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_INPUT_PIN(float, Min, 0.0f);
	VOXEL_MATH_INPUT_PIN(float, Max, 1.0f);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = clamp({Value}, {Min}, {Max})";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_Clamp_Int : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, Value, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, Min, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, Max, nullptr);
	VOXEL_MATH_OUTPUT_PIN(int32, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = clamp({Value}, {Min}, {Max})";
	}
};

USTRUCT(Category = "Math|Misc")
struct VOXELMETAGRAPH_API FVoxelTemplateNode_Clamp : public FVoxelTemplateNode_ClampBase
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetFloatInnerNode() const override
	{
		return FVoxelNode_Clamp::StaticStruct();
	}
	virtual UScriptStruct* GetInt32InnerNode() const override
	{
		return FVoxelNode_Clamp_Int::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_MappedRangeValueClamped : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_INPUT_PIN(float, InMin, nullptr);
	VOXEL_MATH_INPUT_PIN(float, InMax, nullptr);
	VOXEL_MATH_INPUT_PIN(float, OutMin, nullptr);
	VOXEL_MATH_INPUT_PIN(float, OutMax, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = lerp({OutMin}, {OutMax}, clamp(({Value} - {InMin}) / ({InMax} - {InMin}), 0.f, 1.f))";
	}
};

USTRUCT(Category = "Math|Misc")
struct VOXELMETAGRAPH_API FVoxelTemplateNode_MappedRangeValueClamped : public FVoxelTemplateNode_MappedClampBase
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetFloatInnerNode() const override
	{
		return FVoxelNode_MappedRangeValueClamped::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_MappedRangeValueUnclamped : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_INPUT_PIN(float, InMin, nullptr);
	VOXEL_MATH_INPUT_PIN(float, InMax, nullptr);
	VOXEL_MATH_INPUT_PIN(float, OutMin, nullptr);
	VOXEL_MATH_INPUT_PIN(float, OutMax, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = lerp({OutMin}, {OutMax}, ({Value} - {InMin}) / ({InMax} - {InMin}))";
	}
};

USTRUCT(Category = "Math|Misc")
struct VOXELMETAGRAPH_API FVoxelTemplateNode_MappedRangeValueUnclamped : public FVoxelTemplateNode_MappedClampBase
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetFloatInnerNode() const override
	{
		return FVoxelNode_MappedRangeValueUnclamped::StaticStruct();
	}
};