// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelCollision/VoxelCollisionCooker.h"
#include "VoxelCollision/VoxelTriangleMeshCollider.h"
#include "Chaos/CollisionConvexMesh.h"

TSharedPtr<FVoxelTriangleMeshCollider> FVoxelCollisionCooker::CookTriangleMesh(
	const TConstVoxelArrayView<int32> Indices,
	const TConstVoxelArrayView<FVector3f> Vertices,
	const TConstVoxelArrayView<uint16> FaceMaterials)
{
	VOXEL_FUNCTION_COUNTER();

	if (Indices.Num() == 0 ||
		!ensure(Indices.Num() % 3 == 0))
	{
		return nullptr;
	}

	Chaos::TParticles<Chaos::FRealSingle, 3> Particles;
	Particles.AddParticles(Vertices.Num());
	for (int32 Index = 0; Index < Vertices.Num(); Index++)
	{
		Particles.X(Index) = Vertices[Index];
	}

	const auto Process = [&](auto& Triangles) -> TSharedPtr<Chaos::FTriangleMeshImplicitObject>
	{
		const int32 NumTriangles = Indices.Num() / 3;
		Triangles.Reserve(NumTriangles);

		for (int32 Index = 0; Index < NumTriangles; Index++)
		{
			const Chaos::TVector<int32, 3> Triangle{
					Indices[3 * Index + 2],
					Indices[3 * Index + 1],
					Indices[3 * Index + 0]
			};

			if (!Chaos::FConvexBuilder::IsValidTriangle(
				Particles.X(Triangle.X),
				Particles.X(Triangle.Y),
				Particles.X(Triangle.Z)))
			{
				continue;
			}

			Triangles.Add(Triangle);
		}

		if (Triangles.Num() == 0)
		{
			return nullptr;
		}

		VOXEL_SCOPE_COUNTER("Cook");
		return ::MakeShared<Chaos::FTriangleMeshImplicitObject>(MoveTemp(Particles), ::MoveTemp(Triangles), TArray<uint16>(FaceMaterials));
	};

	const TSharedRef<FVoxelTriangleMeshCollider> Collider = MakeShared<FVoxelTriangleMeshCollider>();

	TSharedPtr<Chaos::FTriangleMeshImplicitObject> TriangleMesh;
	if (Vertices.Num() < MAX_uint16)
	{
		TVoxelArray<Chaos::TVector<uint16, 3>> TrianglesSmallIdx;
		TriangleMesh = Process(TrianglesSmallIdx);
	}
	else
	{
		TVoxelArray<Chaos::TVector<int32, 3>> TrianglesLargeIdx;
		TriangleMesh = Process(TrianglesLargeIdx);
	}

	if (!TriangleMesh)
	{
		return nullptr;
	}

	Collider->TriangleMeshes.Add(TriangleMesh);

	{
		VOXEL_SCOPE_COUNTER("Compute bounds");
		Collider->Bounds = FBox(FBox3f(Vertices.GetData(), Vertices.Num()));
	}

	return Collider;
}