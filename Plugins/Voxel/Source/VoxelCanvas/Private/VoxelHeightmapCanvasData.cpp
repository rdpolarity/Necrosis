// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelHeightmapCanvasData.h"

DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelHeightmapCanvasMemory);

void FVoxelHeightmapCanvasData::Initialize(const FIntPoint& NewSize)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(Size == FIntPoint::ZeroValue) ||
		!ensure(0 < NewSize.X && NewSize.X < 4096) ||
		!ensure(0 < NewSize.Y && NewSize.Y < 4096))
	{
		return;
	}

	Size = NewSize;
	FVoxelUtilities::SetNumFast(Data, Size.X * Size.Y);

	UpdateStats();
}

void FVoxelHeightmapCanvasData::Serialize(FArchive& Ar)
{
	VOXEL_FUNCTION_COUNTER();

	using FVersion = DECLARE_VOXEL_VERSION
	(
		FirstVersion
	);

	int32 Version = FVersion::LatestVersion;
	Ar << Version;
	check(Version == FVersion::FirstVersion);

	Ar << Size;

	FVoxelUtilities::SetNumFast(Data, Size.X * Size.Y);
	Ar.Serialize(Data.GetData(), Size.X * Size.Y * Data.GetTypeSize());

	UpdateStats();
}