// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMesh/VoxelMesh.h"
#include "VoxelBlockMesh.generated.h"

struct FVoxelMeshMaterial;

USTRUCT()
struct VOXELBLOCK_API FVoxelBlockMesh : public FVoxelMesh
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	int32 LOD = 0;
	FBox Bounds = FBox(ForceInit);
	TSharedPtr<const FVoxelMeshMaterial> MeshMaterial;
	
	FRawStaticIndexBuffer IndexBuffer{ false };

	FPositionVertexBuffer PositionVertexBuffer;
	// Holds UVs + tangents/normals
	FStaticMeshVertexBuffer StaticMeshVertexBuffer;
	FColorVertexBuffer ColorVertexBuffer;

public:
	virtual FBox GetBounds() const override { return Bounds; }
	virtual int64 GetAllocatedSize() const override;
	virtual TSharedPtr<FVoxelMaterialRef> GetMaterial() const override { return Material; }

	virtual void Initialize_GameThread() override;

	virtual void Initialize_RenderThread(ERHIFeatureLevel::Type FeatureLevel) override;
	virtual void Destroy_RenderThread() override;

	virtual bool Draw_RenderThread(const FPrimitiveSceneProxy& Proxy, FMeshBatch& MeshBatch) const override;
	virtual const FRayTracingGeometry* DrawRaytracing_RenderThread(const FPrimitiveSceneProxy& Proxy, FMeshBatch& MeshBatch) const override;

private:
	TSharedPtr<FLocalVertexFactory> VertexFactory;
	TSharedPtr<FRayTracingGeometry> RayTracingGeometry;
	TSharedPtr<FVoxelMaterialRef> Material;
};