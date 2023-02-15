// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelMathNodes.generated.h"

USTRUCT(Category = "Math|Float")
struct VOXELMETAGRAPH_API FVoxelNode_Power : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Base, nullptr);
	VOXEL_MATH_INPUT_PIN(float, Exp, 2.0f);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		// Base cannot be negative
		return "{ReturnValue} = pow(abs({Base}), {Exp})";
	}
};

USTRUCT(Category = "Math|Trig", DisplayName = "Sin (Radians)", meta = (CompactNodeTitle = "SIN"))
struct VOXELMETAGRAPH_API FVoxelNode_Sin : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = sin({Value})";
	}
};

USTRUCT(Category = "Math|Trig", DisplayName = "Sin (Degrees)", meta = (CompactNodeTitle = "SINd"))
struct VOXELMETAGRAPH_API FVoxelNode_SinDegrees : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = sin(PI / 180.f * {Value})";
	}
};

USTRUCT(Category = "Math|Trig", DisplayName = "Cos (Radians)", meta = (CompactNodeTitle = "COS"))
struct VOXELMETAGRAPH_API FVoxelNode_Cos : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = cos({Value})";
	}
};

USTRUCT(Category = "Math|Trig", DisplayName = "Cos (Degrees)", meta = (CompactNodeTitle = "COSd"))
struct VOXELMETAGRAPH_API FVoxelNode_CosDegrees : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = cos(PI / 180.f * {Value})";
	}
};

USTRUCT(Category = "Math|Trig", DisplayName = "Tan (Radians)", meta = (CompactNodeTitle = "TAN"))
struct VOXELMETAGRAPH_API FVoxelNode_Tan : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = tan({Value})";
	}
};

USTRUCT(Category = "Math|Trig", DisplayName = "Tan (Degrees)", meta = (CompactNodeTitle = "TANd"))
struct VOXELMETAGRAPH_API FVoxelNode_TanDegrees : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = tan(PI / 180.f * {Value})";
	}
};

USTRUCT(Category = "Math|Trig", DisplayName = "Asin (Radians)", meta = (CompactNodeTitle = "ASIN"))
struct VOXELMETAGRAPH_API FVoxelNode_Asin : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = asin({Value})";
	}
};

USTRUCT(Category = "Math|Trig", DisplayName = "Asin (Degrees)", meta = (CompactNodeTitle = "ASINd"))
struct VOXELMETAGRAPH_API FVoxelNode_AsinDegrees : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = 180.f / PI * asin({Value})";
	}
};

USTRUCT(Category = "Math|Trig", DisplayName = "Acos (Radians)", meta = (CompactNodeTitle = "ACOS"))
struct VOXELMETAGRAPH_API FVoxelNode_Acos : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = acos({Value})";
	}
};

USTRUCT(Category = "Math|Trig", DisplayName = "Acos (Degrees)", meta = (CompactNodeTitle = "ACOSd"))
struct VOXELMETAGRAPH_API FVoxelNode_AcosDegrees : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = 180.f / PI * acos({Value})";
	}
};

USTRUCT(Category = "Math|Trig", DisplayName = "Atan (Radians)", meta = (CompactNodeTitle = "ATAN"))
struct VOXELMETAGRAPH_API FVoxelNode_Atan : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = atan({Value})";
	}
};

USTRUCT(Category = "Math|Trig", DisplayName = "Atan (Degrees)", meta = (CompactNodeTitle = "ATANd"))
struct VOXELMETAGRAPH_API FVoxelNode_AtanDegrees : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = 180.f / PI * atan({Value})";
	}
};

USTRUCT(Category = "Math|Trig", DisplayName = "Atan2 (Radians)", meta = (CompactNodeTitle = "ATAN2"))
struct VOXELMETAGRAPH_API FVoxelNode_Atan2 : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Y, nullptr);
	VOXEL_MATH_INPUT_PIN(float, X, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = atan2({Y}, {X})";
	}
};

USTRUCT(Category = "Math|Trig", DisplayName = "Atan2 (Degrees)", meta = (CompactNodeTitle = "ATAN2d"))
struct VOXELMETAGRAPH_API FVoxelNode_Atan2Degrees : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Y, nullptr);
	VOXEL_MATH_INPUT_PIN(float, X, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = 180.f / PI * atan2({Y}, {X})";
	}
};

USTRUCT(Category = "Math|Trig", DisplayName = "Get PI", meta = (CompactNodeTitle = "PI"))
struct VOXELMETAGRAPH_API FVoxelNode_GetPI : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = PI";
	}
};

USTRUCT(Category = "Math|Trig", meta = (CompactNodeTitle = "R2D"))
struct VOXELMETAGRAPH_API FVoxelNode_RadiansToDegrees : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, A, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} * (180.f / PI)";
	}
};

USTRUCT(Category = "Math|Trig", meta = (CompactNodeTitle = "D2R"))
struct VOXELMETAGRAPH_API FVoxelNode_DegreesToRadians : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, A, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} * (PI / 180.f)";
	}
};

