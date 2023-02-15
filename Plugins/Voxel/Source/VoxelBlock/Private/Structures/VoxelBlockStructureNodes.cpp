// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Structures/VoxelBlockStructureNodes.h"
#include "Math/Sobol.h"

#if 0 // TODO
DEFINE_VOXEL_NODE(FVoxelNode_ApplyBlockTree)
{
	ComputeVoxelQueryData(FVoxelBlockChunkQueryData, BlockChunkQueryData);

	constexpr int32 MaxHeight = 12;
	constexpr int32 FoliageSize = 2;

	FVoxelIntBox Bounds = BlockChunkQueryData.Bounds();
	Bounds.Min.Z -= MaxHeight + 1;
	Bounds = Bounds.Extend(2 * FoliageSize);
	Bounds = Bounds.MakeMultipleOfBigger(2);

	FVoxelQuery PositionQuery = Query;
	PositionQuery.Add<FVoxelBlockChunkQueryData>().Initialize(Bounds);
	const TSharedRef<const FVoxelBlockStructurePositions> Positions = PositionsPin.Get(PositionQuery);

	const FVoxelBlockData TrunkBlock = TrunkBlockPin.Get(Query);
	const FVoxelBlockData FoliageBlock = FoliageBlockPin.Get(Query);

	FVoxelBlockDataChunkProvider BlockData(Query, BlockChunkQueryData, BlockPin);

	for (int32 Index = 0; Index < Positions->Positions.Num(); Index++)
	{
		const FIntVector Position = Positions->Positions[Index];

		FRandomStream Stream(FVoxelUtilities::MurmurHash(Position));
		const int32 Height = Stream.RandRange(7, MaxHeight);
		for (int32 Z = 0; Z < Height; Z++)
		{
			BlockData.Get(Position + FIntVector(0, 0, Z)) = TrunkBlock;
		}

		const FIntVector FoliageCenter = Position + FIntVector(0, 0, Height);
		for (int32 X = -FoliageSize; X <= FoliageSize; X++)
		{
			for (int32 Y = -FoliageSize; Y <= FoliageSize; Y++)
			{
				for (int32 Z = -FoliageSize; Z <= FoliageSize; Z++)
				{
					BlockData.Get(FoliageCenter + FIntVector(X, Y, Z)) = FoliageBlock;
				}
			}
		}
	}

	return BlockData.GetBlockDataBuffer(BlockChunkQueryData.Bounds());
}

DEFINE_VOXEL_NODE(FVoxelNode_GetTreePositions)
{
	ComputeVoxelQueryData(FVoxelBlockChunkQueryData, BlockChunkQueryData);

	const float DistanceBetweenTrees = DistanceBetweenTreesPin.Get(Query);

	TUniquePtr<FVoxelBlockStructurePositions> Positions = MakeUnique<FVoxelBlockStructurePositions>();

	const int32 ChunkSize = FMath::RoundUpToPowerOfTwo(FMath::Max(16, FMath::CeilToInt(DistanceBetweenTrees)));
	const float TreeArea = FMath::Max(1.f, FMath::Square(DistanceBetweenTrees));
	const int32 NumTreePerChunk = FMath::CeilToInt(ChunkSize * ChunkSize / TreeArea);

	FVoxelIntBox Bounds2D = BlockChunkQueryData.Bounds();
	Bounds2D.Min.Z = 0;
	Bounds2D.Max.Z = 1;

	TArray<FVoxelIntBox> Children;
	Bounds2D.Subdivide(ChunkSize, Children, false);

	for (const FVoxelIntBox& Child : Children)
	{
		constexpr int32 CellBits = 1;
		const int32 Seed = FVoxelUtilities::MurmurHash(Child.Min);
		const uint32 SeedX = Seed;
		const uint32 SeedY = FVoxelUtilities::MurmurHash32(Seed);
		FVector2D Value = FSobol::Evaluate(0, CellBits, FIntPoint::ZeroValue, FIntPoint(SeedX, SeedY));

		for (int32 Index = 0; Index < NumTreePerChunk; Index++)
		{
			Value = FSobol::Next(Index++, CellBits, Value);

			Positions->Positions.Add(FIntVector(
				Child.Min.X + FMath::FloorToInt(Value.X * ChunkSize),
				Child.Min.Y + FMath::FloorToInt(Value.Y * ChunkSize),
				0));
		}
	}

	FVoxelQuery HeightQuery = Query;
	HeightQuery.Add<FVoxelBlockSparseQueryData>().Initialize(Positions->Positions);
	const TSharedRef<const FVoxelInt32Buffer> Heights = HeightPin.Get(HeightQuery);

	for (int32 Index = 0; Index < Positions->Positions.Num(); Index++)
	{
		Positions->Positions[Index].Z = Heights->GetTyped2(Index);
	}

	Positions->Positions.RemoveAllSwap([&](const FIntVector& Position)
	{
		return !BlockChunkQueryData.Bounds().Contains(Position);
	});

	return Positions;
}
#endif