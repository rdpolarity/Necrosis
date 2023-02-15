// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelPointCanvasData.h"
#include "Transvoxel.h"

BEGIN_VOXEL_NAMESPACE(PointCanvas)

DEFINE_VOXEL_MEMORY_STAT(STAT_ChunkMemory);

int64 FChunk::GetAllocatedSize() const
{
	int64 Size = sizeof(*this);
	Size += Points.GetAllocatedSize();

	ensure(Indices_Self == Indices_Corners);
	ensure(Indices_Faces == Indices_Corners);
	ensure(Indices_Edges == Indices_Corners);

	if (Indices_Corners)
	{
		Size += sizeof(FIndices);
	}

	return Size;
}

void FChunk::Finalize()
{
	ensure(State == EChunkState::JumpFlooded_Corners);
	ensure(Indices_Corners);

	Indices_Self = Indices_Corners;
	Indices_Faces = Indices_Corners;
	Indices_Edges = Indices_Corners;

	UpdateStats();
}

void FChunk::JumpFloodImpl(EChunkState::Type NewState)
{
	VOXEL_FUNCTION_COUNTER();

	const EChunkState::Type OldState = State;

	ensure(State + 1 == NewState);
	State = NewState;

	// Check if we need to compute anything new
#if 0
	if (OldState != EChunkState::Uninitialized &&
		GetIndices(OldState))
#else
	if (OldState != EChunkState::Uninitialized)
#endif
	{
		bool bHasNewNeighbors = false;
		ForeachNeighbor(NewState, [&](const int32, FChunk&)
		{
			bHasNewNeighbors = true;
		});

		if (!bHasNewNeighbors)
		{
			GetIndices(NewState) = GetIndices(OldState);
			return;
		}
	}

	FAllPoints AllPoints;
	for (int32 ChunkIndex = 0; ChunkIndex < 27; ChunkIndex++)
	{
		const FChunk* Chunk = Chunks[ChunkIndex];
		if (!Chunk)
		{
			continue;
		}

		TVoxelArray<FVector3f>& ChunkPoints = AllPoints.ChunksPoints[ChunkIndex];
		FVoxelUtilities::SetNumFast(ChunkPoints, Chunk->Points.Num());
		for (int32 Index = 0; Index < ChunkPoints.Num(); Index++)
		{
			ChunkPoints[Index] = FVector3f(Chunk->ChunkKey - ChunkKey) * ChunkSize + Chunk->Points[Index].GetPosition();
		}
	}

	const TSharedRef<FIndices> IndicesPtr = MakeShared<FIndices>(NoInit);
	GetIndices(NewState) = IndicesPtr;

	FIndices& Indices = *IndicesPtr;
	Indices.Memzero();

	const auto AddPoints = [&](const int32 ChunkIndex, FChunk& Chunk)
	{
		VOXEL_SCOPE_COUNTER("Add points");

		const FIntVector ChunkOffset = (Chunk.ChunkKey - ChunkKey) * ChunkSize;
		for (int32 PointIndex = 0; PointIndex < Chunk.Points.Num(); PointIndex++)
		{
			const FPoint& Point = Chunk.Points[PointIndex];
			const FIndex NewIndex = FIndex::Make(ChunkIndex, PointIndex);

			{
				FIntVector IndexPosition = ChunkOffset + FIntVector(Point.X, Point.Y, Point.Z);
				IndexPosition = FVoxelUtilities::Clamp(IndexPosition, 0, ChunkSize - 1);

				FIndex& OldIndex = Indices[FVoxelUtilities::Get3DIndex<int32>(ChunkSize, IndexPosition)];

				if (!OldIndex.bIsValid ||
					FVector3f::DistSquared(AllPoints[NewIndex], FVector3f(IndexPosition)) <
					FVector3f::DistSquared(AllPoints[OldIndex], FVector3f(IndexPosition)))
				{
					OldIndex = NewIndex;
				}
			}

			{
				FIntVector IndexPosition = ChunkOffset + FIntVector(Point.X, Point.Y, Point.Z);
				IndexPosition[Point.Direction]++;
				IndexPosition = FVoxelUtilities::Clamp(IndexPosition, 0, ChunkSize - 1);

				FIndex& OldIndex = Indices[FVoxelUtilities::Get3DIndex<int32>(ChunkSize, IndexPosition)];

				if (!OldIndex.bIsValid ||
					FVector3f::DistSquared(AllPoints[NewIndex], FVector3f(IndexPosition)) <
					FVector3f::DistSquared(AllPoints[OldIndex], FVector3f(IndexPosition)))
				{
					OldIndex = NewIndex;
				}
			}
		}
	};

	for (EChunkState::Type StateIt = EChunkState::JumpFlooded_Self; StateIt <= State; StateIt = EChunkState::Type(StateIt + 1))
	{
		ForeachNeighbor(StateIt, [&](const int32 ChunkIndex, FChunk& Chunk)
		{
			AddPoints(ChunkIndex, Chunk);
		});
	}

	checkStatic(ChunkSizeLog2 == 3);

	TVoxelStaticArray<FIndex, ChunkCount> TempIndices{ NoInit };
	JumpFloodPass<4>(AllPoints, Indices, TempIndices);
	JumpFloodPass<2>(AllPoints, TempIndices, Indices);
	JumpFloodPass<1>(AllPoints, Indices, TempIndices);
	FVoxelUtilities::Memcpy(Indices, TempIndices);
}

