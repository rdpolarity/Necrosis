// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelPassthroughNodes.generated.h"

USTRUCT(meta = (Abstract))
struct VOXELMETAGRAPH_API FVoxelNode_Passthrough : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual EExecType GetExecType() const override
	{
		return EExecType::Any;
	}
	virtual FString GenerateCode(bool bIsGpu) const final override;
};

USTRUCT(meta = (Abstract, Keywords = "construct build"))
struct VOXELMETAGRAPH_API FVoxelNode_Make : public FVoxelNode_Passthrough
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
};

USTRUCT(meta = (Abstract))
struct VOXELMETAGRAPH_API FVoxelNode_Break : public FVoxelNode_Passthrough
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(Category = "Math|Conversions", meta = (Autocast, CompactNodeTitle = "->", DisplayName = "To Density (Float)", Keywords = "cast convert"))
struct VOXELMETAGRAPH_API FVoxelNode_FloatToDensity : public FVoxelNode_Passthrough
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue, DensityPin);
};

USTRUCT(Category = "Math|Conversions", meta = (Autocast, CompactNodeTitle = "->", DisplayName = "To Float (Density)", Keywords = "cast convert"))
struct VOXELMETAGRAPH_API FVoxelNode_DensityToFloat : public FVoxelNode_Passthrough
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_MATH_INPUT_PIN(float, Value, nullptr, DensityPin);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);
};

USTRUCT(Category = "Math|Conversions", meta = (Autocast, CompactNodeTitle = "->", DisplayName = "To Seed (Integer)", Keywords = "cast convert"))
struct VOXELMETAGRAPH_API FVoxelNode_IntToSeed : public FVoxelNode_Passthrough
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_MATH_INPUT_PIN(int32, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(int32, ReturnValue, SeedPin);
};

USTRUCT(Category = "Math|Conversions", meta = (Autocast, CompactNodeTitle = "->", DisplayName = "To Integer (Seed)", Keywords = "cast convert"))
struct VOXELMETAGRAPH_API FVoxelNode_SeedToInt : public FVoxelNode_Passthrough
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_MATH_INPUT_PIN(int32, Value, nullptr, SeedPin);
	VOXEL_MATH_OUTPUT_PIN(int32, ReturnValue);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(Category = "Math|Vector2D")
struct VOXELMETAGRAPH_API FVoxelNode_MakeVector2D : public FVoxelNode_Make
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, X, nullptr);
	VOXEL_MATH_INPUT_PIN(float, Y, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FVector2D, ReturnValue);
};

USTRUCT(Category = "Math|Vector")
struct VOXELMETAGRAPH_API FVoxelNode_MakeVector : public FVoxelNode_Make
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, X, nullptr);
	VOXEL_MATH_INPUT_PIN(float, Y, nullptr);
	VOXEL_MATH_INPUT_PIN(float, Z, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FVector, ReturnValue);
};

USTRUCT(Category = "Math|Rotation")
struct VOXELMETAGRAPH_API FVoxelNode_MakeQuaternion : public FVoxelNode_Passthrough
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, X, nullptr);
	VOXEL_MATH_INPUT_PIN(float, Y, nullptr);
	VOXEL_MATH_INPUT_PIN(float, Z, nullptr);
	VOXEL_MATH_INPUT_PIN(float, W, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FQuat, ReturnValue);
};

USTRUCT(Category = "Math|Color")
struct VOXELMETAGRAPH_API FVoxelNode_MakeLinearColor : public FVoxelNode_Make
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, R, 0.f);
	VOXEL_MATH_INPUT_PIN(float, G, 0.f);
	VOXEL_MATH_INPUT_PIN(float, B, 0.f);
	VOXEL_MATH_INPUT_PIN(float, A, 1.f);
	VOXEL_MATH_OUTPUT_PIN(FLinearColor, ReturnValue);
};

USTRUCT(Category = "Math|Int Point")
struct VOXELMETAGRAPH_API FVoxelNode_MakeIntPoint : public FVoxelNode_Make
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, X, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, Y, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FIntPoint, ReturnValue);
};

USTRUCT(Category = "Math|Int Vector")
struct VOXELMETAGRAPH_API FVoxelNode_MakeIntVector : public FVoxelNode_Make
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, X, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, Y, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, Z, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FIntVector, ReturnValue);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(Category = "Math|Vector2D")
struct VOXELMETAGRAPH_API FVoxelNode_BreakVector2D : public FVoxelNode_Break
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector2D, Vector, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, X);
	VOXEL_MATH_OUTPUT_PIN(float, Y);
};

USTRUCT(Category = "Math|Vector")
struct VOXELMETAGRAPH_API FVoxelNode_BreakVector : public FVoxelNode_Break
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector, Vector, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, X);
	VOXEL_MATH_OUTPUT_PIN(float, Y);
	VOXEL_MATH_OUTPUT_PIN(float, Z);
};

USTRUCT(Category = "Math|Rotation")
struct VOXELMETAGRAPH_API FVoxelNode_BreakQuaternion : public FVoxelNode_Passthrough
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FQuat, Quaternion, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, X);
	VOXEL_MATH_OUTPUT_PIN(float, Y);
	VOXEL_MATH_OUTPUT_PIN(float, Z);
	VOXEL_MATH_OUTPUT_PIN(float, W);
};

USTRUCT(Category = "Math|Color")
struct VOXELMETAGRAPH_API FVoxelNode_BreakLinearColor : public FVoxelNode_Break
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FLinearColor, Color, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, R);
	VOXEL_MATH_OUTPUT_PIN(float, G);
	VOXEL_MATH_OUTPUT_PIN(float, B);
	VOXEL_MATH_OUTPUT_PIN(float, A);
};