USTRUCT(Category = "Math|Float")
struct VOXELMETAGRAPH_API FVoxelNode_SmoothStep : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, A, 0.f);
	VOXEL_MATH_INPUT_PIN(float, B, 1.f);
	VOXEL_MATH_INPUT_PIN(float, Alpha, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = SmoothStep({A}, {B}, {Alpha})";
	}
};

USTRUCT(Category = "Math|Float")
struct VOXELMETAGRAPH_API FVoxelNode_BilinearInterpolation : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector2D, Position, nullptr);
	VOXEL_MATH_INPUT_PIN(float, X0Y0, nullptr);
	VOXEL_MATH_INPUT_PIN(float, X1Y0, nullptr);
	VOXEL_MATH_INPUT_PIN(float, X0Y1, nullptr);
	VOXEL_MATH_INPUT_PIN(float, X1Y1, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = BilinearInterpolation({X0Y0}, {X1Y0}, {X0Y1}, {X1Y1}, {Position}.x, {Position}.y)";
	}
};

USTRUCT(Category = "Math|Float")
struct VOXELMETAGRAPH_API FVoxelNode_Frac : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {Value} - floor({Value})";
	}
};

USTRUCT(Category = "Math|Float")
struct VOXELMETAGRAPH_API FVoxelNode_SmoothMin : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, DistanceA, nullptr);
	VOXEL_MATH_INPUT_PIN(float, DistanceB, nullptr);
	VOXEL_MATH_INPUT_PIN(float, Smoothness, 100.0f);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = SmoothMin({DistanceA}, {DistanceB}, {Smoothness})";
	}
};

USTRUCT(Category = "Math|Float")
struct VOXELMETAGRAPH_API FVoxelNode_SmoothMax : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, DistanceA, nullptr);
	VOXEL_MATH_INPUT_PIN(float, DistanceB, nullptr);
	VOXEL_MATH_INPUT_PIN(float, Smoothness, 100.0f);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = SmoothMax({DistanceA}, {DistanceB}, {Smoothness})";
	}
};

USTRUCT(Category = "Math|Float")
struct VOXELMETAGRAPH_API FVoxelNode_SmoothSubtraction : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, DistanceA, nullptr);
	VOXEL_MATH_INPUT_PIN(float, DistanceB, nullptr);
	VOXEL_MATH_INPUT_PIN(float, Smoothness, 100.0f);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = SmoothSubtraction({DistanceA}, {DistanceB}, {Smoothness})";
	}
};

USTRUCT(Category = "Math|Boolean", DisplayName = "NOT Boolean", meta = (Keywords = "! not negate", CompactNodeTitle = "NOT"))
struct VOXELMETAGRAPH_API FVoxelNode_BooleanNOT : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(bool, A, nullptr);
	VOXEL_MATH_OUTPUT_PIN(bool, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = !{A}";
	}
};

USTRUCT(Category = "Math|Boolean", DisplayName = "NOR Boolean", meta = (Keywords = "!^ nor", CompactNodeTitle = "NOR"))
struct VOXELMETAGRAPH_API FVoxelNode_BooleanNOR : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(bool, A, nullptr);
	VOXEL_MATH_INPUT_PIN(bool, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(bool, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = !({A} || {B})";
	}
};

USTRUCT(Category = "Math|Boolean", DisplayName = "XOR Boolean", meta = (Keywords = "^ xor", CompactNodeTitle = "XOR"))
struct VOXELMETAGRAPH_API FVoxelNode_BooleanXOR : public FVoxelNode_CodeGen
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

USTRUCT(Category = "Math|Density")
struct VOXELMETAGRAPH_API FVoxelNode_MakeDensityFromHeight : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector, Position, nullptr);
	VOXEL_MATH_INPUT_PIN(float, Height, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue, DensityPin);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {Position}.z - {Height}";
	}
};

USTRUCT(Category = "Math|Density")
struct VOXELMETAGRAPH_API FVoxelNode_Density_SmoothUnion : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, DistanceA, nullptr, DensityPin);
	VOXEL_MATH_INPUT_PIN(float, DistanceB, nullptr, DensityPin);
	VOXEL_MATH_INPUT_PIN(float, Smoothness, 100.0f);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue, DensityPin);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = SmoothMin({DistanceA}, {DistanceB}, {Smoothness})";
	}
};

USTRUCT(Category = "Math|Density")
struct VOXELMETAGRAPH_API FVoxelNode_Density_SmoothIntersection : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_MATH_INPUT_PIN(float, DistanceA, nullptr, DensityPin);
	VOXEL_MATH_INPUT_PIN(float, DistanceB, nullptr, DensityPin);
	VOXEL_MATH_INPUT_PIN(float, Smoothness, 100.0f);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue, DensityPin);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = SmoothMax({DistanceA}, {DistanceB}, {Smoothness})";
	}
};

USTRUCT(Category = "Math|Density")
struct VOXELMETAGRAPH_API FVoxelNode_Density_SmoothSubtraction : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_MATH_INPUT_PIN(float, DistanceA, nullptr, DensityPin);
	VOXEL_MATH_INPUT_PIN(float, DistanceB, nullptr, DensityPin);
	VOXEL_MATH_INPUT_PIN(float, Smoothness, 100.0f);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue, DensityPin);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = SmoothSubtraction({DistanceA}, {DistanceB}, {Smoothness})";
	}
};

