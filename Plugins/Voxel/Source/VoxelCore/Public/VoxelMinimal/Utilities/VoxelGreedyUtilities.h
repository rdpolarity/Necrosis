// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "VoxelMinimal/Containers/VoxelStaticBitArray.h"

namespace FVoxelUtilities
{
	struct FGreedyMeshingQuad
	{
		uint32 Layer;
		uint32 StartX;
		uint32 StartY;
		uint32 SizeX;
		uint32 SizeY;
	};

	template<uint32 GridSize, typename Allocator>
	void GreedyMeshing2D_Static(TVoxelStaticBitArray<GridSize* GridSize* GridSize>& InFaces, TArray<FGreedyMeshingQuad, Allocator>& OutQuads)
	{
		for (uint32 Layer = 0; Layer < GridSize; Layer++)
		{
			static_assert(((GridSize * GridSize) % TVoxelStaticBitArray<GridSize* GridSize* GridSize>::NumBitsPerWord) == 0, "");
			auto& Faces = reinterpret_cast<TVoxelStaticBitArray<GridSize* GridSize>&>(*(InFaces.GetWordData() + Layer * GridSize * GridSize / InFaces.NumBitsPerWord));

			const auto TestAndClear = [&](uint32 X, uint32 Y)
			{
				checkVoxelSlow(X < GridSize);
				checkVoxelSlow(Y < GridSize);

				return Faces.TestAndClear(X + Y * GridSize);
			};
			const auto TestAndClearLine = [&](uint32 X, uint32 Width, uint32 Y)
			{
				checkVoxelSlow(X + Width <= GridSize);
				checkVoxelSlow(Y < GridSize);

				return Faces.TestAndClearRange(X + Y * GridSize, Width);
			};

			uint32 StartY = 0;
			uint32 EndY = GridSize;
			if (GridSize == 32)
			{
				// Skip empty words
				while (StartY < GridSize && !Faces.GetWord(StartY))
				{
					StartY++;
				}
				if (StartY == GridSize)
				{
					continue;
				}
				while (!Faces.GetWord(EndY - 1))
				{
					EndY--;
					checkVoxelSlow(EndY > StartY);
				}
			}

			for (uint32 X = 0; X < GridSize; X++)
			{
				const uint32 Mask = 1 << X;
				for (uint32 Y = StartY; Y < EndY;)
				{
					if (GridSize == 32)
					{
						// Simpler logic, as Y is the word index
						if (!(Faces.GetWord(Y) & Mask))
						{
							Y++;
							continue;
						}
					}
					else
					{
						if (!Faces.Test(X + Y * GridSize))
						{
							Y++;
							continue;
						}
					}

					uint32 Width = 1;
					while (X + Width < GridSize && TestAndClear(X + Width, Y))
					{
						Width++;
					}

					uint32 Height = 1;
					while (Y + Height < GridSize && TestAndClearLine(X, Width, Y + Height))
					{
						Height++;
					}

					OutQuads.Emplace(FGreedyMeshingQuad{ Layer, X, Y, Width, Height });

					Y += Height;
				}
			}
		}
	}