void FChunk::InvalidateImpl(const EChunkState::Type NewState)
{
	checkVoxelSlow(NewState < State);

	if (NewState == EChunkState::Uninitialized)
	{
		checkVoxelSlow(EChunkState::JumpFlooded_Self <= State);

		ForeachNeighbor_Faces([&](const int32, FChunk& Chunk)
		{
			Chunk.Invalidate(EChunkState::JumpFlooded_Self);
		});

		if (EChunkState::JumpFlooded_Faces <= State)
		{
			ForeachNeighbor_Edges([&](const int32, FChunk& Chunk)
			{
				Chunk.Invalidate(EChunkState::JumpFlooded_Faces);
			});
		}

		if (EChunkState::JumpFlooded_Edges <= State)
		{
			ForeachNeighbor_Corners([&](const int32, FChunk& Chunk)
			{
				Chunk.Invalidate(EChunkState::JumpFlooded_Edges);
			});
		}
	}
	else if (NewState == EChunkState::JumpFlooded_Self)
	{
		checkVoxelSlow(EChunkState::JumpFlooded_Faces <= State);

		ForeachNeighbor_Edges([&](const int32, FChunk& Chunk)
		{
			Chunk.Invalidate(EChunkState::JumpFlooded_Faces);
		});

		if (EChunkState::JumpFlooded_Edges <= State)
		{
			ForeachNeighbor_Corners([&](const int32, FChunk& Chunk)
			{
				Chunk.Invalidate(EChunkState::JumpFlooded_Edges);
			});
		}
	}
	else if (NewState == EChunkState::JumpFlooded_Faces)
	{
		checkVoxelSlow(EChunkState::JumpFlooded_Edges <= State);

		ForeachNeighbor_Corners([&](const int32, FChunk& Chunk)
		{
			Chunk.Invalidate(EChunkState::JumpFlooded_Edges);
		});
	}
	else
	{
		checkVoxelSlow(NewState == EChunkState::JumpFlooded_Edges);
		checkVoxelSlow(State == EChunkState::JumpFlooded_Corners);
		// No neighbors to recompute
	}

	State = NewState;
}

