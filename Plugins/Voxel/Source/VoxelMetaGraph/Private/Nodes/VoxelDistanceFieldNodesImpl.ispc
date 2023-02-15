// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphImpl.isph"

export void VoxelNode_ComputeDensityFromDistanceField_Merge(
	uniform float OutDistances[],
	const uniform float Distances[],
	const uniform bool bConstDistances,
	const uniform bool bSubtractive,
	const uniform float Smoothness,
	const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const varying float Distance = bConstDistances ? Distances[0] : Distances[Index];

		if (bSubtractive)
		{
			OutDistances[Index] = SmoothSubtraction(OutDistances[Index], -Distance, Smoothness);
		}
		else
		{
			OutDistances[Index] = SmoothMin(OutDistances[Index], Distance, Smoothness);
		}
	}
}