USTRUCT(Category = "Math|Int Point")
struct VOXELMETAGRAPH_API FVoxelNode_BreakIntPoint : public FVoxelNode_Break
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FIntPoint, Vector, nullptr);
	VOXEL_MATH_OUTPUT_PIN(int32, X);
	VOXEL_MATH_OUTPUT_PIN(int32, Y);
};

USTRUCT(Category = "Math|Int Vector")
struct VOXELMETAGRAPH_API FVoxelNode_BreakIntVector : public FVoxelNode_Break
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FIntVector, Vector, nullptr);
	VOXEL_MATH_OUTPUT_PIN(int32, X);
	VOXEL_MATH_OUTPUT_PIN(int32, Y);
	VOXEL_MATH_OUTPUT_PIN(int32, Z);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(Category = "Math|Conversions", meta = (Autocast, CompactNodeTitle = "->", DisplayName = "To Vector (Vector2D)", Keywords = "cast convert"))
struct VOXELMETAGRAPH_API FVoxelNode_Vector2DToVector : public FVoxelNode_Passthrough
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector2D, Vector2D, nullptr);
	VOXEL_MATH_INPUT_PIN(float, Z, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FVector, ReturnValue);
};

USTRUCT(Category = "Math|Conversions", meta = (Autocast, CompactNodeTitle = "->", DisplayName = "To Color (Vector2D)", Keywords = "cast convert"))
struct VOXELMETAGRAPH_API FVoxelNode_Vector2DToColor : public FVoxelNode_Passthrough
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector2D, Vector2D, nullptr);
	VOXEL_MATH_INPUT_PIN(float, B, nullptr);
	VOXEL_MATH_INPUT_PIN(float, A, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FLinearColor, ReturnValue);
};

USTRUCT(Category = "Math|Conversions", meta = (Autocast, CompactNodeTitle = "->", DisplayName = "To Vector2D (Vector)", Keywords = "cast convert"))
struct VOXELMETAGRAPH_API FVoxelNode_VectorToVector2D : public FVoxelNode_Passthrough
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector, Vector, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FVector2D, ReturnValue);
};

USTRUCT(Category = "Math|Conversions", meta = (Autocast, CompactNodeTitle = "->", DisplayName = "To Color (Vector)", Keywords = "cast convert"))
struct VOXELMETAGRAPH_API FVoxelNode_VectorToColor : public FVoxelNode_Passthrough
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector, Vector, nullptr);
	VOXEL_MATH_INPUT_PIN(float, A, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FLinearColor, ReturnValue);
};

USTRUCT(Category = "Math|Conversions", meta = (Autocast, CompactNodeTitle = "->", DisplayName = "To Vector2D (Color)", Keywords = "cast convert"))
struct VOXELMETAGRAPH_API FVoxelNode_ColorToVector2D : public FVoxelNode_Passthrough
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FLinearColor, Color, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FVector2D, ReturnValue);
};

USTRUCT(Category = "Math|Conversions", meta = (Autocast, CompactNodeTitle = "->", DisplayName = "To Vector (Color)", Keywords = "cast convert"))
struct VOXELMETAGRAPH_API FVoxelNode_ColorToVector : public FVoxelNode_Passthrough
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FLinearColor, Color, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FVector, ReturnValue);
};

USTRUCT(Category = "Math|Conversions", meta = (Autocast, CompactNodeTitle = "->", DisplayName = "To IntVector (IntPoint)", Keywords = "cast convert"))
struct VOXELMETAGRAPH_API FVoxelNode_IntPointToIntVector : public FVoxelNode_Passthrough
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FIntPoint, Vector2D, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, Z, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FIntVector, ReturnValue);
};

USTRUCT(Category = "Math|Conversions", meta = (Autocast, CompactNodeTitle = "->", DisplayName = "To IntPoint (IntVector)", Keywords = "cast convert"))
struct VOXELMETAGRAPH_API FVoxelNode_IntVectorToIntPoint : public FVoxelNode_Passthrough
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FIntVector, Vector, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FIntPoint, ReturnValue);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(Category = "Math|Conversions", meta = (Autocast, CompactNodeTitle = "->", DisplayName = "To Vector (Float)", Keywords = "cast convert"))
struct VOXELMETAGRAPH_API FVoxelNode_FloatToVector : public FVoxelNode_Passthrough
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FVector, ReturnValue);
};

USTRUCT(Category = "Math|Conversions", meta = (Autocast, CompactNodeTitle = "->", DisplayName = "To Vector2D (Float)", Keywords = "cast convert"))
struct VOXELMETAGRAPH_API FVoxelNode_FloatToVector2D : public FVoxelNode_Passthrough
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FVector2D, ReturnValue);
};

USTRUCT(Category = "Math|Conversions", meta = (Autocast, CompactNodeTitle = "->", DisplayName = "To Color (Float)", Keywords = "cast convert"))
struct VOXELMETAGRAPH_API FVoxelNode_FloatToColor : public FVoxelNode_Passthrough
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FLinearColor, ReturnValue);
};

USTRUCT(Category = "Math|Conversions", meta = (Autocast, CompactNodeTitle = "->", DisplayName = "To IntPoint (Integer)", Keywords = "cast convert"))
struct VOXELMETAGRAPH_API FVoxelNode_Int32ToIntPoint : public FVoxelNode_Passthrough
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FIntPoint, ReturnValue);
};

USTRUCT(Category = "Math|Conversions", meta = (Autocast, CompactNodeTitle = "->", DisplayName = "To IntVector (Integer)", Keywords = "cast convert"))
struct VOXELMETAGRAPH_API FVoxelNode_Int32ToIntVector : public FVoxelNode_Passthrough
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FIntVector, ReturnValue);
};