template<int32 Step>
void FChunk::JumpFloodPass(
	const FAllPoints& AllPoints,
	const TVoxelStaticArray<FIndex, ChunkCount>& InIndices,
	TVoxelStaticArray<FIndex, ChunkCount>& OutIndices)
{
	VOXEL_FUNCTION_COUNTER();
	
	for (int32 Z = 0; Z < ChunkSize; Z++)
	{
		for (int32 Y = 0; Y < ChunkSize; Y++)
		{
			int32 Index = FVoxelUtilities::Get3DIndex<int32>(ChunkSize, FIntVector(0, Y, Z));
			for (int32 X = 0; X < ChunkSize; X++)
			{
				FIndex BestPointIndex;
				{
					const FVector3f Position(X, Y, Z);
					float BestDistance = MAX_flt;

#define CheckNeighbor_DX(DX, DY, DZ) \
					{ \
						const int32 NeighborIndex = Index + DX * Step + DY * Step * ChunkSize + DZ * Step * ChunkSize * ChunkSize; \
						checkVoxelSlow(NeighborIndex == FVoxelUtilities::Get3DIndex(ChunkSize, FIntVector(X, Y, Z) + FIntVector(DX * Step, DY * Step, DZ * Step))); \
						const FIndex NeighborPointIndex = InIndices[NeighborIndex]; \
						if (NeighborPointIndex.bIsValid) \
						{ \
							const FVector3f NeighborPosition = AllPoints[NeighborPointIndex]; \
							const float Distance = FVector3f::DistSquared(NeighborPosition, Position); \
							if (Distance < BestDistance) \
							{ \
								BestDistance = Distance; \
								BestPointIndex = NeighborPointIndex; \
							} \
						} \
					}

#define CheckNeighbor_DY(DY, DZ) \
					if (X - Step >= 0) { CheckNeighbor_DX(-1, DY, DZ); } \
					CheckNeighbor_DX(+0, DY, DZ); \
					if (X + Step < ChunkSize) { CheckNeighbor_DX(+1, DY, DZ); }

#define CheckNeighbor_DZ(DZ) \
					if (Y - Step >= 0) { CheckNeighbor_DY(-1, DZ); } \
					CheckNeighbor_DY(+0, DZ); \
					if (Y + Step < ChunkSize) { CheckNeighbor_DY(+1, DZ); }

#define CheckNeighbor() \
					if (Z - Step >= 0) { CheckNeighbor_DZ(-1); } \
					CheckNeighbor_DZ(+0); \
					if (Z + Step < ChunkSize) { CheckNeighbor_DZ(+1); }

					CheckNeighbor();

#undef CheckNeighbor
#undef CheckNeighbor_DX
#undef CheckNeighbor_DY
#undef CheckNeighbor_DZ
				}
				
				OutIndices[Index] = BestPointIndex;
				Index++;
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FLODData::Serialize(FArchive& Ar)
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelScopeLock_Write Lock(CriticalSection);

#if 0
	if (Ar.IsSaving())
	{
		Ar << DataType;

		TArray<FIntVector> Keys;
		Chunks.GenerateKeyArray(Keys);

		FVoxelUtilities::ForceBulkSerializeArray(Ar, Keys);

		for (const auto& It : Chunks)
		{
			Ar.Serialize(It.Value->Data, GetChunkNumBytes(DataType));
		}
	}
	else
	{
		check(Ar.IsLoading());

		Ar << DataType;

		TArray<FIntVector> Keys;
		FVoxelUtilities::ForceBulkSerializeArray(Ar, Keys);

		Chunks.Reset();
		Chunks.Reserve(Keys.Num());

		for (const FIntVector& Key : Keys)
		{
			const TSharedRef<FChunk> Chunk = MakeShared<FChunk>(DataType);
			Ar.Serialize(Chunk->Data, GetChunkNumBytes(DataType));
			Chunks.Add(Key, Chunk);
		}
	}
#endif
}

FVoxelIntBox FLODData::GetBounds() const
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelScopeLock_Read Lock(CriticalSection);

	FVoxelOptionalIntBox Bounds;
	for (const auto& It : Chunks)
	{
		Bounds += FVoxelIntBox(It.Key).Scale(ChunkSize);
	}

	return Bounds.IsValid() ? Bounds.GetBox() : FVoxelIntBox();
}

TVoxelArray<FVector4f> FLODData::GetAllPoints() const
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelArray<FVector4f> Points;

	FVoxelScopeLock_Read Lock(CriticalSection);

	int32 NumPoints = 0;
	for (const auto& It : Chunks)
	{
		NumPoints += It.Value->Points.Num();
	}

	Points.Reserve(NumPoints);

	for (const auto& It : Chunks)
	{
		for (const FPoint& Point : It.Value->Points)
		{
			Points.Add(FVector4f(FVector3f(It.Key * ChunkSize) + Point.GetPosition(), Point.Direction));
		}
	}

	return Points;
}

TVoxelArray<FVoxelIntBox> FLODData::GetChunkBounds() const
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelScopeLock_Read Lock(CriticalSection);

	TVoxelArray<FVoxelIntBox> Bounds;
	Bounds.Reserve(Chunks.Num());
	for (const auto& It : Chunks)
	{
		Bounds.Add(FVoxelIntBox(It.Key).Scale(ChunkSize));
	}
	return Bounds;
}

