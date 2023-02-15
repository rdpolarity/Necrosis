// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelFoliageUtilities.h"
#include "Nodes/VoxelPositionNodes.h"
#include "VoxelFoliageAssetNodesImpl.ispc.generated.h"

bool FVoxelFoliageUtilities::GenerateHaltonPositions(const FVoxelNode* Node, const FVoxelBox& Bounds, const double DistanceBetweenPositions, uint32 Seed, FVoxelFloatBuffer &OutPositionsX, FVoxelFloatBuffer& OutPositionsY)
{
	const int32 ChunkSize = FMath::CeilToInt(FMath::Max(DistanceBetweenPositions * 16.f, 16.f));
	const FVoxelIntBox BoundsWithPadding = FVoxelIntBox::FromFloatBox_WithPadding(Bounds);

	TArray<FVoxelIntBox> Chunks;
	if (!ensure(BoundsWithPadding.Subdivide(ChunkSize, Chunks, false, 1024 * 1024)))
	{
		VOXEL_MESSAGE(Warning, "{0}: failed to generate positions", Node);
		return false;
	}

	const double PositionArea = FMath::Square<double>(DistanceBetweenPositions);
	const int64 NumPositions = FMath::CeilToInt64(int64(ChunkSize) * ChunkSize / PositionArea);

	if (NumPositions * Chunks.Num() > 1024 * 1024)
	{
		VOXEL_MESSAGE(Error, "{0}: more than 1M positions would be generated, aborting", Node);
		return false;
	}

	TVoxelArray<float> PositionsX;
	TVoxelArray<float> PositionsY;
	FVoxelBuffer::Reserve(PositionsX, NumPositions * Chunks.Num());
	FVoxelBuffer::Reserve(PositionsY, NumPositions * Chunks.Num());

	for (const FVoxelIntBox& Chunk : Chunks)
	{
		uint32 SeedValue = FVoxelUtilities::MurmurHash(Seed);
		SeedValue ^= FVoxelUtilities::MurmurHash(Chunk);

		for (int32 Index = 0; Index < NumPositions; Index++)
		{
			FVector Position = FVector::ZeroVector;
			Position.X = Chunk.Min.X + FVoxelUtilities::Halton<2>(SeedValue) * (Chunk.Max.X - Chunk.Min.X);
			Position.Y = Chunk.Min.Y + FVoxelUtilities::Halton<3>(SeedValue) * (Chunk.Max.Y - Chunk.Min.Y);
			SeedValue++;

			if (!Bounds.Contains(Position))
			{
				continue;
			}

			PositionsX.Add(Position.X);
			PositionsY.Add(Position.Y);
		}
	}

	FVoxelBuffer::Shrink(PositionsX);
	FVoxelBuffer::Shrink(PositionsY);

	OutPositionsX = FVoxelFloatBuffer::MakeCpu(PositionsX);
	OutPositionsY = FVoxelFloatBuffer::MakeCpu(PositionsY);

	return
		OutPositionsX.Num() > 0 &&
		OutPositionsY.Num() > 0;
}

TVoxelFutureValue<TVoxelBufferView<float>> FVoxelFoliageUtilities::SplitGradientsBuffer(const FVoxelNodeRuntime& Runtime, const FVoxelQuery& Query, const TVoxelPinRef<FVoxelFloatBuffer>& HeightPin, FVoxelVectorBufferView PositionsView, float Step)
{
	ensure(!PositionsView.X.IsConstant() || PositionsView.IsConstant());
	ensure(!PositionsView.Y.IsConstant() || PositionsView.IsConstant());

	TVoxelArray<float> PositionsX = FVoxelFloatBuffer::Allocate(PositionsView.Num() * 4);
	TVoxelArray<float> PositionsY = FVoxelFloatBuffer::Allocate(PositionsView.Num() * 4);

	ispc::VoxelFoliageAssetNodes_SplitGradientBuffer(
		PositionsView.X.GetData(),
		PositionsView.Y.GetData(),
		PositionsView.Num(),
		Step / 2.f,
		PositionsX.GetData(),
		PositionsY.GetData());

	FVoxelVectorBuffer GradientPositionsBuffer;
	GradientPositionsBuffer.X = FVoxelFloatBuffer::MakeCpu(PositionsX);
	GradientPositionsBuffer.Y = FVoxelFloatBuffer::MakeCpu(PositionsY);
	GradientPositionsBuffer.Z = FVoxelFloatBuffer::Constant(0.f);

	FVoxelQuery GradientQuery = Query;
	GradientQuery.Add<FVoxelSparsePositionQueryData>().Initialize(GradientPositionsBuffer);

	return Runtime.GetBufferView(HeightPin, GradientQuery);
}

FVoxelVectorBufferView FVoxelFoliageUtilities::CollapseGradient(FVoxelFloatBufferView GradientHeight, FVoxelVectorBufferView PositionsView, float Step)
{
	FVoxelVectorBuffer GradientBuffer;
	if (GradientHeight.IsConstant())
	{
		GradientBuffer.X = FVoxelFloatBuffer::Constant(0.f);
		GradientBuffer.Y = FVoxelFloatBuffer::Constant(0.f);
		GradientBuffer.Z = FVoxelFloatBuffer::Constant(0.f);
	}
	else
	{
		ensure(PositionsView.Num() * 4 == GradientHeight.Num());

		TVoxelArray<float> GradientX = FVoxelFloatBuffer::Allocate(PositionsView.Num());
		TVoxelArray<float> GradientY = FVoxelFloatBuffer::Allocate(PositionsView.Num());
		TVoxelArray<float> GradientZ = FVoxelFloatBuffer::Allocate(PositionsView.Num());

		ispc::VoxelFoliageAssetNodes_GetGradientCollapse(
			GradientHeight.GetData(),
			FVoxelBuffer::AlignNum(GradientHeight.Num()),
			Step,
			GradientX.GetData(),
			GradientY.GetData(),
			GradientZ.GetData());

		GradientBuffer.X = FVoxelFloatBuffer::MakeCpu(GradientX);
		GradientBuffer.Y = FVoxelFloatBuffer::MakeCpu(GradientY);
		GradientBuffer.Z = FVoxelFloatBuffer::MakeCpu(GradientZ);
	}

	return GradientBuffer.MakeView().Get_CheckCompleted();
}