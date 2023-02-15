// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelFoliageObjectVersion.h"

const FGuid FVoxelFoliageObjectVersion::GUID(0x2BE57310, 0x1A0F97DA, 0x04A3F8AD, 0x15B32E18);
FCustomVersionRegistration GRegisterVoxelFoliageVersion(FVoxelFoliageObjectVersion::GUID, FVoxelFoliageObjectVersion::LatestVersion, TEXT("Voxel-Foliage"));