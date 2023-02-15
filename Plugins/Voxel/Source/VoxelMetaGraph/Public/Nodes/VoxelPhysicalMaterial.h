// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelExposedPinType.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "VoxelPhysicalMaterial.generated.h"

USTRUCT(DisplayName = "Physical Material")
struct VOXELMETAGRAPH_API FVoxelPhysicalMaterial
{
	GENERATED_BODY()

	TWeakObjectPtr<UPhysicalMaterial> Material;
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelPhysicalMaterialPinType : public FVoxelExposedPinType
{
	GENERATED_BODY()

	DEFINE_VOXEL_EXPOSED_PIN_TYPE(FVoxelPhysicalMaterial, TSoftObjectPtr<UPhysicalMaterial>)
	{
		const TSharedRef<FVoxelPhysicalMaterial> Material = MakeShared<FVoxelPhysicalMaterial>();
		Material->Material = Value.LoadSynchronous();
		return Material;
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DECLARE_VOXEL_TERMINAL_BUFFER(FVoxelPhysicalMaterialBufferView, FVoxelPhysicalMaterialBuffer, FVoxelPhysicalMaterial, PF_R32G32_UINT);

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelPhysicalMaterialBufferView : public FVoxelTerminalBufferView
{
	GENERATED_BODY()
	GENERATED_VOXEL_TERMINAL_BUFFER_VIEW_BODY(FVoxelPhysicalMaterial);
};

USTRUCT(DisplayName = "Physical Material Buffer")
struct VOXELMETAGRAPH_API FVoxelPhysicalMaterialBuffer : public FVoxelTerminalBuffer
{
	GENERATED_BODY()
	GENERATED_VOXEL_TERMINAL_BUFFER_BODY(FVoxelPhysicalMaterial);
};