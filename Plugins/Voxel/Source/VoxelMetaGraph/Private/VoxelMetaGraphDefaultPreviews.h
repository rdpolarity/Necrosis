// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMetaGraphPreview.h"
#include "VoxelMetaGraphDefaultPreviews.generated.h"

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelMetaGraphPreviewFactory_FVoxelFloatBuffer : public FVoxelMetaGraphPreviewFactory_Texture
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()
		
	virtual void Create(const FParameters& Parameters) const override;
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelMetaGraphPreviewFactory_DensityBuffer : public FVoxelMetaGraphPreviewFactory_Texture
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	virtual bool CanCreate(const FVoxelPinType& Type) const override;
	virtual void Create(const FParameters& Parameters) const override;
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelMetaGraphPreviewFactory_FVoxelVector2DBuffer : public FVoxelMetaGraphPreviewFactory_Texture
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()
		
	virtual void Create(const FParameters& Parameters) const override;
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelMetaGraphPreviewFactory_FVoxelVectorBuffer : public FVoxelMetaGraphPreviewFactory_Texture
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()
		
	virtual void Create(const FParameters& Parameters) const override;
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelMetaGraphPreviewFactory_FVoxelLinearColorBuffer : public FVoxelMetaGraphPreviewFactory_Texture
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()
		
	virtual void Create(const FParameters& Parameters) const override;
};