// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelMeshDistanceNodes.h"

void FVoxelMeshDistanceData::Build()
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_LOG_FUNCTION_STATS();

	if (!ensure(Indices.Num() > 0))
	{
		return;
	}
	check(Indices.Num() % 3 == 0);
	const int32 NumTriangles = Indices.Num() / 3;

	Bounds = FBox3f(Vertices);

	Size = FIntVector(16, 16, 16);
	FVoxelUtilities::SetNumFast(Cells, Size.X * Size.Y * Size.Z);

	CellSize = Bounds.GetSize() / FVector3f(Size);
	const float CellRadius = CellSize.GetMax() / 2.f * UE_SQRT_3;

	TVoxelArray<float> Distances;
	FVoxelUtilities::SetNumFast(Distances, NumTriangles);
	
	TVoxelArray<int32> TrianglesToAdd;
	TrianglesToAdd.Reserve(NumTriangles);

	TMap<TPair<int32, int32>, TVoxelArray<int32>> TrianglesMap;
	TrianglesMap.Reserve(Size.X * Size.Y * Size.Z);

	int32 TotalNumTriangles = 0;
	int32 NumTriangleSets = 0;
	for (int32 X = 0; X < Size.X; X++)
	{
		for (int32 Y = 0; Y < Size.Y; Y++)
		{
			for (int32 Z = 0; Z < Size.Z; Z++)
			{
				const FVector3f Center = Bounds.Min + (FVector3f(X, Y, Z) + 0.5f) * CellSize;

				float MinDistance = MAX_flt;
				for (int32 Index = 0; Index < NumTriangles; Index++)
				{
					const FVector3f A = Vertices[Indices[3 * Index + 0]];
					const FVector3f B = Vertices[Indices[3 * Index + 1]];
					const FVector3f C = Vertices[Indices[3 * Index + 2]];

					const float Distance = FMath::Sqrt(Voxel::MeshVoxelizer::PointTriangleDistanceSquared(Center, A, B, C));
					MinDistance = FMath::Min(MinDistance, Distance);
					Distances[Index] = Distance;
				}

				TrianglesToAdd.Reset();
				for (int32 Index = 0; Index < NumTriangles; Index++)
				{
					if (Distances[Index] < MinDistance + CellRadius)
					{
						TrianglesToAdd.Add(Index);
					}
				}
				check(TrianglesToAdd.Num() > 0);

				int32 TrianglesIndex = -1;
				TVoxelArray<int32>& ExistingTriangles = TrianglesMap.FindOrAdd({ TrianglesToAdd.Num(), TrianglesToAdd[0] });
				for (const int32 Index : ExistingTriangles)
				{
					checkVoxelSlow(Triangles[Index] == TrianglesToAdd.Num());
					if (!FVoxelUtilities::MemoryEqual(
						MakeVoxelArrayView(Triangles).Slice(Index + 1, TrianglesToAdd.Num()), 
						TrianglesToAdd))
					{
						continue;
					}

					checkVoxelSlow(TrianglesIndex == -1);
					TrianglesIndex = Index;
				}

				if (TrianglesIndex == -1)
				{
					TrianglesIndex = Triangles.Num();
					ExistingTriangles.Add(TrianglesIndex);

					Triangles.Add(TrianglesToAdd.Num());
					Triangles.Append(TrianglesToAdd);

					NumTriangleSets++;
					TotalNumTriangles += TrianglesToAdd.Num();
				}

				Cells[FVoxelUtilities::Get3DIndex(Size, X, Y, Z)] = TrianglesIndex;

				if (X == 0 && Y == 0 && Z == 0 && false)
				{
					constexpr float Scale = 20.f;

					const auto DrawLine = [&](const FVector3f& Start, const FVector3f& End)
					{
						DrawDebugLine(GWorld, FVector(Start) * Scale, FVector(End) * Scale, FColor::Red, false, 30.f);
					};

					for (const int32 Index : TrianglesToAdd)
					{
						const FVector3f A = Vertices[Indices[3 * Index + 0]];
						const FVector3f B = Vertices[Indices[3 * Index + 1]];
						const FVector3f C = Vertices[Indices[3 * Index + 2]];

						DrawLine(A, B);
						DrawLine(A, C);
						DrawLine(B, C);
					}

					const FVector3f Min = Bounds.Min + FVector3f(X, Y, Z) * CellSize;
					const FVector3f Max = Min + CellSize;

					DrawLine(FVector3f(Min.X, Min.Y, Min.Z), FVector3f(Max.X, Min.Y, Min.Z));
					DrawLine(FVector3f(Min.X, Min.Y, Min.Z), FVector3f(Min.X, Max.Y, Min.Z));
					DrawLine(FVector3f(Min.X, Max.Y, Min.Z), FVector3f(Max.X, Max.Y, Min.Z));
					DrawLine(FVector3f(Max.X, Min.Y, Min.Z), FVector3f(Max.X, Max.Y, Min.Z));

					DrawLine(FVector3f(Min.X, Min.Y, Max.Z), FVector3f(Max.X, Min.Y, Max.Z));
					DrawLine(FVector3f(Min.X, Min.Y, Max.Z), FVector3f(Min.X, Max.Y, Max.Z));
					DrawLine(FVector3f(Min.X, Max.Y, Max.Z), FVector3f(Max.X, Max.Y, Max.Z));
					DrawLine(FVector3f(Max.X, Min.Y, Max.Z), FVector3f(Max.X, Max.Y, Max.Z));

					DrawLine(FVector3f(Min.X, Min.Y, Min.Z), FVector3f(Min.X, Min.Y, Max.Z));
					DrawLine(FVector3f(Max.X, Min.Y, Min.Z), FVector3f(Max.X, Min.Y, Max.Z));
					DrawLine(FVector3f(Min.X, Max.Y, Min.Z), FVector3f(Min.X, Max.Y, Max.Z));
					DrawLine(FVector3f(Max.X, Max.Y, Min.Z), FVector3f(Max.X, Max.Y, Max.Z));

					DrawDebugSphere(GWorld, FVector(Min + Max) / 2.f * Scale, CellRadius * Scale, 10, FColor::Red, false, 30.f);
				}
			}
		}
	}

	LOG_VOXEL(Error, "%d sets; %f triangles/set; %d triangles", NumTriangleSets, TotalNumTriangles / float(NumTriangleSets), Triangles.Num());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_MeshDistance, Distance)
{
	const TValue<FVoxelStaticMeshDistanceData> MeshData = Get(MeshPin, Query);
	const TValue<TBufferView<FVector>> Positions = GetBufferView(PositionPin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, MeshData, Positions)
	{
		const FVoxelMeshDistanceData& Data = *MeshData->Data;

		TVoxelArray<float> Distances = FVoxelFloatBuffer::Allocate(Positions.Num());
		for (int32 PositionIndex = 0; PositionIndex < Positions.Num(); PositionIndex++)
		{
			const FVector3f Position = Positions[PositionIndex];
			FIntVector CellPosition = FVoxelUtilities::FloorToInt((Position - Data.Bounds.Min) / Data.CellSize);
			CellPosition = FVoxelUtilities::Clamp(CellPosition, FIntVector(ForceInit), Data.Size - 1);

			const int32 CellIndex = Data.Cells[FVoxelUtilities::Get3DIndex(Data.Size, CellPosition)];
			const int32 NumTriangles = Data.Triangles[CellIndex];

			float DistanceSquared = MAX_flt;
			int32 BestTriangleIndex = 0;
			for (int32 Index = 0; Index < NumTriangles; Index++)
			{
				const int32 TriangleIndex = Data.Triangles[CellIndex + 1 + Index];
				const FVector3f A = Data.Vertices[Data.Indices[3 * TriangleIndex + 0]];
				const FVector3f B = Data.Vertices[Data.Indices[3 * TriangleIndex + 1]];
				const FVector3f C = Data.Vertices[Data.Indices[3 * TriangleIndex + 2]];

				const float NewDistanceSquared = Voxel::MeshVoxelizer::PointTriangleDistanceSquared(Position, A, B, C);
				if (NewDistanceSquared > DistanceSquared)
				{
					continue;
				}

				DistanceSquared = NewDistanceSquared;
				BestTriangleIndex = TriangleIndex;
			}

			const FVector3f A = Data.Vertices[Data.Indices[3 * BestTriangleIndex + 0]];
			const FVector3f B = Data.Vertices[Data.Indices[3 * BestTriangleIndex + 1]];
			const FVector3f C = Data.Vertices[Data.Indices[3 * BestTriangleIndex + 2]];
			const float Sign = FVector3f::DotProduct(Position - A, FVector3f::CrossProduct(B - A, C - A)) < 0 ? 1.f : -1.f;

			Distances[PositionIndex] = FMath::Sqrt(DistanceSquared) * Sign;
		}
		return FVoxelFloatBuffer::MakeCpu(Distances);
	};
}