USTRUCT(Category = "Math|Density")
struct VOXELMETAGRAPH_API FVoxelNode_Density_Invert : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_MATH_INPUT_PIN(float, Density, nullptr, DensityPin);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue, DensityPin);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = -{Density}";
	}
};

USTRUCT(Category = "Math|Conversions", DisplayName = "To Float (Integer)", meta = (Keywords = "cast convert", CompactNodeTitle = "->", Autocast))
struct VOXELMETAGRAPH_API FVoxelNode_Conv_IntToFloat : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = (float){Value}";
	}
};

USTRUCT(Category = "Math|Integer")
struct VOXELMETAGRAPH_API FVoxelNode_LeftShift : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, Value, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, Shift, nullptr);
	VOXEL_MATH_OUTPUT_PIN(int32, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		if (bIsGpu)
		{
			return "{ReturnValue} = {Value} << clamp({Shift}, 0, 31)";
		}
		else
		{
			return "IGNORE_PERF_WARNING\n{ReturnValue} = {Value} << clamp({Shift}, 0, 31)";
		}
	}
};

USTRUCT(Category = "Math|Integer")
struct VOXELMETAGRAPH_API FVoxelNode_RightShift : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, Value, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, Shift, nullptr);
	VOXEL_MATH_OUTPUT_PIN(int32, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		if (bIsGpu)
		{
			return "{ReturnValue} = {Value} >> clamp({Shift}, 0, 31)";
		}
		else
		{
			return "IGNORE_PERF_WARNING\n{ReturnValue} = {Value} >> clamp({Shift}, 0, 31)";
		}
	}
};

USTRUCT(Category = "Math|Integer", DisplayName = "Bitwise AND", meta = (Keywords = "& and", CompactNodeTitle = "&"))
struct VOXELMETAGRAPH_API FVoxelNode_Bitwise_And : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, A, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(int32, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} & {B}";
	}
};

USTRUCT(Category = "Math|Integer", DisplayName = "Bitwise OR", meta = (Keywords = "| or", CompactNodeTitle = "|"))
struct VOXELMETAGRAPH_API FVoxelNode_Bitwise_Or : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, A, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(int32, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} | {B}";
	}
};

USTRUCT(Category = "Math|Integer", DisplayName = "Bitwise XOR", meta = (Keywords = "^ xor", CompactNodeTitle = "^"))
struct VOXELMETAGRAPH_API FVoxelNode_Bitwise_Xor : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, A, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, B, nullptr);
	VOXEL_MATH_OUTPUT_PIN(int32, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = {A} ^ {B}";
	}
};

USTRUCT(Category = "Math|Integer", DisplayName = "Bitwise NOT", meta = (Keywords = "~ not", CompactNodeTitle = "~"))
struct VOXELMETAGRAPH_API FVoxelNode_Bitwise_Not : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, A, nullptr);
	VOXEL_MATH_OUTPUT_PIN(int32, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = ~{A}";
	}
};

USTRUCT(Category = "Math|Conversions", DisplayName = "To Vector2D (IntPoint)", meta = (Keywords = "cast convert", CompactNodeTitle = "->", Autocast))
struct VOXELMETAGRAPH_API FVoxelNode_Conv_IntPointToVector2D : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FIntPoint, Vector, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FVector2D, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue}.x = {Vector}.x; {ReturnValue}.y = {Vector}.y";
	}
};

USTRUCT(Category = "Math|Conversions", DisplayName = "To Vector2D (IntVector)", meta = (Keywords = "cast convert", CompactNodeTitle = "->", Autocast))
struct VOXELMETAGRAPH_API FVoxelNode_Conv_IntVectorToVector2D : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FIntVector, Vector, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FVector2D, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue}.x = {Vector}.x; {ReturnValue}.y = {Vector}.y";
	}
};

USTRUCT(Category = "Math|Conversions", DisplayName = "To Vector (IntVector)", meta = (Keywords = "cast convert", CompactNodeTitle = "->", Autocast))
struct VOXELMETAGRAPH_API FVoxelNode_Conv_IntVectorToVector : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FIntVector, Vector, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FVector, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue}.x = {Vector}.x; {ReturnValue}.y = {Vector}.y; {ReturnValue}.z = {Vector}.z";
	}
};

USTRUCT(Category = "Math|Rotation")
struct VOXELMETAGRAPH_API FVoxelNode_MakeRotationFromEuler : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(float, Roll, nullptr);
	VOXEL_MATH_INPUT_PIN(float, Pitch, nullptr);
	VOXEL_MATH_INPUT_PIN(float, Yaw, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FQuat, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = MakeQuaternionFromEuler({Pitch}, {Yaw}, {Roll})";
	}
};

USTRUCT(Category = "Math|Rotation")
struct VOXELMETAGRAPH_API FVoxelNode_MakeRotationFromZ : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector, Z, nullptr);
	VOXEL_MATH_OUTPUT_PIN(FQuat, ReturnValue);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = MakeQuaternionFromZ({Z})";
	}
};