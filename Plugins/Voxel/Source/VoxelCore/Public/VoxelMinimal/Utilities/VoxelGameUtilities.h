// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"

struct VOXELCORE_API FVoxelGameUtilities
{
	static bool GetCameraView(const UWorld* World, FVector& OutPosition, FRotator& OutRotation, float& OutFOV);
	static bool GetCameraView(const UWorld* World, FVector& OutPosition)
	{
		FRotator Rotation;
		float FOV;
		return GetCameraView(World, OutPosition, Rotation, FOV);
	}
};