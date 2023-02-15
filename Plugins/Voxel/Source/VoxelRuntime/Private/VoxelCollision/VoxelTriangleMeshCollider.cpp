// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelCollision/VoxelTriangleMeshCollider.h"

int64 FVoxelTriangleMeshCollider::GetAllocatedSize() const
{
	int64 Result = 0;

	Result += TriangleMeshes.GetAllocatedSize();
	
	for (const TSharedPtr<Chaos::FTriangleMeshImplicitObject>& TriangleMesh : TriangleMeshes)
	{
		// TODO Record memory usage
	}
	
	return Result;
}

void FVoxelTriangleMeshCollider::AddToBodySetup(UBodySetup& BodySetup) const
{
	VOXEL_FUNCTION_COUNTER();

	for (const TSharedPtr<Chaos::FTriangleMeshImplicitObject>& TriangleMesh : TriangleMeshes)
	{
		// Copied from UBodySetup::FinishCreatingPhysicsMeshes_Chaos
#if TRACK_CHAOS_GEOMETRY
		TriangleMesh->Track(Chaos::MakeSerializable(TriangleMesh), "Voxel Mesh");
#endif
		// Force trimesh collisions off
		TriangleMesh->SetDoCollide(false);

		BodySetup.ChaosTriMeshes.Add(TriangleMesh);
	}
}

TSharedPtr<IVoxelColliderRenderData> FVoxelTriangleMeshCollider::GetRenderData() const
{
	if (!ensure(TriangleMeshes.Num() > 0) ||
		!ensure(TriangleMeshes[0]->Elements().GetNumTriangles() > 0))
	{
		return nullptr;
	}

	return MakeShared<FVoxelTriangleMeshCollider_RenderData>(*this);
}

TArray<TWeakObjectPtr<UPhysicalMaterial>> FVoxelTriangleMeshCollider::GetPhysicalMaterials() const
{
	return PhysicalMaterials;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelTriangleMeshCollider_RenderData::FVoxelTriangleMeshCollider_RenderData(const FVoxelTriangleMeshCollider& Collider)
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelArray<uint32> Indices;
	TVoxelArray<FVector3f> Vertices;
	for (const TSharedPtr<Chaos::FTriangleMeshImplicitObject>& TriangleMesh : Collider.TriangleMeshes)
	{
		const auto Process = [&](auto Elements)
		{
			for (const auto& Element : Elements)
			{
				Indices.Add(Vertices.Num());
				Vertices.Add(TriangleMesh->Particles().X(Element[0]));
				
				Indices.Add(Vertices.Num());
				Vertices.Add(TriangleMesh->Particles().X(Element[1]));

				Indices.Add(Vertices.Num());
				Vertices.Add(TriangleMesh->Particles().X(Element[2]));
			}
		};

		if (TriangleMesh->Elements().RequiresLargeIndices())
		{
			Process(TriangleMesh->Elements().GetLargeIndexBuffer());
		}
		else
		{
			Process(TriangleMesh->Elements().GetSmallIndexBuffer());
		}
	}

	IndexBuffer.SetIndices(Indices, EIndexBufferStride::Force32Bit);
	PositionVertexBuffer.Init(Vertices.Num(), false);
	StaticMeshVertexBuffer.Init(Vertices.Num(), 1, false);
	ColorVertexBuffer.Init(Vertices.Num(), false);

	for (int32 Index = 0; Index < Vertices.Num(); Index++)
	{
		PositionVertexBuffer.VertexPosition(Index) = Vertices[Index];
		StaticMeshVertexBuffer.SetVertexUV(Index, 0, FVector2f::ZeroVector);
		ColorVertexBuffer.VertexColor(Index) = FColor::Black;
	}

	for (int32 Index = 0; Index < Vertices.Num(); Index += 3)
	{
		const FVector3f Normal = FVoxelUtilities::GetTriangleNormal(
			Vertices[Index + 0],
			Vertices[Index + 1],
			Vertices[Index + 2]);

		const FVector3f Tangent = FVector3f::ForwardVector;
		const FVector3f Bitangent = FVector3f::CrossProduct(Normal, Tangent);

		StaticMeshVertexBuffer.SetVertexTangents(Index + 0, Tangent, Bitangent, Normal);
		StaticMeshVertexBuffer.SetVertexTangents(Index + 1, Tangent, Bitangent, Normal);
		StaticMeshVertexBuffer.SetVertexTangents(Index + 2, Tangent, Bitangent, Normal);
	}

	IndexBuffer.InitResource();
	PositionVertexBuffer.InitResource();
	StaticMeshVertexBuffer.InitResource();
	ColorVertexBuffer.InitResource();

	VertexFactory = MakeUnique<FLocalVertexFactory>(GMaxRHIFeatureLevel, "FVoxelTriangleMeshCollider_Chaos_RenderData");

	FLocalVertexFactory::FDataType Data;
	PositionVertexBuffer.BindPositionVertexBuffer(VertexFactory.Get(), Data);
	StaticMeshVertexBuffer.BindTangentVertexBuffer(VertexFactory.Get(), Data);
	StaticMeshVertexBuffer.BindPackedTexCoordVertexBuffer(VertexFactory.Get(), Data);
	ColorVertexBuffer.BindColorVertexBuffer(VertexFactory.Get(), Data);

	VertexFactory->SetData(Data);
	VertexFactory->InitResource();
}

FVoxelTriangleMeshCollider_RenderData::~FVoxelTriangleMeshCollider_RenderData()
{
	IndexBuffer.ReleaseResource();
	PositionVertexBuffer.ReleaseResource();
	StaticMeshVertexBuffer.ReleaseResource();
	ColorVertexBuffer.ReleaseResource();
	VertexFactory->ReleaseResource();
}

void FVoxelTriangleMeshCollider_RenderData::Draw_RenderThread(const FPrimitiveSceneProxy& Proxy, FMeshBatch& MeshBatch)
{
	MeshBatch.Type = PT_TriangleList;
	MeshBatch.VertexFactory = VertexFactory.Get();

	FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
	BatchElement.IndexBuffer = &IndexBuffer;
	BatchElement.FirstIndex = 0;
	BatchElement.NumPrimitives = IndexBuffer.GetNumIndices() / 3;
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = PositionVertexBuffer.GetNumVertices() - 1;
}