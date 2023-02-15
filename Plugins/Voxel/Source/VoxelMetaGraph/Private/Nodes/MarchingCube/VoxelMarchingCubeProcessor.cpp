// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/MarchingCube/VoxelMarchingCubeProcessor.h"
#include "Transvoxel.h"

void FVoxelMarchingCubeProcessor::MainPass(
	const TConstVoxelArrayView<float> Densities,
	TVoxelArray<FVoxelInt4>& OutCells,
	TVoxelArray<int32>& OutIndices,
	TVoxelArray<float>& OutVerticesX,
	TVoxelArray<float>& OutVerticesY,
	TVoxelArray<float>& OutVerticesZ) const
{
	VOXEL_FUNCTION_COUNTER();

	const int32 EstimatedNumCells = 4 * ChunkSize * ChunkSize;

	OutCells.Reserve(EstimatedNumCells);
	OutIndices.Reserve(12 * EstimatedNumCells);
	OutVerticesX.Reserve(4 * EstimatedNumCells);
	OutVerticesY.Reserve(4 * EstimatedNumCells);
	OutVerticesZ.Reserve(4 * EstimatedNumCells);

	// Cache to get index of already created vertices
	TVoxelArray<int32> CurrentCache;
	TVoxelArray<int32> OldCache;

	FVoxelUtilities::SetNumFast(CurrentCache, ChunkSize * ChunkSize * EdgeIndexCount);
	FVoxelUtilities::SetNumFast(OldCache, ChunkSize * ChunkSize * EdgeIndexCount);

	uint32 VoxelIndex = 0;
	for (int32 LZ = 0; LZ < ChunkSize; LZ++)
	{
		ON_SCOPE_EXIT
		{
			VoxelIndex += (DataSize - ChunkSize) * DataSize; // End edge voxel

			Swap(CurrentCache, OldCache);
		};
		
		for (int32 LY = 0; LY < ChunkSize; LY++)
		{
			ON_SCOPE_EXIT
			{
				VoxelIndex += DataSize - ChunkSize; // End edge voxel
			};
			
			for (int32 LX = 0; LX < ChunkSize; LX++)
			{
				ON_SCOPE_EXIT
				{
					VoxelIndex++;
				};

				{
					// Most voxels are going to end up empty
					// We heavily optimize that hot path by just checking if they all have the same sign
					
					bool bValue = (Densities[VoxelIndex + 0 + 0 * DataSize + 0 * DataSize * DataSize] > 0);
					if (bValue != (Densities[VoxelIndex + 1 + 0 * DataSize + 0 * DataSize * DataSize] > 0)) goto Full;
					if (bValue != (Densities[VoxelIndex + 0 + 1 * DataSize + 0 * DataSize * DataSize] > 0)) goto Full;
					if (bValue != (Densities[VoxelIndex + 1 + 1 * DataSize + 0 * DataSize * DataSize] > 0)) goto Full;
					if (bValue != (Densities[VoxelIndex + 0 + 0 * DataSize + 1 * DataSize * DataSize] > 0)) goto Full;
					if (bValue != (Densities[VoxelIndex + 1 + 0 * DataSize + 1 * DataSize * DataSize] > 0)) goto Full;
					if (bValue != (Densities[VoxelIndex + 0 + 1 * DataSize + 1 * DataSize * DataSize] > 0)) goto Full;
					if (bValue != (Densities[VoxelIndex + 1 + 1 * DataSize + 1 * DataSize * DataSize] > 0)) goto Full;
					continue;
				}

Full:
				
				const uint32 CaseCode =
					((Densities[VoxelIndex + 0 + 0 * DataSize + 0 * DataSize * DataSize] > 0) << 0) |
					((Densities[VoxelIndex + 1 + 0 * DataSize + 0 * DataSize * DataSize] > 0) << 1) |
					((Densities[VoxelIndex + 0 + 1 * DataSize + 0 * DataSize * DataSize] > 0) << 2) |
					((Densities[VoxelIndex + 1 + 1 * DataSize + 0 * DataSize * DataSize] > 0) << 3) |
					((Densities[VoxelIndex + 0 + 0 * DataSize + 1 * DataSize * DataSize] > 0) << 4) |
					((Densities[VoxelIndex + 1 + 0 * DataSize + 1 * DataSize * DataSize] > 0) << 5) |
					((Densities[VoxelIndex + 0 + 1 * DataSize + 1 * DataSize * DataSize] > 0) << 6) |
					((Densities[VoxelIndex + 1 + 1 * DataSize + 1 * DataSize * DataSize] > 0) << 7);

				checkVoxelSlow(CaseCode != 0 && CaseCode != 255);

				const uint8 ValidityMask = (LX != 0) + 2 * (LY != 0) + 4 * (LZ != 0);

				checkVoxelSlow(0 <= CaseCode && CaseCode < 256);
				const uint8 CellClass = Transvoxel::RegularCellClass[CaseCode];
				const uint16* RESTRICT VertexData = Transvoxel::RegularVertexData[CaseCode];

				checkVoxelSlow(0 <= CellClass && CellClass < 16);
				const Transvoxel::FRegularCellData& CellData = Transvoxel::RegularCellData[CellClass];

				// Indices of the vertices used in this cube
				TVoxelStaticArray<int32, 16> VertexIndices{ NoInit };
				for (int32 I = 0; I < CellData.GetVertexCount(); I++)
				{
					int32 VertexIndex = -2;
					const uint16 EdgeCode = VertexData[I];

					// A: low point / B: high point
					const uint8 LocalIndexA = (EdgeCode >> 4) & 0x0F;
					const uint8 LocalIndexB = EdgeCode & 0x0F;

					checkVoxelSlow(0 <= LocalIndexA && LocalIndexA < 8);
					checkVoxelSlow(0 <= LocalIndexB && LocalIndexB < 8);

					const uint32 IndexA = VoxelIndex + bool(LocalIndexA & 0x1) + bool(LocalIndexA & 0x2) * DataSize + bool(LocalIndexA & 0x4) * DataSize * DataSize;
					const uint32 IndexB = VoxelIndex + bool(LocalIndexB & 0x1) + bool(LocalIndexB & 0x2) * DataSize + bool(LocalIndexB & 0x4) * DataSize * DataSize;

					const float ValueAtA = Densities[IndexA];
					const float ValueAtB = Densities[IndexB];

					ensureVoxelSlow((ValueAtA > 0) != (ValueAtB > 0));

					const uint8 EdgeIndex = ((EdgeCode >> 8) & 0x0F);
					checkVoxelSlow(0 <= EdgeIndex && EdgeIndex < 3);

					// Direction to go to use an already created vertex: 
					// first bit:  x is different
					// second bit: y is different
					// third bit:  z is different
					// fourth bit: vertex isn't cached
					const uint8 CacheDirection = EdgeCode >> 12;

					const bool bIsVertexCached = ((ValidityMask & CacheDirection) == CacheDirection) && CacheDirection; // CacheDirection == 0 => LocalIndexB = 0 (as only B can be = 7) and ValueAtB = 0

					if (bIsVertexCached)
					{
						checkVoxelSlow(!(CacheDirection & 0x08));

						const bool XIsDifferent = !!(CacheDirection & 0x01);
						const bool YIsDifferent = !!(CacheDirection & 0x02);
						const bool ZIsDifferent = !!(CacheDirection & 0x04);
						
						VertexIndex = (ZIsDifferent ? OldCache : CurrentCache)[GetCacheIndex(EdgeIndex, LX - XIsDifferent, LY - YIsDifferent)];
						ensureVoxelSlow(-1 <= VertexIndex && VertexIndex < OutVerticesX.Num());
						ensureVoxelSlow(-1 <= VertexIndex && VertexIndex < OutVerticesY.Num());
						ensureVoxelSlow(-1 <= VertexIndex && VertexIndex < OutVerticesZ.Num());
					}

					if (!bIsVertexCached || VertexIndex == -1)
					{
						// We are on one the lower edges of the chunk. Compute vertex
					
						const FIntVector PositionA(LX + bool(LocalIndexA & 0x1), LY + bool(LocalIndexA & 0x2), LZ + bool(LocalIndexA & 0x4));
						const FIntVector PositionB(LX + bool(LocalIndexB & 0x1), LY + bool(LocalIndexB & 0x2), LZ + bool(LocalIndexB & 0x4));

						const float Alpha = ValueAtA / (ValueAtA - ValueAtB);

						if (VOXEL_DEBUG)
						{
							FIntVector Offset = FIntVector(ForceInit);
							Offset[EdgeIndex] = 1;
							ensure(PositionA + Offset == PositionB);
						}
						
						FVector3f Position = FVector3f(PositionA);
						Position[EdgeIndex] += Alpha;

						VertexIndex = OutVerticesX.Num();

						ensureVoxelSlow(VertexIndex == OutVerticesX.Add(Position.X));
						ensureVoxelSlow(VertexIndex == OutVerticesY.Add(Position.Y));
						ensureVoxelSlow(VertexIndex == OutVerticesZ.Add(Position.Z));

						// See comment above related to null values
						checkVoxelSlow(CacheDirection);

						// Save vertex if not on edge
						if (CacheDirection & 0x08)
						{
							CurrentCache[GetCacheIndex(EdgeIndex, LX, LY)] = VertexIndex;
						}
					}

					VertexIndices[I] = VertexIndex;
				}

				checkVoxelSlow(OutIndices.Num() % 3 == 0);
				const int32 FirstTriangle = OutIndices.Num() / 3;
				const int32 NumTriangles = CellData.GetTriangleCount();

				FVoxelInt4 Cell;
				Cell.X = LX;
				Cell.Y = LY;
				Cell.Z = LZ;
				Cell.W = (FirstTriangle << 8) | NumTriangles;
				OutCells.Add(Cell);

				for (int32 Index = 0; Index < NumTriangles; Index++)
				{
					OutIndices.Add(VertexIndices[CellData.VertexIndex[3 * Index + 0]]);
					OutIndices.Add(VertexIndices[CellData.VertexIndex[3 * Index + 1]]);
					OutIndices.Add(VertexIndices[CellData.VertexIndex[3 * Index + 2]]);
				}
			}
		}
	}
}