void FLODData::AddPoints(const TVoxelArray<FVector3f>& Vertices)
{
	VOXEL_FUNCTION_COUNTER();
	ensure(Vertices.Num() % 3 == 0);
	checkVoxelSlow(CriticalSection.IsLocked_Write_Debug());

	LastVertices = Vertices;

	FChunk* Chunk = nullptr;

	const int32 NumTriangles = Vertices.Num() / 3;
	for (int32 TriangleIndex = 0; TriangleIndex < NumTriangles; TriangleIndex++)
	{
		const FVector3f VertexA = Vertices[3 * TriangleIndex + 0];
		const FVector3f VertexB = Vertices[3 * TriangleIndex + 1];
		const FVector3f VertexC = Vertices[3 * TriangleIndex + 2];

		const FVector3f Normal = FVoxelUtilities::GetTriangleNormal(VertexA, VertexB, VertexC);

		const FVector3f BoundsMin = FVoxelUtilities::ComponentMin3(VertexA, VertexB, VertexC);
		const FVector3f BoundsMax = FVoxelUtilities::ComponentMax3(VertexA, VertexB, VertexC);

		const FIntVector Start = FVoxelUtilities::FloorToInt(BoundsMin);
		const FIntVector End = FVoxelUtilities::CeilToInt(BoundsMax);

		if (FVoxelIntBox(Start, End).Count_LargeBox() > 1024 * 1024)
		{
			ensure(false);
			continue;
		}

		for (int32 Direction = 0; Direction < 3; Direction++)
		{
			const int32 DirectionI = (Direction + 2) % 3;
			const int32 DirectionJ = (Direction + 1) % 3;
			const int32 DirectionK = (Direction + 0) % 3;

			const int32 StartI = Start[DirectionI];
			const int32 StartJ = Start[DirectionJ];

			const int32 EndI = End[DirectionI];
			const int32 EndJ = End[DirectionJ];

			const float MinK = BoundsMin[DirectionK];

			FVector3f RayOrigin;
			RayOrigin[DirectionK] = MinK;

			FVector3f RayDirection = FVector3f(ForceInit);
			RayDirection[DirectionK] = 1;

			for (int32 I = StartI; I < EndI; I++)
			{
				RayOrigin[DirectionI] = I;

				for (int32 J = StartJ; J < EndJ; J++)
				{
					RayOrigin[DirectionJ] = J;

					float Time;
					if (!FVoxelUtilities::RayTriangleIntersection(
						RayOrigin,
						RayDirection,
						VertexA,
						VertexB,
						VertexC,
						false,
						Time))
					{
						continue;
					}

					const float ExactK = MinK + Time;
					const int32 K = FMath::FloorToInt(ExactK);
					const float Alpha = ExactK - K;

					FIntVector Position;
					Position[DirectionI] = I;
					Position[DirectionJ] = J;
					Position[DirectionK] = K;

					const FIntVector ChunkKey = FVoxelUtilities::DivideFloor_FastLog2(Position, ChunkSizeLog2);

					if (!Chunk || Chunk->ChunkKey != ChunkKey)
					{
						Chunk = &FindOrAddChunk(ChunkKey);
						Chunk->Invalidate(EChunkState::Uninitialized);
					}

					const FIntVector LocalPosition = Position - ChunkKey * ChunkSize;

					Chunk->Points.Add(FPoint::Make(
						LocalPosition,
						DirectionK,
						Alpha,
						Normal));
				}
			}
		}
	}

	JumpFloodChunks();

	RenderCounter.Increment();
}

void FLODData::JumpFloodChunks()
{
	VOXEL_FUNCTION_COUNTER();
	checkVoxelSlow(CriticalSection.IsLocked_Write_Debug());

	{
		VOXEL_SCOPE_COUNTER("Cleanup");

		// Remove chunks with no points at all (including in their neighbors)
		for (auto It = Chunks.CreateIterator(); It; ++It)
		{
			const bool bHasPoints = INLINE_LAMBDA
			{
				for (const FChunk* Chunk : It.Value()->Chunks)
				{
					if (Chunk &&
						Chunk->Points.Num() > 0)
					{
						return true;
					}
				}
				return false;
			};

			if (!bHasPoints)
			{
				It.RemoveCurrent();
				RecomputeNeighbors(It.Key());
			}
		}

		// Add a 1 chunk wide border
		for (auto& It : MakeCopy(Chunks))
		{
			if (It.Value->Points.Num() == 0)
			{
				continue;
			}

			for (int32 X = -1; X <= 1; X++)
			{
				for (int32 Y = -1; Y <= 1; Y++)
				{
					for (int32 Z = -1; Z <= 1; Z++)
					{
						FindOrAddChunk(It.Key + FIntVector(X, Y, Z));
					}
				}
			}
		}
	}

	for (EChunkState::Type State = EChunkState::JumpFlooded_Self; State <= EChunkState::JumpFlooded_Corners; State = EChunkState::Type(State + 1))
	{
		VOXEL_SCOPE_COUNTER("JumpFlood");

		for (const auto& It : Chunks)
		{
			It.Value->JumpFlood(State);
		}
	}
}

