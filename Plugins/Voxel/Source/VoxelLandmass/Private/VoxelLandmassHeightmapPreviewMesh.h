// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelHeightmap.h"
#include "VoxelMesh/VoxelMesh.h"
#include "VoxelLandmassHeightmapPreviewMesh.generated.h"

USTRUCT()
struct FVoxelLandmassHeightmapPreviewMesh : public FVoxelMesh
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	FBox Bounds = FBox(ForceInit);
	
	FRawStaticIndexBuffer IndexBuffer{ false };
	FPositionVertexBuffer PositionVertexBuffer;
	FStaticMeshVertexBuffer StaticMeshVertexBuffer;
	FColorVertexBuffer ColorVertexBuffer;

	void Initialize(const UVoxelHeightmap& Heightmap);

	virtual FBox GetBounds() const override { return Bounds; }
	virtual int64 GetAllocatedSize() const override;
	virtual TSharedPtr<FVoxelMaterialRef> GetMaterial() const override { return Material; }

	virtual void Initialize_GameThread() override;
	virtual void Initialize_RenderThread(ERHIFeatureLevel::Type FeatureLevel) override;
	virtual void Destroy_RenderThread() override;
	virtual bool Draw_RenderThread(const FPrimitiveSceneProxy& Proxy, FMeshBatch& MeshBatch) const override;

private:
	TSharedPtr<FLocalVertexFactory> VertexFactory;
	TSharedPtr<FVoxelMaterialRef> Material;
};