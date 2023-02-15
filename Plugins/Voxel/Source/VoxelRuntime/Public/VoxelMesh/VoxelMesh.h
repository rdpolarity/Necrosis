// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMesh.generated.h"

DECLARE_VOXEL_MEMORY_STAT(VOXELRUNTIME_API, STAT_VoxelMeshMemory, "Voxel Mesh Memory");

USTRUCT()
struct VOXELRUNTIME_API FVoxelMesh : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	virtual ~FVoxelMesh() override;
	
	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelMeshMemory);

	void CallInitialize_GameThread() const;
	void CallInitialize_RenderThread(ERHIFeatureLevel::Type FeatureLevel) const;

	TSharedRef<FVoxelMaterialRef> GetMaterialSafe() const;

public:
	virtual FBox GetBounds() const VOXEL_PURE_VIRTUAL({});
	virtual int64 GetAllocatedSize() const override VOXEL_PURE_VIRTUAL({});
	virtual TSharedPtr<FVoxelMaterialRef> GetMaterial() const VOXEL_PURE_VIRTUAL({});
	virtual FVector GetScale() const { return FVector::OneVector; }

	virtual bool Draw_RenderThread(const FPrimitiveSceneProxy& Proxy, FMeshBatch& MeshBatch) const { return false; }
	virtual const FRayTracingGeometry* DrawRaytracing_RenderThread(const FPrimitiveSceneProxy& Proxy, FMeshBatch& MeshBatch) const { return nullptr; }

	virtual const FCardRepresentationData* GetCardRepresentationData() const { return nullptr; }
	virtual const FDistanceFieldVolumeData* GetDistanceFieldVolumeData() const { return nullptr; }

	virtual bool ShouldDrawVelocity() const { return true; }

protected:
	virtual void Initialize_GameThread() {}
	virtual void Initialize_RenderThread(ERHIFeatureLevel::Type FeatureLevel) {}
	virtual void Destroy_RenderThread() {}

private:
	mutable bool bIsInitialized_GameThread = false;
	mutable bool bIsInitialized_RenderThread = false;
	bool bIsDestroyed_RenderThread = false;

	template<typename T>
	friend TSharedRef<T> MakeVoxelMesh();
};

template<typename T>
TSharedRef<T> MakeVoxelMesh()
{
	return TSharedPtr<T>(new T(), [](T* Mesh)
	{
		if (!Mesh)
		{
			return;
		}

		ENQUEUE_RENDER_COMMAND(DeleteVoxelMesh)([=](FRHICommandListImmediate& RHICmdList)
		{
			if (Mesh->bIsInitialized_RenderThread &&
				ensure(!Mesh->bIsDestroyed_RenderThread))
			{
				Mesh->Destroy_RenderThread();
			}
			Mesh->bIsDestroyed_RenderThread = true;

			delete Mesh;
		});
	}).ToSharedRef();
}