// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelFoliageRandomGenerator.h"

FVoxelFoliageRandomGenerator::FVoxelFoliageRandomGenerator()
{
	Reset();
}

FVoxelFoliageRandomGenerator::FVoxelFoliageRandomGenerator(const uint64 SeedHash, const FVector3f& Position)
	: InitialSeedHash(SeedHash)
	, InstancePosition(Position)
{
	Reset();
}

#define INIT_STREAM(Name) \
	Seed = FVoxelUtilities::MurmurHash64(Seed); \
	Name.Initialize(Seed)

void FVoxelFoliageRandomGenerator::Reset()
{
	uint64 Seed = InitialSeedHash ^ FVoxelUtilities::MurmurHash(InstancePosition);

	INIT_STREAM(MeshSelection);

	INIT_STREAM(Scale);
	INIT_STREAM(YawRotation);
	INIT_STREAM(PitchRotation);
	INIT_STREAM(RandomAlignRotation);

	INIT_STREAM(HeightRestriction);
	INIT_STREAM(MaskRestriction);

	INIT_STREAM(RadialOffset);

	INIT_STREAM(InstancesCount);
	INIT_STREAM(SpawnRadius);
}

#undef INIT_STREAM