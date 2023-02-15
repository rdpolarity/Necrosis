// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphImpl.isph"

FORCEINLINE void VoxelBufferUtilities_GetGradient_Impl(
	const uniform float Values[],
	const uniform int32 Num,
	const uniform float Step,
	const uniform int32 Stride,
	uniform float OutValues[])
{
	check(Num % programCount == 0);

	for (uniform int32 BlockIndex = 0; BlockIndex < Num; BlockIndex += programCount)
	{
		const varying int32 Index = BlockIndex + programIndex;
		const varying float Value = Values[Index];

		varying float OutValue = 0.f;

#if TARGET_WIDTH == 8
		if (Stride == 1)
		{
			const varying int32 PermA = MakeRegister(0, 0, 2, 2, 4, 4, 6, 6);
			const varying int32 PermB = MakeRegister(1, 1, 3, 3, 5, 5, 7, 7);
			OutValue = shuffle(Value, PermB) - shuffle(Value, PermA);
		}
		else if (Stride == 2)
		{
			const varying int32 PermA = MakeRegister(0, 1, 0, 1, 4, 5, 4, 5);
			const varying int32 PermB = MakeRegister(2, 3, 2, 3, 6, 7, 6, 7);
			OutValue = shuffle(Value, PermB) - shuffle(Value, PermA);
		}
		else
		{
			check(Stride == 4);
			const varying int32 PermA = MakeRegister(0, 1, 2, 3, 0, 1, 2, 3);
			const varying int32 PermB = MakeRegister(4, 5, 6, 7, 4, 5, 6, 7);
			OutValue = shuffle(Value, PermB) - shuffle(Value, PermA);
		}
#else
		check(false);
#endif

		OutValues[Index] = OutValue / Step;
	}
}

export void VoxelBufferUtilities_GetGradient(
	const uniform float Values[],
	const uniform int32 Num,
	const uniform float Step,
	const uniform int32 Stride,
	uniform float OutValues[])
{
	check(Num % programCount == 0);

	if (Stride == 1)
	{
		VoxelBufferUtilities_GetGradient_Impl(Values, Num, Step, 1, OutValues);
	}
	else if (Stride == 2)
	{
		VoxelBufferUtilities_GetGradient_Impl(Values, Num, Step, 2, OutValues);
	}
	else
	{
		check(Stride == 4);
		VoxelBufferUtilities_GetGradient_Impl(Values, Num, Step, 4, OutValues);
	}
}

export void VoxelBufferUtilities_GetGradientCollapse(
	const uniform float Values[],
	const uniform int32 Num,
	const uniform float Step,
	uniform float OutValues[])
{
	check(Num % programCount == 0);

	for (uniform int32 BlockIndex = 0; BlockIndex < Num; BlockIndex += programCount)
	{
		const varying int32 Index = BlockIndex + programIndex;
		const varying float Value = Values[Index];

#if TARGET_WIDTH == 8
		const varying int32 PermA = MakeRegister(0, 2, 4, 6, 0, 0, 0, 0);
		const varying int32 PermB = MakeRegister(1, 3, 5, 7, 0, 0, 0, 0);
		const varying float Result = (shuffle(Value, PermB) - shuffle(Value, PermA)) / Step;

		OutValues[BlockIndex / 2 + 0] = extract(Result, 0);
		OutValues[BlockIndex / 2 + 1] = extract(Result, 1);
		OutValues[BlockIndex / 2 + 2] = extract(Result, 2);
		OutValues[BlockIndex / 2 + 3] = extract(Result, 3);
#else
		check(false);
#endif
	}
}

export void VoxelBufferUtilities_SplitGradientBuffer(
	const uniform float Values[],
	const uniform int32 Num,
	const uniform float HalfStep,
	uniform float OutValues[])
{
	FOREACH(Index, 0, Num)
	{
		IGNORE_PERF_WARNING
		OutValues[2 * Index + 0] = Values[Index] - HalfStep;
		IGNORE_PERF_WARNING
		OutValues[2 * Index + 1] = Values[Index] + HalfStep;
	}
}