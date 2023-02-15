// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelCollider.generated.h"

DECLARE_VOXEL_MEMORY_STAT(VOXELRUNTIME_API, STAT_VoxelColliderMemory, "Voxel Collider Memory");

class IVoxelColliderRenderData
{
public:
	IVoxelColliderRenderData() = default;
	virtual ~IVoxelColliderRenderData() = default;
	
	virtual void Draw_RenderThread(const FPrimitiveSceneProxy& Proxy, FMeshBatch& MeshBatch) = 0;
};

USTRUCT()
struct VOXELRUNTIME_API FVoxelCollider : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()
	
	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelColliderMemory);

	virtual FBox GetBounds() const VOXEL_PURE_VIRTUAL({});
	virtual void AddToBodySetup(UBodySetup& BodySetup) const VOXEL_PURE_VIRTUAL();
	virtual TSharedPtr<IVoxelColliderRenderData> GetRenderData() const { return nullptr; }
	virtual TArray<TWeakObjectPtr<UPhysicalMaterial>> GetPhysicalMaterials() const { return {}; }
};