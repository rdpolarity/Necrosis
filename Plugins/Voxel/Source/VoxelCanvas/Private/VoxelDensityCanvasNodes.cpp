// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelDensityCanvasNodes.h"
#include "Nodes/VoxelPositionNodes.h"

DEFINE_VOXEL_NODE_CPU(FVoxelNode_ApplyDensityCanvas, OutDensity)
{
	VOXEL_USE_NAMESPACE(DensityCanvas);
	FindVoxelQueryData(FVoxelPositionQueryData, PositionQueryData)

	const TValue<FVoxelDensityCanvasData> Canvas = Get(CanvasPin, Query);
	const TValue<TBufferView<FVector>> Positions = PositionQueryData->GetPositions().MakeView();

	return VOXEL_ON_COMPLETE(AsyncThread, PositionQueryData, Canvas, Positions)
	{
		const TSharedPtr<FData> Data = Canvas->Data;
		if (!Data)
		{
			return Get(InDensityPin, Query);
		}

		Data->UseNode(this);

		FVoxelScopeLock_Read Lock(Data->CriticalSection);

		FVoxelBox Bounds;
		{
			VOXEL_SCOPE_COUNTER("Compute Bounds");

			const FFloatInterval X = FVoxelUtilities::GetMinMax(Positions.X.GetRawView());
			const FFloatInterval Y = FVoxelUtilities::GetMinMax(Positions.Y.GetRawView());
			const FFloatInterval Z = FVoxelUtilities::GetMinMax(Positions.Z.GetRawView());

			Bounds.Min.X = X.Min;
			Bounds.Min.Y = Y.Min;
			Bounds.Min.Z = Z.Min;

			Bounds.Max.X = X.Max;
			Bounds.Max.Y = Y.Max;
			Bounds.Max.Z = Z.Max;
		}
		Data->AddDependency(Bounds, Query.AllocateDependency());

		if (!Data->HasChunks(Bounds))
		{
			return Get(InDensityPin, Query);
		}

		const TSharedRef<TVoxelArray<float>> DensitiesPtr = MakeShared<TVoxelArray<float>>();
		DensitiesPtr->Reserve(Positions.Num() * 8);

		TVoxelArray<float> QueryPositionsX;
		TVoxelArray<float> QueryPositionsY;
		TVoxelArray<float> QueryPositionsZ;

		FVoxelBuffer::Reserve(QueryPositionsX, Positions.Num());
		FVoxelBuffer::Reserve(QueryPositionsY, Positions.Num());
		FVoxelBuffer::Reserve(QueryPositionsZ, Positions.Num());

		static constexpr float InvalidDensity = std::numeric_limits<float>::infinity();

		{
			VOXEL_SCOPE_COUNTER("Find chunks");

			FIntVector LastChunkKey = FIntVector(MAX_int32);
			const FChunk* Chunk = nullptr;

			const float VoxelSize = Data->VoxelSize;
			for (int32 Index = 0; Index < Positions.Num(); Index++)
			{
				const FVector3f Position = Positions[Index] / VoxelSize;
				const FIntVector Min = FVoxelUtilities::FloorToInt(Position);
				const FIntVector Max = FVoxelUtilities::CeilToInt(Position);

				const auto GetDensity = [&](const FIntVector& QueryPosition)
				{
					const FIntVector ChunkKey = FVoxelUtilities::DivideFloor_FastLog2(QueryPosition, ChunkSizeLog2);
					if (ChunkKey != LastChunkKey)
					{
						LastChunkKey = ChunkKey;
						Chunk = Data->FindChunk(ChunkKey);
					}

					if (!Chunk)
					{
						DensitiesPtr->Add(InvalidDensity);
						QueryPositionsX.Add(QueryPosition.X * VoxelSize);
						QueryPositionsY.Add(QueryPosition.Y * VoxelSize);
						QueryPositionsZ.Add(QueryPosition.Z * VoxelSize);
						return;
					}

					const FIntVector LocalPosition = QueryPosition - ChunkKey * ChunkSize;
					const FDensity Density = (*Chunk)[FVoxelUtilities::Get3DIndex<int32>(ChunkSize, LocalPosition)];
					DensitiesPtr->Add(FromDensity(Density) * VoxelSize);
				};

				GetDensity(FIntVector(Min.X, Min.Y, Min.Z));

				if (Min != Max)
				{
					GetDensity(FIntVector(Max.X, Min.Y, Min.Z));
					GetDensity(FIntVector(Min.X, Max.Y, Min.Z));
					GetDensity(FIntVector(Max.X, Max.Y, Min.Z));
					GetDensity(FIntVector(Min.X, Min.Y, Max.Z));
					GetDensity(FIntVector(Max.X, Min.Y, Max.Z));
					GetDensity(FIntVector(Min.X, Max.Y, Max.Z));
					GetDensity(FIntVector(Max.X, Max.Y, Max.Z));
				}
			}
		}

		FVoxelBuffer::Shrink(QueryPositionsX);
		FVoxelBuffer::Shrink(QueryPositionsY);
		FVoxelBuffer::Shrink(QueryPositionsZ);

		FVoxelVectorBuffer QueryPositions;
		QueryPositions.X = FVoxelFloatBuffer::MakeCpu(QueryPositionsX);
		QueryPositions.Y = FVoxelFloatBuffer::MakeCpu(QueryPositionsY);
		QueryPositions.Z = FVoxelFloatBuffer::MakeCpu(QueryPositionsZ);
		QueryPositions.SetBounds(Bounds);

		FVoxelQuery QueryCopy = Query;
		QueryCopy.Add<FVoxelSparsePositionQueryData>().Initialize(QueryPositions);

		const TValue<TBufferView<float>> QueriedDensities = GetBufferView(InDensityPin, QueryCopy);

		return VOXEL_ON_COMPLETE(AsyncThread, Data, Positions, DensitiesPtr, QueriedDensities)
		{
			int32 QueryIndex = 0;
			for (int32 Index = 0; Index < DensitiesPtr->Num(); Index++)
			{
				float& Density = (*DensitiesPtr)[Index];
				if (Density != InvalidDensity)
				{
					continue;
				}

				if (!ensure(QueriedDensities.IsConstant() || QueryIndex < QueriedDensities.Num()))
				{
					return {};
				}

				Density = QueriedDensities[QueryIndex++];
			}
			ensure(QueriedDensities.IsConstant() || QueryIndex == QueriedDensities.Num());

			TVoxelArray<float> FinalDensities = FVoxelFloatBuffer::Allocate(Positions.Num());

			int32 DensityIndex = 0;
			for (int32 Index = 0; Index < Positions.Num(); Index++)
			{
				const FVector3f Position = Positions[Index] / Data->VoxelSize;
				const FIntVector Min = FVoxelUtilities::FloorToInt(Position);
				const FIntVector Max = FVoxelUtilities::CeilToInt(Position);

				if (Min == Max)
				{
					FinalDensities[Index] = (*DensitiesPtr)[DensityIndex];
					DensityIndex++;
				}
				else
				{
					const FVector3f Alpha = Position - FVector3f(Min);

					FinalDensities[Index] = FVoxelUtilities::TrilinearInterpolation(
						(*DensitiesPtr)[DensityIndex + 0],
						(*DensitiesPtr)[DensityIndex + 1],
						(*DensitiesPtr)[DensityIndex + 2],
						(*DensitiesPtr)[DensityIndex + 3],
						(*DensitiesPtr)[DensityIndex + 4],
						(*DensitiesPtr)[DensityIndex + 5],
						(*DensitiesPtr)[DensityIndex + 6],
						(*DensitiesPtr)[DensityIndex + 7],
						Alpha.X,
						Alpha.Y,
						Alpha.Z);

					DensityIndex += 8;
				}
			}
			ensure(DensityIndex == DensitiesPtr->Num());

			return FVoxelFloatBuffer::MakeCpu(FinalDensities);
		};
	};
}