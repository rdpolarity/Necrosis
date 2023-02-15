// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMinimal.isph"

// ReSharper disable CppCStyleCast

export void SampleLandmassHeightmapBrush(
	const uniform float ArrayPositionX[],
	const uniform bool bConstPositionX,
	const uniform float ArrayPositionY[],
	const uniform bool bConstPositionY,
	const uniform float BrushPositionX,
	const uniform float BrushPositionY,
	const uniform float BrushRotationX,
	const uniform float BrushRotationY,
	const uniform float BrushRotation3DX,
	const uniform float BrushRotation3DY,
	const uniform float BrushScaleX,
	const uniform float BrushScaleY,
	const uniform float BrushOuterSlope,
	const uniform float BrushInnerScaleZ,
	const uniform float BrushInnerOffsetZ,
	const uniform float BrushScaleZ,
	const uniform float BrushOffsetZ,
	const uniform float BrushSmoothness,
	const uniform bool bSubtractive,
	const uniform int32 SizeX,
	const uniform int32 SizeY,
	const uniform uint16 Heightmap[],
	const uniform int32 Num,
	uniform float Heights[])
{
	FOREACH(Index, 0, Num)
	{
		varying float PositionX = bConstPositionX ? ArrayPositionX[0] : ArrayPositionX[Index];
		varying float PositionY = bConstPositionY ? ArrayPositionY[0] : ArrayPositionY[Index];

		// Same logic as FVoxelLandmassHeightmapBrush::GetHeight

		PositionX -= BrushPositionX;
		PositionY -= BrushPositionY;

		const varying float Rotation3DHeight = PositionX * BrushRotation3DX + PositionY * BrushRotation3DY;

		{
			const varying float PrevPositionX = PositionX;
			const varying float PrevPositionY = PositionY;

			PositionX = PrevPositionX * BrushRotationX - PrevPositionY * BrushRotationY;
			PositionY = PrevPositionX * BrushRotationY + PrevPositionY * BrushRotationX;
		}

		PositionX /= BrushScaleX;
		PositionY /= BrushScaleY;

		PositionX += SizeX / 2.f;
		PositionY += SizeY / 2.f;

		const float MinXf = clamp(floor(PositionX), 0.f, SizeX - 2.f);
		const float MinYf = clamp(floor(PositionY), 0.f, SizeY - 2.f);

		const float AlphaX = clamp(PositionX - MinXf, 0.f, 1.f);
		const float AlphaY = clamp(PositionY - MinYf, 0.f, 1.f);

		const int32 MinX = (int32)MinXf;
		const int32 MinY = (int32)MinYf;
		const int32 MaxX = MinX + 1;
		const int32 MaxY = MinY + 1;

		float Height = BilinearInterpolation(
			IGNORE_PERF_WARNING
			Heightmap[MinX + SizeX * MinY],
			IGNORE_PERF_WARNING
			Heightmap[MaxX + SizeX * MinY],
			IGNORE_PERF_WARNING
			Heightmap[MinX + SizeX * MaxY],
			IGNORE_PERF_WARNING
			Heightmap[MaxX + SizeX * MaxY],
			AlphaX,
			AlphaY) / MAX_uint16;

		const float Distance = length(MakeFloat2(
			MinXf + AlphaX - PositionX,
			MinYf + AlphaY - PositionY));

		Height *= BrushInnerScaleZ;
		Height -= BrushOuterSlope * Distance;
		Height += BrushInnerOffsetZ;

		if (bSubtractive)
		{
			Height *= -1;
		}

		Height -= Rotation3DHeight;

		Height *= BrushScaleZ;
		Height += BrushOffsetZ;

		if (bSubtractive)
		{
			Heights[Index] = SmoothMin(Heights[Index], Height, BrushSmoothness);
		}
		else
		{
			Heights[Index] = SmoothMax(Heights[Index], Height, BrushSmoothness);
		}
	}
}