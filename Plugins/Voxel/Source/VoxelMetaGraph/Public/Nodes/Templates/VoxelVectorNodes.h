// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Nodes/Templates/VoxelTemplateNode.h"
#include "VoxelVectorNodes.generated.h"

USTRUCT(meta = (Abstract))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_AbstractVectorBase : public FVoxelTemplateNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_GENERIC_OUTPUT_PIN(ReturnValue);

public:
	virtual bool IsPureNode() const override
	{
		return true;
	}

	virtual FPin* ExpandPins(FNode& Node, TArray<FPin*> Pins, const TArray<FPin*>& AllPins) const override;

	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;

	static bool IsVector(const FVoxelPinType& Type);

	virtual UScriptStruct* GetVector2DInnerNode() const
	{
		return nullptr;
	}
	virtual UScriptStruct* GetVectorInnerNode() const
	{
		return nullptr;
	}
	virtual FVoxelPinType GetVector2DResultType() const
	{
		return FVoxelPinType::Make<float>();
	}
	virtual FVoxelPinType GetVectorResultType() const
	{
		return FVoxelPinType::Make<float>();
	}
};

USTRUCT(meta = (Abstract))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_VectorUnaryOperator : public FVoxelTemplateNode_AbstractVectorBase
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_GENERIC_INPUT_PIN(Vector);
};

USTRUCT(meta = (Abstract))
struct VOXELMETAGRAPH_API FVoxelTemplateNode_VectorBinaryOperator : public FVoxelTemplateNode_AbstractVectorBase
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_GENERIC_INPUT_PIN(V1);
	VOXEL_GENERIC_INPUT_PIN(V2);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_VectorCrossProduct : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector, V1, nullptr);
	VOXEL_MATH_INPUT_PIN(FVector, V2, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FVector, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = cross({V1}, {V2})";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_Vector2DCrossProduct : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector2D, V1, nullptr);
	VOXEL_MATH_INPUT_PIN(FVector2D, V2, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {V1}.x * {V2}.y - {V1}.y * {V2}.x";
	}
};

USTRUCT(Category = "Math|Vector Operators")
struct VOXELMETAGRAPH_API FVoxelTemplateNode_CrossProduct : public FVoxelTemplateNode_VectorBinaryOperator
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetVectorInnerNode() const override
	{
		return FVoxelNode_VectorCrossProduct::StaticStruct();
	}
	virtual UScriptStruct* GetVector2DInnerNode() const override
	{
		return FVoxelNode_Vector2DCrossProduct::StaticStruct();
	}

	virtual FVoxelPinType GetVectorResultType() const override
	{
		return FVoxelPinType::Make<FVector>();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_VectorDotProduct : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector, V1, nullptr);
	VOXEL_MATH_INPUT_PIN(FVector, V2, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {V1}.x * {V2}.x + {V1}.y * {V2}.y + {V1}.z * {V2}.z";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_Vector2DDotProduct : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector2D, V1, nullptr);
	VOXEL_MATH_INPUT_PIN(FVector2D, V2, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {V1}.x * {V2}.x + {V1}.y * {V2}.y";
	}
};

USTRUCT(Category = "Math|Vector Operators")
struct VOXELMETAGRAPH_API FVoxelTemplateNode_DotProduct : public FVoxelTemplateNode_VectorBinaryOperator
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetVectorInnerNode() const override
	{
		return FVoxelNode_VectorDotProduct::StaticStruct();
	}
	virtual UScriptStruct* GetVector2DInnerNode() const override
	{
		return FVoxelNode_Vector2DDotProduct::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_NormalizeVector2D : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector2D, Vector, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FVector2D, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = normalize({Vector})";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_NormalizeVector : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector, Vector, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FVector, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = normalize({Vector})";
	}
};

USTRUCT(Category = "Math|Vector Operators")
struct VOXELMETAGRAPH_API FVoxelTemplateNode_Normalize : public FVoxelTemplateNode_VectorUnaryOperator
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetVectorInnerNode() const override
	{
		return FVoxelNode_NormalizeVector::StaticStruct();
	}
	virtual UScriptStruct* GetVector2DInnerNode() const override
	{
		return FVoxelNode_NormalizeVector2D::StaticStruct();
	}

	virtual FVoxelPinType GetVector2DResultType() const override
	{
		return FVoxelPinType::Make<FVector2D>();
	}
	virtual FVoxelPinType GetVectorResultType() const override
	{
		return FVoxelPinType::Make<FVector>();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_VectorLength : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector, Vector, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = length({Vector})";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_Vector2DLength : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector2D, Vector, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = length({Vector})";
	}
};

USTRUCT(Category = "Math|Vector Operators")
struct VOXELMETAGRAPH_API FVoxelTemplateNode_Length : public FVoxelTemplateNode_VectorUnaryOperator
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetVectorInnerNode() const override
	{
		return FVoxelNode_VectorLength::StaticStruct();
	}
	virtual UScriptStruct* GetVector2DInnerNode() const override
	{
		return FVoxelNode_Vector2DLength::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_VectorLengthXY : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector, Vector, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = length(MakeFloat2({Vector}.x, {Vector}.y))";
	}
};

USTRUCT(Category = "Math|Vector Operators")
struct VOXELMETAGRAPH_API FVoxelTemplateNode_LengthXY : public FVoxelTemplateNode_VectorUnaryOperator
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetVectorInnerNode() const override
	{
		return FVoxelNode_VectorLengthXY::StaticStruct();
	}
	virtual UScriptStruct* GetVector2DInnerNode() const override
	{
		return FVoxelNode_Vector2DLength::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_VectorDistance : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector, V1, nullptr);
	VOXEL_MATH_INPUT_PIN(FVector, V2, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = distance({V1}, {V2})";
	}
};

USTRUCT(meta = (Internal))
struct FVoxelNode_Vector2DDistance : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector2D, V1, nullptr);
	VOXEL_MATH_INPUT_PIN(FVector2D, V2, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = distance({V1}, {V2})";
	}
};

USTRUCT(Category = "Math|Vector Operators")
struct VOXELMETAGRAPH_API FVoxelTemplateNode_Distance : public FVoxelTemplateNode_VectorBinaryOperator
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetVectorInnerNode() const override
	{
		return FVoxelNode_VectorDistance::StaticStruct();
	}
	virtual UScriptStruct* GetVector2DInnerNode() const override
	{
		return FVoxelNode_Vector2DDistance::StaticStruct();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct FVoxelNode_VectorDistance2D : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector, V1, nullptr);
	VOXEL_MATH_INPUT_PIN(FVector, V2, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = distance(MakeFloat2({V1}.x, {V1}.y), MakeFloat2({V2}.x, {V2}.y))";
	}
};

USTRUCT(Category = "Math|Vector Operators")
struct VOXELMETAGRAPH_API FVoxelTemplateNode_Distance2D : public FVoxelTemplateNode_VectorBinaryOperator
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual UScriptStruct* GetVectorInnerNode() const override
	{
		return FVoxelNode_VectorDistance2D::StaticStruct();
	}
	virtual UScriptStruct* GetVector2DInnerNode() const override
	{
		return FVoxelNode_Vector2DDistance::StaticStruct();
	}
};