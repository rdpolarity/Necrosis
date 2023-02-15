// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "Misc/Guid.h"

struct VOXELFOLIAGE_API FVoxelFoliageObjectVersion
{
	enum Type
	{
		BeforeCustomVersionWasAdded = 0,
		FoliageInstanceStructureChanged,

		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version number
	const static FGuid GUID;

private:
	FVoxelFoliageObjectVersion() {}
};