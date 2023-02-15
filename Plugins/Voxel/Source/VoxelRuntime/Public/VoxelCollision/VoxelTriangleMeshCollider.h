// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelCollision/VoxelCollider.h"
#include "Chaos/TriangleMeshImplicitObject.h"
#include "VoxelTriangleMeshCollider.generated.h"

USTRUCT()
struct VOXELRUNTIME_API FVoxelTriangleMeshCollider : public FVoxelCollider
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	FBox Bounds = FBox(ForceInit);
	TArray<TSharedPtr<Chaos::FTriangleMeshImplicitObject>> TriangleMeshes;
	TArray<TWeakObjectPtr<UPhysicalMaterial>> PhysicalMaterials;

	virtual FBox GetBounds() const override { return Bounds; }
	virtual int64 GetAllocatedSize() const override;
	virtual void AddToBodySetup(UBodySetup& BodySetup) const override;
	virtual TSharedPtr<IVoxelColliderRenderData> GetRenderData() const override;
	virtual TArray<TWeakObjectPtr<UPhysicalMaterial>> GetPhysicalMaterials() const override;
};

class VOXELRUNTIME_API FVoxelTriangleMeshCollider_RenderData : public IVoxelColliderRenderData
{
public:
	explicit FVoxelTriangleMeshCollider_RenderData(const FVoxelTriangleMeshCollider& Collider);
	virtual ~FVoxelTriangleMeshCollider_RenderData() override;

	//~ Begin IVoxelColliderRenderData Interface
	virtual void Draw_RenderThread(const FPrimitiveSceneProxy& Proxy, FMeshBatch& MeshBatch) override;
	//~ End IVoxelColliderRenderData Interface

private:
	FRawStaticIndexBuffer IndexBuffer{ false };
	FPositionVertexBuffer PositionVertexBuffer;
	FStaticMeshVertexBuffer StaticMeshVertexBuffer;
	FColorVertexBuffer ColorVertexBuffer;
	TUniquePtr<FLocalVertexFactory> VertexFactory;
};