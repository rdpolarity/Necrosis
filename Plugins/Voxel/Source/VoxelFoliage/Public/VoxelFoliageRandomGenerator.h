// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

class VOXELFOLIAGE_API FVoxelFoliageRandomGenerator
{
private:
	uint64 InitialSeedHash = 0;
	FVector3f InstancePosition = FVector3f::ZeroVector;

	FRandomStream MeshSelection;

	FRandomStream Scale;
	FRandomStream YawRotation;
	FRandomStream PitchRotation;
	FRandomStream RandomAlignRotation;

	FRandomStream HeightRestriction;
	FRandomStream MaskRestriction;

	FRandomStream InstancesCount;
	FRandomStream SpawnRadius;
	FRandomStream RadialOffset;

public:
	FVoxelFoliageRandomGenerator();
	explicit FVoxelFoliageRandomGenerator(const uint64 SeedHash, const FVector3f& Position);

	void Reset();

#define GET_FRACTION(Name) \
	float Get ## Name ## Fraction() const \
	{ \
		return Name.GetFraction(); \
	}

	GET_FRACTION(MeshSelection)

	GET_FRACTION(Scale)
	GET_FRACTION(YawRotation)
	GET_FRACTION(PitchRotation)

	FVector3f GetRandomAlignUnitVector() const
	{
		return FVector3f(RandomAlignRotation.GetUnitVector());
	}

	GET_FRACTION(HeightRestriction)
	GET_FRACTION(MaskRestriction)

	GET_FRACTION(InstancesCount)
	GET_FRACTION(SpawnRadius)

	GET_FRACTION(RadialOffset)

#undef GET_FRACTION
};