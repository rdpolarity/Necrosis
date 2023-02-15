// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelFoliageTemplatesData.h"
#include "VoxelFoliageRandomGenerator.h"

FVector3f FVoxelFoliageScaleSettings::GetScale(const FVoxelFoliageRandomGenerator& RandomGenerator) const
{
	FVector3f Scale;
	switch (Scaling)
	{
	default: check(false);
	case EVoxelFoliageScaling::Uniform:
	{
		Scale.X = ScaleX.Interpolate(RandomGenerator.GetScaleFraction());
		Scale.Y = Scale.X;
		Scale.Z = Scale.X;
		break;
	}
	case EVoxelFoliageScaling::Free:
	{
		Scale.X = ScaleX.Interpolate(RandomGenerator.GetScaleFraction());
		Scale.Y = ScaleY.Interpolate(RandomGenerator.GetScaleFraction());
		Scale.Z = ScaleZ.Interpolate(RandomGenerator.GetScaleFraction());
		break;
	}
	case EVoxelFoliageScaling::LockXY:
	{
		Scale.X = ScaleX.Interpolate(RandomGenerator.GetScaleFraction());
		Scale.Y = Scale.X;
		Scale.Z = ScaleZ.Interpolate(RandomGenerator.GetScaleFraction());
		break;
	}
	}

	return Scale;
}

bool FVoxelFoliageSpawnRestriction::CanSpawn(const FVoxelFoliageRandomGenerator& RandomGenerator, const FVector3f& Position, const FVector3f& Normal, const FVector3f& WorldUp) const
{
	if (bEnableHeightRestriction)
	{
		if (!HeightRestriction.Contains(Position.Z))
		{
			return false;
		}

		const float Center = (HeightRestriction.Min + HeightRestriction.Max) / 2.f;
		const float Radius = HeightRestriction.Size() / 2.f;
		const float Distance = FMath::Abs(Center - Position.Z);
		const float Falloff = FMath::Min(HeightRestrictionFalloff, Radius);

		const float Alpha = FVoxelUtilities::SmoothFalloff(Distance, Radius - Falloff, Falloff);

		if (RandomGenerator.GetHeightRestrictionFraction() >= Alpha)
		{
			return false;
		}
	}

	if (bEnableSlopeRestriction)
	{
		const float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector3f::DotProduct(Normal, WorldUp)));
		if (!GroundSlopeAngle.Contains(Angle))
		{
			return false;
		}
	}

	return true;
}