	template<uint32 GridSize, typename Allocator>
	void GreedyMeshing3D_Static(TVoxelStaticBitArray<GridSize* GridSize* GridSize>& Data, TArray<FVoxelIntBox, Allocator>& OutCubes)
	{
		const auto TestAndClear = [&](uint32 X, uint32 Y, uint32 Z)
		{
			checkVoxelSlow(X < GridSize);
			checkVoxelSlow(Y < GridSize);
			checkVoxelSlow(Z < GridSize);

			return Data.TestAndClear(X + Y * GridSize + Z * GridSize * GridSize);
		};
		const auto TestAndClearLine = [&](uint32 X, uint32 SizeX, uint32 Y, uint32 Z)
		{
			checkVoxelSlow(X + SizeX <= GridSize);
			checkVoxelSlow(Y < GridSize);
			checkVoxelSlow(Z < GridSize);

			return Data.TestAndClearRange(X + Y * GridSize + Z * GridSize * GridSize, SizeX);
		};
		const auto TestAndClearBlock = [&](uint32 X, uint32 SizeX, uint32 Y, uint32 SizeY, uint32 Z)
		{
			checkVoxelSlow(X + SizeX <= GridSize);
			checkVoxelSlow(Y + SizeY <= GridSize);
			checkVoxelSlow(Z < GridSize);

			for (uint32 Index = 0; Index < SizeY; Index++)
			{
				if (!Data.TestRange(X + (Y + Index) * GridSize + Z * GridSize * GridSize, SizeX))
				{
					return false;
				}
			}
			for (uint32 Index = 0; Index < SizeY; Index++)
			{
				Data.SetRange(X + (Y + Index) * GridSize + Z * GridSize * GridSize, SizeX, false);
			}
			return true;
		};

		for (uint32 X = 0; X < GridSize; X++)
		{
			for (uint32 Y = 0; Y < GridSize; Y++)
			{
				for (uint32 Z = 0; Z < GridSize;)
				{
					if (!Data.Test(X + Y * GridSize + Z * GridSize * GridSize))
					{
						Z++;
						continue;
					}

					uint32 SizeX = 1;
					while (X + SizeX < GridSize && TestAndClear(X + SizeX, Y, Z))
					{
						SizeX++;
					}

					uint32 SizeY = 1;
					while (Y + SizeY < GridSize && TestAndClearLine(X, SizeX, Y + SizeY, Z))
					{
						SizeY++;
					}

					uint32 SizeZ = 1;
					while (Z + SizeZ < GridSize && TestAndClearBlock(X, SizeX, Y, SizeY, Z + SizeZ))
					{
						SizeZ++;
					}

					const auto Min = FIntVector(X, Y, Z);
					const auto Max = Min + FIntVector(SizeX, SizeY, SizeZ);
					OutCubes.Add(FVoxelIntBox(Min, Max));

					Z += SizeZ;
				}
			}
		}
	}

	template<typename Allocator>
	void GreedyMeshing3D_Dynamic(const FIntVector& GridSize, FVoxelBitArray32& Data, TArray<FVoxelIntBox, Allocator>& OutCubes)
	{
		const auto TestAndClear = [&](int32 X, int32 Y, int32 Z)
		{
			checkVoxelSlow(X < GridSize.X);
			checkVoxelSlow(Y < GridSize.Y);
			checkVoxelSlow(Z < GridSize.Z);

			return Data.TestAndClear(X + Y * GridSize.X + Z * GridSize.X * GridSize.Y);
		};
		const auto TestAndClearLine = [&](int32 X, int32 SizeX, int32 Y, int32 Z)
		{
			checkVoxelSlow(X + SizeX <= GridSize.X);
			checkVoxelSlow(Y < GridSize.Y);
			checkVoxelSlow(Z < GridSize.Z);

			return Data.TestAndClearRange(X + Y * GridSize.X + Z * GridSize.X * GridSize.Y, SizeX);
		};
		const auto TestAndClearBlock = [&](int32 X, int32 SizeX, int32 Y, int32 SizeY, int32 Z)
		{
			checkVoxelSlow(X + SizeX <= GridSize.X);
			checkVoxelSlow(Y + SizeY <= GridSize.Y);
			checkVoxelSlow(Z < GridSize.Z);

			for (int32 Index = 0; Index < SizeY; Index++)
			{
				if (!Data.TestRange(X + (Y + Index) * GridSize.X + Z * GridSize.X * GridSize.Y, SizeX))
				{
					return false;
				}
			}
			for (int32 Index = 0; Index < SizeY; Index++)
			{
				Data.SetRange(X + (Y + Index) * GridSize.X + Z * GridSize.X * GridSize.Y, SizeX, false);
			}
			return true;
		};

		for (int32 X = 0; X < GridSize.X; X++)
		{
			for (int32 Y = 0; Y < GridSize.Y; Y++)
			{
				for (int32 Z = 0; Z < GridSize.Z;)
				{
					if (!Data[X + Y * GridSize.X + Z * GridSize.X * GridSize.Y])
					{
						Z++;
						continue;
					}

					int32 SizeX = 1;
					while (X + SizeX < GridSize.X && TestAndClear(X + SizeX, Y, Z))
					{
						SizeX++;
					}

					int32 SizeY = 1;
					while (Y + SizeY < GridSize.Y && TestAndClearLine(X, SizeX, Y + SizeY, Z))
					{
						SizeY++;
					}

					int32 SizeZ = 1;
					while (Z + SizeZ < GridSize.Z && TestAndClearBlock(X, SizeX, Y, SizeY, Z + SizeZ))
					{
						SizeZ++;
					}

					const auto Min = FIntVector(X, Y, Z);
					const auto Max = Min + FIntVector(SizeX, SizeY, SizeZ);
					OutCubes.Add(FVoxelIntBox(Min, Max));

					Z += SizeZ;
				}
			}
		}
	}
}
