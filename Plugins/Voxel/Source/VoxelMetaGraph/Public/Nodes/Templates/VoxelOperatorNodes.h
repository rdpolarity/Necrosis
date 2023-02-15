// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Nodes/Templates/VoxelTemplateNode.h"
#include "VoxelOperatorNodes.generated.h"

USTRUCT(meta = (Abstract))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_OperatorBase : public FVoxelTemplateNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	virtual bool IsPureNode() const override
	{
		return true;
	}

	virtual FPin* ExpandPins(FNode& Node, TArray<FPin*> Pins, const TArray<FPin*>& AllPins) const override;

	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;

	virtual UScriptStruct* GetFloatInnerNode() const VOXEL_PURE_VIRTUAL({});
	virtual UScriptStruct* GetInt32InnerNode() const VOXEL_PURE_VIRTUAL({});

public:
	void FixupPinTypes(const FVoxelPin* SelectedPin);
};

USTRUCT(meta = (Abstract))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_UnaryOperator : public FVoxelTemplateNode_OperatorBase
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_GENERIC_INPUT_PIN(Value);
	VOXEL_GENERIC_OUTPUT_PIN(Result);
};

USTRUCT(meta = (Abstract))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_BinaryOperator : public FVoxelTemplateNode_OperatorBase
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_GENERIC_INPUT_PIN(A);
	VOXEL_GENERIC_INPUT_PIN(B);
	VOXEL_GENERIC_OUTPUT_PIN(Result);
};

USTRUCT(meta = (Abstract))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_CommutativeAssociativeOperator : public FVoxelTemplateNode_OperatorBase
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_GENERIC_INPUT_PIN_ARRAY(Input, 2);
	VOXEL_GENERIC_OUTPUT_PIN(Result);

public:
	class FDefinition : public Super::FDefinition
	{
	public:
		GENERATED_VOXEL_NODE_DEFINITION_BODY(FVoxelTemplateNode_CommutativeAssociativeOperator);

		virtual bool CanAddInputPin() const override
		{
			return CanAddToCategory(Node.InputPins);
		}
		virtual void AddInputPin() override;

		virtual bool CanRemoveInputPin() const override
		{
			return CanRemoveFromCategory(Node.InputPins);
		}
		virtual void RemoveInputPin() override
		{
			RemoveFromCategory(Node.InputPins);
		}
	};
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_Add : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, A, nullptr);
	VOXEL_MATH_INPUT_PIN(float, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} + {B}";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_Add_Int : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, A, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(int32, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} + {B}";
	}
};

USTRUCT(Category = "Math|Operators", meta = (CompactNodeTitle = "+", Keywords = "+ add plus", Operator = "+"))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_Add : public FVoxelTemplateNode_CommutativeAssociativeOperator
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetFloatInnerNode() const override
	{
		return FVoxelNode_Add::StaticStruct();
	}
	virtual UScriptStruct* GetInt32InnerNode() const override
	{
		return FVoxelNode_Add_Int::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_Subtract : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, A, nullptr);
	VOXEL_MATH_INPUT_PIN(float, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} - {B}";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_Subtract_Int : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, A, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(int32, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} - {B}";
	}
};

USTRUCT(Category = "Math|Operators", meta = (CompactNodeTitle = "-", Keywords = "- subtract minus", Operator = "-"))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_Subtract : public FVoxelTemplateNode_BinaryOperator
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetFloatInnerNode() const override
	{
		return FVoxelNode_Subtract::StaticStruct();
	}
	virtual UScriptStruct* GetInt32InnerNode() const override
	{
		return FVoxelNode_Subtract_Int::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_Multiply : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, A, nullptr);
	VOXEL_MATH_INPUT_PIN(float, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} * {B}";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_Multiply_Int : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, A, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(int32, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} * {B}";
	}
};

USTRUCT(Category = "Math|Operators", meta = (CompactNodeTitle = "*", Keywords = "* multiply", Operator = "*"))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_Multiply : public FVoxelTemplateNode_CommutativeAssociativeOperator
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetFloatInnerNode() const override
	{
		return FVoxelNode_Multiply::StaticStruct();
	}
	virtual UScriptStruct* GetInt32InnerNode() const override
	{
		return FVoxelNode_Multiply_Int::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_Divide : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, A, nullptr);
	VOXEL_MATH_INPUT_PIN(float, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} / {B}";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_Divide_Int : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, A, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(int32, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		if (bIsGpu)
		{
			return "{ReturnValue} = {B} != 0 ? {A} / {B} : 0";
		}
		else
		{
			return "IGNORE_PERF_WARNING\n{ReturnValue} = {B} != 0 ? {A} / {B} : 0";
		}
	}
};

USTRUCT(Category = "Math|Operators", meta = (CompactNodeTitle = "/", Keywords = "/ divide division", Operator = "/"))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_Divide : public FVoxelTemplateNode_BinaryOperator
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetFloatInnerNode() const override
	{
		return FVoxelNode_Divide::StaticStruct();
	}
	virtual UScriptStruct* GetInt32InnerNode() const override
	{
		return FVoxelNode_Divide_Int::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_Min : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, A, nullptr);
	VOXEL_MATH_INPUT_PIN(float, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = min({A}, {B})";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_Min_Int : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, A, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(int32, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = min({A}, {B})";
	}
};

USTRUCT(Category = "Math|Operators", meta = (CompactNodeTitle = "MIN"))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_Min : public FVoxelTemplateNode_CommutativeAssociativeOperator
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetFloatInnerNode() const override
	{
		return FVoxelNode_Min::StaticStruct();
	}
	virtual UScriptStruct* GetInt32InnerNode() const override
	{
		return FVoxelNode_Min_Int::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_Max : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, A, nullptr);
	VOXEL_MATH_INPUT_PIN(float, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = max({A}, {B})";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_Max_Int : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, A, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(int32, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = max({A}, {B})";
	}
};

USTRUCT(Category = "Math|Operators", meta = (CompactNodeTitle = "MAX"))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_Max : public FVoxelTemplateNode_CommutativeAssociativeOperator
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetFloatInnerNode() const override
	{
		return FVoxelNode_Max::StaticStruct();
	}
	virtual UScriptStruct* GetInt32InnerNode() const override
	{
		return FVoxelNode_Max_Int::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_Abs : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = abs({Value})";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_Abs_Int : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(int32, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = abs({Value})";
	}
};

USTRUCT(Category = "Math|Operators", meta = (CompactNodeTitle = "ABS"))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_Abs : public FVoxelTemplateNode_UnaryOperator
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetFloatInnerNode() const override
	{
		return FVoxelNode_Abs::StaticStruct();
	}
	virtual UScriptStruct* GetInt32InnerNode() const override
	{
		return FVoxelNode_Abs_Int::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_OneMinus : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = 1 - {Value}";
	}
};

USTRUCT(Category = "Math|Operators", meta = (CompactNodeTitle = "1-X"))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_OneMinus : public FVoxelTemplateNode_UnaryOperator
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetFloatInnerNode() const override
	{
		return FVoxelNode_OneMinus::StaticStruct();
	}
	virtual UScriptStruct* GetInt32InnerNode() const override
	{
		return nullptr;
	}
};