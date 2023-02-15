// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Nodes/Templates/VoxelTemplateNode.h"
#include "VoxelBoolNodes.generated.h"

USTRUCT(meta = (Abstract))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_EqualityBase : public FVoxelTemplateNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_GENERIC_INPUT_PIN(A);
	VOXEL_GENERIC_INPUT_PIN(B);
	VOXEL_MATH_OUTPUT_PIN(bool, Result);

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
	virtual UScriptStruct* GetBoolInnerNode() const VOXEL_PURE_VIRTUAL({});
	virtual UScriptStruct* GetConnectionInnerNode() const VOXEL_PURE_VIRTUAL({});
};

USTRUCT(meta = (Abstract))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_MultiInputBooleanNode : public FVoxelTemplateNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN_ARRAY(bool, Input, false, 2);
	VOXEL_MATH_OUTPUT_PIN(bool, Result);

public:
	virtual bool IsPureNode() const override
	{
		return true;
	}
	virtual bool ShowPromotablePinsAsWildcards() const override
	{
		return false;
	}

	virtual FPin* ExpandPins(FNode& Node, TArray<FPin*> Pins, const TArray<FPin*>& AllPins) const override;

	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;

	virtual UScriptStruct* GetBooleanNode() const VOXEL_PURE_VIRTUAL({});

public:
	struct FDefinition : public Super::FDefinition
	{
		GENERATED_VOXEL_NODE_DEFINITION_BODY(FVoxelTemplateNode_MultiInputBooleanNode);

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

USTRUCT(meta = (Abstract))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_EqualitySingleDimension : public FVoxelTemplateNode_EqualityBase
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_BooleanAND : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(bool, A, nullptr);
	VOXEL_MATH_INPUT_PIN(bool, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(bool, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} && {B}";
	}
};

USTRUCT(Category = "Math|Boolean", meta = (DisplayName = "AND Boolean", CompactNodeTitle = "AND", Keywords = "& and"))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_BooleanAND : public FVoxelTemplateNode_MultiInputBooleanNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetBooleanNode() const override
	{
		return FVoxelNode_BooleanAND::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_BooleanOR : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(bool, A, nullptr);
	VOXEL_MATH_INPUT_PIN(bool, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(bool, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} || {B}";
	}
};

USTRUCT(Category = "Math|Boolean", meta = (DisplayName = "OR Boolean", CompactNodeTitle = "OR", Keywords = "| or"))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_BooleanOR : public FVoxelTemplateNode_MultiInputBooleanNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetBooleanNode() const override
	{
		return FVoxelNode_BooleanOR::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_BooleanNAND : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(bool, A, nullptr);
	VOXEL_MATH_INPUT_PIN(bool, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(bool, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = !({A} && {B})";
	}
};

USTRUCT(Category = "Math|Boolean", meta = (DisplayName = "NAND Boolean", CompactNodeTitle = "NAND", Keywords = "!& nand"))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_BooleanNAND : public FVoxelTemplateNode_MultiInputBooleanNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetBooleanNode() const override
	{
		return FVoxelNode_BooleanNAND::StaticStruct();
	}
};
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_EqualEqual : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, A, nullptr);
	VOXEL_MATH_INPUT_PIN(float, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(bool, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} == {B}";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_EqualEqual_IntInt : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, A, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(bool, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} == {B}";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_EqualEqual_BoolBool : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(bool, A, nullptr);
	VOXEL_MATH_INPUT_PIN(bool, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(bool, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} == {B}";
	}
};

USTRUCT(Category = "Math|Operators", meta = (CompactNodeTitle = "==", Keywords = "== equal"))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_Equal : public FVoxelTemplateNode_EqualityBase
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetFloatInnerNode() const override
	{
		return FVoxelNode_EqualEqual::StaticStruct();
	}
	virtual UScriptStruct* GetInt32InnerNode() const override
	{
		return FVoxelNode_EqualEqual_IntInt::StaticStruct();
	}
	virtual UScriptStruct* GetBoolInnerNode() const override
	{
		return FVoxelNode_EqualEqual_BoolBool::StaticStruct();
	}
	virtual UScriptStruct* GetConnectionInnerNode() const override
	{
		return FVoxelNode_BooleanAND::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_NearlyEqual : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, A, nullptr);
	VOXEL_MATH_INPUT_PIN(float, B, nullptr);
	VOXEL_MATH_INPUT_PIN(float, ErrorTolerance, 1.e-6f);
	VOXEL_MATH_OUTPUT_PIN(bool, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = abs({A} - {B}) <= {ErrorTolerance}";
	}
};

USTRUCT(Category = "Math|Operators", meta = (Keywords = "== equal"))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_NearlyEqual : public FVoxelTemplateNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_GENERIC_INPUT_PIN(A);
	VOXEL_GENERIC_INPUT_PIN(B);
	VOXEL_MATH_INPUT_PIN(float, ErrorTolerance, nullptr);
	VOXEL_MATH_OUTPUT_PIN(bool, Result);

