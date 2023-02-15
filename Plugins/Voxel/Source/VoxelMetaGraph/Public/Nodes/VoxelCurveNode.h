// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelExposedPinType.h"
#include "VoxelCurveNode.generated.h"

USTRUCT(DisplayName = "Curve")
struct VOXELMETAGRAPH_API FVoxelCurveData
{
	GENERATED_BODY()

	TSharedPtr<const FRichCurve> Curve;
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelCurveDataPinType : public FVoxelExposedPinType
{
	GENERATED_BODY()

	DEFINE_VOXEL_EXPOSED_PIN_TYPE(FVoxelCurveData, TSoftObjectPtr<UCurveFloat>)
	{
		const UCurveFloat* Curve = Value.LoadSynchronous();
		if (!Curve)
		{
			return {};
		}

		const TSharedRef<FVoxelCurveData> CurveData = MakeShared<FVoxelCurveData>();
		CurveData->Curve = MakeSharedCopy(Curve->FloatCurve);
		return CurveData;
	}
};

USTRUCT(Category = "Math|Curve")
struct VOXELMETAGRAPH_API FVoxelNode_SampleCurve : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelCurveData, Curve, nullptr);
	VOXEL_MATH_INPUT_PIN(float, Value, nullptr);
	VOXEL_MATH_OUTPUT_PIN(float, Result);
};