// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelExposedPinType.h"
#include "VoxelNodeTypes.generated.h"

USTRUCT(DisplayName = "Static Mesh")
struct VOXELMETAGRAPH_API FVoxelStaticMesh
{
	GENERATED_BODY()

	TWeakObjectPtr<UStaticMesh> StaticMesh;
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelStaticMeshPinType : public FVoxelExposedPinType
{
	GENERATED_BODY()

	DEFINE_VOXEL_EXPOSED_PIN_TYPE(FVoxelStaticMesh, TSoftObjectPtr<UStaticMesh>)
	{
		const TSharedRef<FVoxelStaticMesh> Mesh = MakeShared<FVoxelStaticMesh>();
		Mesh->StaticMesh = Value.LoadSynchronous();
		return Mesh;
	}
};