// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelSurfaceNodes.generated.h"

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelSurface : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelSurfaceQueryData : public FVoxelQueryData
{
	GENERATED_BODY()
	GENERATED_VOXEL_QUERY_DATA_BODY()

public:
	TSharedPtr<const FVoxelSurface> Surface;

	uint64 GetHash() const
	{
		return FVoxelUtilities::MurmurHash(Surface.Get());
	}
	bool Identical(const FVoxelSurfaceQueryData& Other) const
	{
		return Surface == Other.Surface;
	}
};