// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelExposedPinType.h"

FVoxelPinValue FVoxelExposedPinType::Compute(const FVoxelPinValue& Value, FVoxelRuntime& InRuntime) const
{
	FVoxelPinValue OutValue(GetComputedType().WithoutTag());
	if (!ensure(Value.GetType().IsDerivedFrom(GetExposedType().WithoutTag())))
	{
		return OutValue;
	}

	ensure(!Runtime);
	Runtime = &InRuntime;
	ComputeImpl(OutValue, Value);
	Runtime = nullptr;

	return OutValue;
}