public:
	virtual bool IsPureNode() const override
	{
		return true;
	}

	virtual FPin* ExpandPins(FNode& Node, TArray<FPin*> Pins, const TArray<FPin*>& AllPins) const override;

	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_NotEqual : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, A, nullptr);
	VOXEL_MATH_INPUT_PIN(float, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(bool, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} != {B}";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_NotEqual_IntInt : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, A, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(bool, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} != {B}";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_NotEqual_BoolBool : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(bool, A, nullptr);
	VOXEL_MATH_INPUT_PIN(bool, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(bool, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} != {B}";
	}
};

USTRUCT(Category = "Math|Operators", meta = (CompactNodeTitle = "!=", Keywords = "!= not equal"))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_NotEqual : public FVoxelTemplateNode_EqualityBase
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetFloatInnerNode() const override
	{
		return FVoxelNode_NotEqual::StaticStruct();
	}
	virtual UScriptStruct* GetInt32InnerNode() const override
	{
		return FVoxelNode_NotEqual_IntInt::StaticStruct();
	}
	virtual UScriptStruct* GetBoolInnerNode() const override
	{
		return FVoxelNode_NotEqual_BoolBool::StaticStruct();
	}
	virtual UScriptStruct* GetConnectionInnerNode() const override
	{
		return FVoxelNode_BooleanOR::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_Less : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, A, nullptr);
	VOXEL_MATH_INPUT_PIN(float, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(bool, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} < {B}";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_Less_IntInt : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, A, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(bool, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} < {B}";
	}
};

USTRUCT(Category = "Math|Operators", meta = (CompactNodeTitle = "<", Keywords = "< less"))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_Less : public FVoxelTemplateNode_EqualitySingleDimension
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetFloatInnerNode() const override
	{
		return FVoxelNode_Less::StaticStruct();
	}
	virtual UScriptStruct* GetInt32InnerNode() const override
	{
		return FVoxelNode_Less_IntInt::StaticStruct();
	}
	virtual UScriptStruct* GetBoolInnerNode() const override
	{
		return nullptr;
	}
	virtual UScriptStruct* GetConnectionInnerNode() const override
	{
		return FVoxelNode_BooleanAND::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_Greater : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, A, nullptr);
	VOXEL_MATH_INPUT_PIN(float, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(bool, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} > {B}";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_Greater_IntInt : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, A, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(bool, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} > {B}";
	}
};

USTRUCT(Category = "Math|Operators", meta = (CompactNodeTitle = ">", Keywords = "> greater"))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_Greater : public FVoxelTemplateNode_EqualitySingleDimension
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetFloatInnerNode() const override
	{
		return FVoxelNode_Greater::StaticStruct();
	}
	virtual UScriptStruct* GetInt32InnerNode() const override
	{
		return FVoxelNode_Greater_IntInt::StaticStruct();
	}
	virtual UScriptStruct* GetBoolInnerNode() const override
	{
		return nullptr;
	}
	virtual UScriptStruct* GetConnectionInnerNode() const override
	{
		return FVoxelNode_BooleanAND::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_LessEqual : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, A, nullptr);
	VOXEL_MATH_INPUT_PIN(float, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(bool, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} <= {B}";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_LessEqual_IntInt : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, A, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(bool, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} <= {B}";
	}
};

USTRUCT(Category = "Math|Operators", meta = (CompactNodeTitle = "<=", Keywords = "<= less"))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_LessEqual : public FVoxelTemplateNode_EqualitySingleDimension
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetFloatInnerNode() const override
	{
		return FVoxelNode_LessEqual::StaticStruct();
	}
	virtual UScriptStruct* GetInt32InnerNode() const override
	{
		return FVoxelNode_LessEqual_IntInt::StaticStruct();
	}
	virtual UScriptStruct* GetBoolInnerNode() const override
	{
		return nullptr;
	}
	virtual UScriptStruct* GetConnectionInnerNode() const override
	{
		return FVoxelNode_BooleanAND::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_GreaterEqual : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, A, nullptr);
	VOXEL_MATH_INPUT_PIN(float, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(bool, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} >= {B}";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_GreaterEqual_IntInt : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, A, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(bool, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} >= {B}";
	}
};

USTRUCT(Category = "Math|Operators", meta = (CompactNodeTitle = ">=", Keywords = ">= greater"))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_GreaterEqual : public FVoxelTemplateNode_EqualitySingleDimension
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetFloatInnerNode() const override
	{
		return FVoxelNode_GreaterEqual::StaticStruct();
	}
	virtual UScriptStruct* GetInt32InnerNode() const override
	{
		return FVoxelNode_GreaterEqual_IntInt::StaticStruct();
	}
	virtual UScriptStruct* GetBoolInnerNode() const override
	{
		return nullptr;
	}
	virtual UScriptStruct* GetConnectionInnerNode() const override
	{
		return FVoxelNode_BooleanAND::StaticStruct();
	}
};