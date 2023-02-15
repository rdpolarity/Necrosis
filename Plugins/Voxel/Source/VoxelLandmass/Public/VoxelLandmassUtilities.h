// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLandmassBrush.h"
#include "VoxelLandmassBrushData.h"

struct FVoxelLandmassUtilities
{
	static constexpr float DefaultDistance = 1048576.f;

	static float SampleBrush(const FVoxelLandmassBrushImpl& Brush, const FVector3f& WorldPosition)
	{
		const FIntVector Size = Brush.Data->Size;
		const FVector3f FullFloatPosition = Brush.LocalToData.TransformPosition(WorldPosition);
		const FVector3f ClampedFloatPosition = ClampVector(FullFloatPosition, FVector3f(ForceInit), FVector3f(Size) - 1.f - 0.0001f);
		const FIntVector Position = FVoxelUtilities::FloorToInt(ClampedFloatPosition);
		const FVector3f Alpha = ClampedFloatPosition - FVector3f(Position);

		checkVoxelSlow(0 <= Position.X && Position.X + 1 < Size.X);
		checkVoxelSlow(0 <= Position.Y && Position.Y + 1 < Size.Y);
		checkVoxelSlow(0 <= Position.Z && Position.Z + 1 < Size.Z);

		const uint32 X = Position.X;
		const uint32 Y = Position.Y;
		const uint32 Z = Position.Z;

		const uint32 SizeX = Size.X;
		const uint32 SizeXSizeY = Size.X * Size.Y;
		
		const float* RESTRICT DistanceField = Brush.Data->DistanceField.GetData();

		const float Distance000 = DistanceField[(X + 0) + SizeX * (Y + 0) + SizeXSizeY * (Z + 0)];
		const float Distance100 = DistanceField[(X + 1) + SizeX * (Y + 0) + SizeXSizeY * (Z + 0)];
		const float Distance010 = DistanceField[(X + 0) + SizeX * (Y + 1) + SizeXSizeY * (Z + 0)];
		const float Distance110 = DistanceField[(X + 1) + SizeX * (Y + 1) + SizeXSizeY * (Z + 0)];
		const float Distance001 = DistanceField[(X + 0) + SizeX * (Y + 0) + SizeXSizeY * (Z + 1)];
		const float Distance101 = DistanceField[(X + 1) + SizeX * (Y + 0) + SizeXSizeY * (Z + 1)];
		const float Distance011 = DistanceField[(X + 0) + SizeX * (Y + 1) + SizeXSizeY * (Z + 1)];
		const float Distance111 = DistanceField[(X + 1) + SizeX * (Y + 1) + SizeXSizeY * (Z + 1)];

		float Distance = FVoxelUtilities::TrilinearInterpolation<float>(
			Distance000,
			Distance100,
			Distance010,
			Distance110,
			Distance001,
			Distance101,
			Distance011,
			Distance111,
			Alpha.X,
			Alpha.Y,
			Alpha.Z);

		Distance += FVector3f::Distance(FullFloatPosition, ClampedFloatPosition);

		// Put the distance in voxels
		Distance *= Brush.DataToLocalScale;
		Distance -= Brush.Brush.DistanceOffset;

		if (Brush.Brush.bInvert)
		{
			Distance = -Distance;
		}

		return Distance;
	}
};