void FLODData::RecomputeNeighbors(const FIntVector& ChunkKey)
{
	VOXEL_FUNCTION_COUNTER();
	checkVoxelSlow(CriticalSection.IsLocked_Write_Debug());

	const auto Recompute = [&](const FIntVector& ChunkKeyToRecompute)
	{
		FChunk* Chunk = FindChunk(ChunkKeyToRecompute);
		if (!Chunk)
		{
			return;
		}

		for (int32 X = -1; X <= 1; X++)
		{
			for (int32 Y = -1; Y <= 1; Y++)
			{
				for (int32 Z = -1; Z <= 1; Z++)
				{
					Chunk->Chunks[FVoxelUtilities::Get3DIndex<int32>(3, X, Y, Z, -1)] = FindChunk(Chunk->ChunkKey + FIntVector(X, Y, Z));
				}
			}
		}
	};

	for (int32 X = -1; X <= 1; X++)
	{
		for (int32 Y = -1; Y <= 1; Y++)
		{
			for (int32 Z = -1; Z <= 1; Z++)
			{
				Recompute(ChunkKey + FIntVector(X, Y, Z));
			}
		}
	}
}

void FLODData::EditPoints(const TFunctionRef<bool(FVoxelIntBox)> ShouldVisitChunk, const TFunctionRef<void(TVoxelArrayView<FVector3f>)> EditPoints)
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelScopeLock_Write Lock(CriticalSection);
	
	TVoxelArray<FVector3f> Vertices;
	TVoxelArray<FChunk*> ChunksToInvalidate;

	for (const auto& It : Chunks)
	{
		if (It.Value->Points.Num() == 0 ||
			!ShouldVisitChunk(FVoxelIntBox(It.Key).Scale(ChunkSize)))
		{
			continue;
		}

		ChunksToInvalidate.Add(It.Value.Get());
		GenerateTriangles(It.Value->ChunkKey * ChunkSize, Vertices);
	}

	for (FChunk* Chunk : ChunksToInvalidate)
	{
		Chunk->Points.Reset();
		Chunk->Invalidate(EChunkState::Uninitialized);
	}

	EditPoints(Vertices);

	AddPoints(Vertices);
}

