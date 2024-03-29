﻿// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelBlockAssetThumbnailRenderer.generated.h"

UCLASS()
class UVoxelBlockAssetThumbnailRenderer : public UVoxelStaticMeshThumbnailRenderer
{
	GENERATED_BODY()

public:
	//~ Begin UVoxelStaticMeshThumbnailRenderer Interface
	virtual UStaticMesh* GetStaticMesh(UObject* Object, TArray<UMaterialInterface*>& OutMaterialOverrides) const override;
	//~ End UVoxelStaticMeshThumbnailRenderer Interface
};