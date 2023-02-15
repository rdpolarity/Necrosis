// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphImpl.isph"

export void VoxelFoliageAssetNodes_SplitGradientBuffer(
    const uniform float PositionsX[],
    const uniform float PositionsY[],
    const uniform int32 Num,
    const uniform float HalfStep,
    uniform float OutPositionsX[],
    uniform float OutPositionsY[])
{
    FOREACH(Index, 0, Num)
    {
        IGNORE_PERF_WARNING
        OutPositionsX[4 * Index + 0] = PositionsX[Index] - HalfStep;
        IGNORE_PERF_WARNING
        OutPositionsX[4 * Index + 1] = PositionsX[Index] + HalfStep;
        IGNORE_PERF_WARNING
        OutPositionsX[4 * Index + 2] = PositionsX[Index] - HalfStep;
        IGNORE_PERF_WARNING
        OutPositionsX[4 * Index + 3] = PositionsX[Index] + HalfStep;
        
        IGNORE_PERF_WARNING
        OutPositionsY[4 * Index + 0] = PositionsY[Index] - HalfStep;
        IGNORE_PERF_WARNING
        OutPositionsY[4 * Index + 1] = PositionsY[Index] - HalfStep;
        IGNORE_PERF_WARNING
        OutPositionsY[4 * Index + 2] = PositionsY[Index] + HalfStep;
        IGNORE_PERF_WARNING
        OutPositionsY[4 * Index + 3] = PositionsY[Index] + HalfStep;
    }
}

export void VoxelFoliageAssetNodes_GetGradientCollapse(
	const uniform float Values[],
	const uniform int32 Num,
	const uniform float Step,
	uniform float OutValuesX[],
	uniform float OutValuesY[],
	uniform float OutValuesZ[])
{
	check(Num % programCount == 0);

	for (uniform int32 BlockIndex = 0; BlockIndex < Num; BlockIndex += programCount)
	{
		const varying int32 Index = BlockIndex + programIndex;
		const varying float Value = Values[Index];

#if TARGET_WIDTH == 8
		const varying int32 PermA = MakeRegister(0, 2, 0, 1, 4, 6, 4, 5);
		const varying int32 PermB = MakeRegister(1, 3, 2, 3, 5, 7, 6, 7);
		const varying float Result = shuffle(Value, PermB) - shuffle(Value, PermA);

		const varying int32 PermC = MakeRegister(0, 2, 4, 6, 0, 0, 0, 0);
		const varying int32 PermD = MakeRegister(1, 3, 5, 7, 0, 0, 0, 0);
		const varying float Result2 = shuffle(Result, PermC) + shuffle(Result, PermD);

		OutValuesX[BlockIndex / 4 + 0] = extract(Result2, 0);
		OutValuesY[BlockIndex / 4 + 0] = extract(Result2, 1);
		OutValuesX[BlockIndex / 4 + 1] = extract(Result2, 2);
		OutValuesY[BlockIndex / 4 + 1] = extract(Result2, 3);
#else
		check(false);
#endif
	}

    FOREACH(Index, 0, Num / 4)
    {
        const varying float X = -OutValuesX[Index] / Step / 2;
        const varying float Y = -OutValuesY[Index] / Step / 2;
        const varying float Z = 1;
        const varying float3 Vector = normalize(MakeFloat3(X, Y, Z));

        OutValuesX[Index] = Vector.x;
        OutValuesY[Index] = Vector.y;
        OutValuesZ[Index] = Vector.z;
    }
}