void FLODData::GenerateTriangles(const FIntVector& Start, TVoxelArray<FVector3f>& OutVertices) const
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelStaticArray<float, FMath::Cube(ChunkSize + 1)> Densities{ NoInit };
	{
		VOXEL_SCOPE_COUNTER("Compute densities");
		
		const FChunk* Chunk = nullptr;
		FIntVector ChunkKey = FIntVector(MAX_int32);

		for (int32 Z = 0; Z < ChunkSize + 1; Z++)
		{
			for (int32 Y = 0; Y < ChunkSize + 1; Y++)
			{
				for (int32 X = 0; X < ChunkSize + 1; X++)
				{
					float Distance = 1;
					ensureVoxelSlow(FUtilities::FindClosestPosition(
						Distance,
						*this,
						FVector3f(Start.X + X, Start.Y + Y, Start.Z + Z),
						Chunk,
						ChunkKey));

					Densities[FVoxelUtilities::Get3DIndex<int32>(ChunkSize + 1, X, Y, Z)] = Distance;
				}
			}
		}
	}

	for (int32 Z = 0; Z < ChunkSize; Z++)
	{
		for (int32 Y = 0; Y < ChunkSize; Y++)
		{
			for (int32 X = 0; X < ChunkSize; X++)
			{
				const uint32 CaseCode =
					((Densities[FVoxelUtilities::Get3DIndex<int32>(ChunkSize + 1, X + 0, Y + 0, Z + 0)] > 0) << 0) |
					((Densities[FVoxelUtilities::Get3DIndex<int32>(ChunkSize + 1, X + 1, Y + 0, Z + 0)] > 0) << 1) |
					((Densities[FVoxelUtilities::Get3DIndex<int32>(ChunkSize + 1, X + 0, Y + 1, Z + 0)] > 0) << 2) |
					((Densities[FVoxelUtilities::Get3DIndex<int32>(ChunkSize + 1, X + 1, Y + 1, Z + 0)] > 0) << 3) |
					((Densities[FVoxelUtilities::Get3DIndex<int32>(ChunkSize + 1, X + 0, Y + 0, Z + 1)] > 0) << 4) |
					((Densities[FVoxelUtilities::Get3DIndex<int32>(ChunkSize + 1, X + 1, Y + 0, Z + 1)] > 0) << 5) |
					((Densities[FVoxelUtilities::Get3DIndex<int32>(ChunkSize + 1, X + 0, Y + 1, Z + 1)] > 0) << 6) |
					((Densities[FVoxelUtilities::Get3DIndex<int32>(ChunkSize + 1, X + 1, Y + 1, Z + 1)] > 0) << 7);

				if (CaseCode == 0 ||
					CaseCode == 255)
				{
					continue;
				}

				checkVoxelSlow(0 <= CaseCode && CaseCode < 256);
				const uint8 CellClass = Transvoxel::RegularCellClass[CaseCode];
				const uint16* RESTRICT VertexData = Transvoxel::RegularVertexData[CaseCode];

				checkVoxelSlow(0 <= CellClass && CellClass < 16);
				const Transvoxel::FRegularCellData& CellData = Transvoxel::RegularCellData[CellClass];

				TVoxelArray<FVector3f, TFixedAllocator<16>> CellVertices;
				for (int32 I = 0; I < CellData.GetVertexCount(); I++)
				{
					const uint16 EdgeCode = VertexData[I];

					const uint8 LocalIndexA = (EdgeCode >> 4) & 0x0F;
					const uint8 LocalIndexB = EdgeCode & 0x0F;

					checkVoxelSlow(0 <= LocalIndexA && LocalIndexA < 8);
					checkVoxelSlow(0 <= LocalIndexB && LocalIndexB < 8);

					const int32 IndexA = FVoxelUtilities::Get3DIndex<int32>(ChunkSize + 1, X + bool(LocalIndexA & 0x1), Y + bool(LocalIndexA & 0x2), Z + bool(LocalIndexA & 0x4));
					const int32 IndexB = FVoxelUtilities::Get3DIndex<int32>(ChunkSize + 1, X + bool(LocalIndexB & 0x1), Y + bool(LocalIndexB & 0x2), Z + bool(LocalIndexB & 0x4));

					const float ValueAtA = Densities[IndexA];
					const float ValueAtB = Densities[IndexB];

					ensureVoxelSlow((ValueAtA > 0) != (ValueAtB > 0));

					const uint8 EdgeIndex = ((EdgeCode >> 8) & 0x0F);
					checkVoxelSlow(0 <= EdgeIndex && EdgeIndex < 3);

					const FIntVector PositionA(X + bool(LocalIndexA & 0x1), Y + bool(LocalIndexA & 0x2), Z + bool(LocalIndexA & 0x4));

					FVector3f Position = FVector3f(PositionA);
					Position[EdgeIndex] += ValueAtA / (ValueAtA - ValueAtB);
					CellVertices.Add(FVector3f(Start) + Position);
				}

				for (int32 Index = 0; Index < CellData.GetTriangleCount(); Index++)
				{
					FVector3f VertexA = CellVertices[CellData.VertexIndex[3 * Index + 0]];
					FVector3f VertexB = CellVertices[CellData.VertexIndex[3 * Index + 1]];
					FVector3f VertexC = CellVertices[CellData.VertexIndex[3 * Index + 2]];

					const FVector3f Centroid = (VertexA + VertexB + VertexC) / 3.f;

					// Scale triangle up a bit to avoid precision issue when tracing rays
					VertexA = Centroid + 1.01f * (VertexA - Centroid);
					VertexB = Centroid + 1.01f * (VertexB - Centroid);
					VertexC = Centroid + 1.01f * (VertexC - Centroid);

					OutVertices.Add(VertexA);
					OutVertices.Add(VertexB);
					OutVertices.Add(VertexC);
				}
			}
		}
	}
}

END_VOXEL_NAMESPACE(PointCanvas)

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelPointCanvasData::Serialize(FArchive& Ar)
{
	VOXEL_FUNCTION_COUNTER();

	using FVersion = DECLARE_VOXEL_VERSION
	(
		FirstVersion
	);

	int32 Version = FVersion::LatestVersion;
	Ar << Version;
	check(Version == FVersion::FirstVersion);
}