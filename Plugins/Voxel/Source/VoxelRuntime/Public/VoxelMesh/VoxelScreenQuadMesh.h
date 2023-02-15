﻿// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMesh/VoxelMesh.h"
#include "VoxelScreenQuadMesh.generated.h"

USTRUCT()
struct VOXELRUNTIME_API FVoxelScreenQuadMesh : public FVoxelMesh
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	TSharedPtr<FVoxelMaterialRef> Material;

	virtual FBox GetBounds() const override;
	virtual int64 GetAllocatedSize() const override;
	virtual TSharedPtr<FVoxelMaterialRef> GetMaterial() const override;

	virtual void Initialize_RenderThread(ERHIFeatureLevel::Type FeatureLevel) override;
	virtual void Destroy_RenderThread() override;

	virtual bool Draw_RenderThread(const FPrimitiveSceneProxy& Proxy, FMeshBatch& MeshBatch) const override;

	virtual bool ShouldDrawVelocity() const override { return false; }

private:
	TSharedPtr<FLocalVertexFactory> VertexFactory;
};