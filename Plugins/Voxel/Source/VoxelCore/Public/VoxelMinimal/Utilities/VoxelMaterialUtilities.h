// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;

struct VOXELCORE_API FVoxelMaterialUtilities
{
	static float GetMaterialTime(UWorld* World);

	static void SetMatrixParameter(
		UMaterialInstanceDynamic& MaterialInstance,
		const FString& ParameterName,
		const FMatrix& Matrix);
};