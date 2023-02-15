// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/MarchingCube/VoxelMarchingCubeMesh_LocalVF.h"
#include "Nodes/VoxelMeshMaterialNodes.h"

int64 FVoxelMarchingCubeMesh_LocalVF::GetAllocatedSize() const
{
	return
		IndexBuffer.GetAllocatedSize() +
		PositionVertexBuffer.GetNumVertices() * PositionVertexBuffer.GetStride() +
		StaticMeshVertexBuffer.GetResourceSize() +
		ColorVertexBuffer.GetNumVertices() * ColorVertexBuffer.GetStride();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMarchingCubeMesh_LocalVF::Initialize_GameThread()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	check(MeshMaterial);

	ensure(!Material);
	Material = MeshMaterial->GetMaterial_GameThread();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMarchingCubeMesh_LocalVF::Initialize_RenderThread(ERHIFeatureLevel::Type FeatureLevel)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInRenderingThread());

	ensure(IndexBuffer.GetNumIndices() > 0);
	ensure(PositionVertexBuffer.GetNumVertices() > 0);

	VOXEL_INLINE_COUNTER("Index"     , IndexBuffer           .InitResource());
	VOXEL_INLINE_COUNTER("Position"  , PositionVertexBuffer  .InitResource());
	VOXEL_INLINE_COUNTER("StaticMesh", StaticMeshVertexBuffer.InitResource());
	VOXEL_INLINE_COUNTER("Color"     , ColorVertexBuffer     .InitResource());
	
	check(!VertexFactory);
	VertexFactory = MakeShared<FLocalVertexFactory>(FeatureLevel, "FVoxelDefaultMeshRenderData");

	FLocalVertexFactory::FDataType Data;
	PositionVertexBuffer.BindPositionVertexBuffer(VertexFactory.Get(), Data);
	StaticMeshVertexBuffer.BindTangentVertexBuffer(VertexFactory.Get(), Data);
	StaticMeshVertexBuffer.BindPackedTexCoordVertexBuffer(VertexFactory.Get(), Data);
	ColorVertexBuffer.BindColorVertexBuffer(VertexFactory.Get(), Data);

	VertexFactory->SetData(Data);
	VOXEL_INLINE_COUNTER("VertexFactory", VertexFactory->InitResource());

#if RHI_RAYTRACING
	if (IsRayTracingEnabled())
	{
		VOXEL_SCOPE_COUNTER("Raytracing");
		
		check(!RayTracingGeometry);
		RayTracingGeometry = MakeShared<FRayTracingGeometry>();
		
		FRayTracingGeometryInitializer Initializer;
		Initializer.DebugName = "FVoxelDefaultMeshRenderData";
		Initializer.IndexBuffer = IndexBuffer.IndexBufferRHI;
		Initializer.TotalPrimitiveCount = IndexBuffer.GetNumIndices() / 3;
		Initializer.GeometryType = RTGT_Triangles;
		Initializer.bFastBuild = true;
		Initializer.bAllowUpdate = false;

		FRayTracingGeometrySegment Segment;
		Segment.VertexBuffer = PositionVertexBuffer.VertexBufferRHI;
		Segment.NumPrimitives = Initializer.TotalPrimitiveCount;
		Segment.MaxVertices = PositionVertexBuffer.GetNumVertices();
		Initializer.Segments.Add(Segment);

		RayTracingGeometry->SetInitializer(Initializer);
		RayTracingGeometry->InitResource();
	}
#endif
}

void FVoxelMarchingCubeMesh_LocalVF::Destroy_RenderThread()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInRenderingThread());

	IndexBuffer.ReleaseResource();
	PositionVertexBuffer.ReleaseResource();
	StaticMeshVertexBuffer.ReleaseResource();
	ColorVertexBuffer.ReleaseResource();

	check(VertexFactory);
	VertexFactory->ReleaseResource();
	VertexFactory.Reset();
	
#if RHI_RAYTRACING
	if (IsRayTracingEnabled())
	{
		check(RayTracingGeometry);
		RayTracingGeometry->ReleaseResource();
		RayTracingGeometry.Reset();
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelMarchingCubeMesh_LocalVF::Draw_RenderThread(
	const FPrimitiveSceneProxy& Proxy,
	FMeshBatch& MeshBatch) const
{
	MeshBatch.Type = PT_TriangleList;
	MeshBatch.VertexFactory = VertexFactory.Get();
	
	FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
	BatchElement.IndexBuffer = &IndexBuffer;
	BatchElement.FirstIndex = 0;
	BatchElement.NumPrimitives = IndexBuffer.GetNumIndices() / 3;
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = PositionVertexBuffer.GetNumVertices() - 1;
	
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	MeshBatch.VisualizeLODIndex = LOD % GEngine->LODColorationColors.Num();
#endif

	return true;
}

const FRayTracingGeometry* FVoxelMarchingCubeMesh_LocalVF::DrawRaytracing_RenderThread(const FPrimitiveSceneProxy& Proxy, FMeshBatch& MeshBatch) const
{
	MeshBatch.VertexFactory = VertexFactory.Get();
	return RayTracingGeometry.Get();
}