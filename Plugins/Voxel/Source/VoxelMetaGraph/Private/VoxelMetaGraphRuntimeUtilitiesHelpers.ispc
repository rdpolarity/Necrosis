// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMinimal.isph"

export void MetaGraph_WritePositions(
	uniform float DataX[],
	uniform float DataY[],
	uniform float DataZ[],
	const uniform int32 SizeX,
	const uniform int32 SizeY,
	const uniform int32 SizeZ,
	const uniform float StartX,
	const uniform float StartY,
	const uniform float StartZ,
	const uniform float Step)
{
	for (uniform int32 Z = 0; Z < SizeZ; Z++)
	{
		for (uniform int32 Y = 0; Y < SizeY; Y++)
		{
			const uniform int32 BaseIndex = SizeX * Y + SizeX * SizeY * Z;
			for (uniform int32 X = 0; X < SizeX; X++)
			{
				const uniform int32 Index = 8 * (BaseIndex + X);

				FOREACH(It, 0, 8)
				{
					DataX[Index + It] = StartX + (2 * X + ((It & 0x1) ? 1 : 0)) * Step;
				}
				FOREACH(It, 0, 8)
				{
					DataY[Index + It] = StartY + (2 * Y + ((It & 0x2) ? 1 : 0)) * Step;
				}
				FOREACH(It, 0, 8)
				{
					DataZ[Index + It] = StartZ + (2 * Z + ((It & 0x4) ? 1 : 0)) * Step;
				}
			}
		}
	}
}

export void MetaGraph_WritePositions2D(
	uniform float DataX[],
	uniform float DataY[],
	const uniform int32 SizeX,
	const uniform int32 SizeY,
	const uniform float StartX,
	const uniform float StartY,
	const uniform float Step)
{
	for (uniform int32 Y = 0; Y < SizeY; Y++)
	{
		for (uniform int32 X = 0; X < SizeX; X++)
		{
			const uniform int32 Index = 4 * (X + SizeX * Y);

			FOREACH(It, 0, 4)
			{
				DataX[Index + It] = StartX + (2 * X + ((It & 0x1) ? 1 : 0)) * Step;
			}
			FOREACH(It, 0, 4)
			{
				DataY[Index + It] = StartY + (2 * Y + ((It & 0x2) ? 1 : 0)) * Step;
			}
		}
	}
}

export void MetaGraph_ReplicatePacked(
	const uniform float Data[],
	uniform float OutData[],
	const uniform int32 Num)
{
	check(Num % 4 == 0);

	for (uniform int32 BaseIndex = 0; BaseIndex < Num; BaseIndex += 4)
	{
		const uniform int32 OutBaseIndex = 2 * BaseIndex;

		FOREACH(Index, 0, 4)
		{
			const varying float Value = Data[BaseIndex + Index];
			OutData[OutBaseIndex + 0 + Index] = Value;
			OutData[OutBaseIndex + 4 + Index] = Value;
		}
	}
}