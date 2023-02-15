// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMinimal.h"

FVoxelBVH_Chaos::FVoxelBVH_Chaos(const TVoxelArray<FVoxelBox>& AllBounds)
	: Tree(ReinterpretCastArray<FWrapper>(AllBounds))
{
}