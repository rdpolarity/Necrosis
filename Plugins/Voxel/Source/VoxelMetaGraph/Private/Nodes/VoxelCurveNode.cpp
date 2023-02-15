// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelCurveNode.h"

DEFINE_VOXEL_NODE(FVoxelNode_SampleCurve, Result)
{
	const TValue<FVoxelCurveData> Curve = Get(CurvePin, Query);

	if (AreMathPinsBuffers())
	{
		const TValue<TBufferView<float>> Value = GetBufferView<float>(ValuePin, Query);

		return VOXEL_ON_COMPLETE(AsyncThread, Curve, Value)
		{
			if (!Curve->Curve)
			{
				VOXEL_MESSAGE(Error, "{0}: Curve is null", this);
				return {};
			}

			TVoxelArray<float> ReturnValue = FVoxelFloatBuffer::Allocate(Value.Num());
	
			for (int32 Index = 0; Index < Value.Num(); Index++)
			{
				ReturnValue[Index] = Curve->Curve->Eval(Value[Index]);
			}

			return FVoxelSharedPinValue::Make(FVoxelFloatBuffer::MakeCpu(ReturnValue));
		};
	}
	else
	{
		const TValue<float> Value = Get<float>(ValuePin, Query);

		return VOXEL_ON_COMPLETE(AsyncThread, Curve, Value)
		{
			return FVoxelSharedPinValue::Make(Curve->Curve->Eval(Value));
		};
	}
}