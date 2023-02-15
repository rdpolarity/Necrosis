// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelFoliageNodes.h"
#include "VoxelFoliageUtilities.h"
#include "Nodes/VoxelPositionNodes.h"

DEFINE_VOXEL_NODE(FVoxelNode_GenerateManualFoliageData, ChunkData)
{
	FindVoxelQueryData(FVoxelBoundsQueryData, BoundsQueryData);

	const TValue<FVoxelStaticMesh> StaticMesh = Get(StaticMeshPin, Query);

	const TValue<FVoxelVectorBuffer> Positions = Get(PositionsPin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, StaticMesh, Positions)
	{
		if (StaticMesh.StaticMesh.IsExplicitlyNull())
		{
			VOXEL_MESSAGE(Error, "{0}: Static Mesh is null", this);
			return {};
		}

		if (Positions.Num() == 0)
		{
			return {};
		}

		FVoxelQuery PositionQuery = Query;
		PositionQuery.Add<FVoxelSparsePositionQueryData>().Initialize(Positions);

		const TValue<TBufferView<FQuat>> Rotations = GetBufferView(RotationsPin, PositionQuery);
		const TValue<TBufferView<FVector>> Scales = GetBufferView(ScalesPin, PositionQuery);
		const TValue<FVoxelFoliageSettings> FoliageSettings = Get(FoliageSettingsPin, Query);
		const TValue<FBodyInstance> BodyInstance = Get(BodyInstancePin, Query);

		const TValue<TBufferView<FVector>> PositionsView = Positions.MakeView();

		return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, StaticMesh, FoliageSettings, BodyInstance, PositionsView, Rotations, Scales)
		{
			const int32 Num = ComputeVoxelBuffersNum(PositionsView, Rotations, Scales);

			if (Num == 0)
			{
				return {};
			}

			const TSharedRef<TVoxelArray<FTransform3f>> Transforms = MakeShared<TVoxelArray<FTransform3f>>();
			FVoxelUtilities::SetNumFast(*Transforms, Num);

			for (int32 Index = 0; Index < Num; Index++)
			{
				(*Transforms)[Index] = FTransform3f
				(
					Rotations[Index],
					PositionsView[Index] - FVector3f(BoundsQueryData->Bounds.Min),
					Scales[Index]
				);
			}

			const TSharedRef<FVoxelFoliageChunkData> Result = MakeShared<FVoxelFoliageChunkData>();
			Result->ChunkPosition = BoundsQueryData->Bounds.Min;
			Result->InstancesCount = Num;

			const TSharedRef<FVoxelFoliageChunkMeshData> ResultMeshData = MakeShared<FVoxelFoliageChunkMeshData>();
			ResultMeshData->StaticMesh = StaticMesh.StaticMesh;
			ResultMeshData->FoliageSettings = *FoliageSettings;
			ResultMeshData->BodyInstance = *BodyInstance;
			ResultMeshData->Transforms = Transforms;
			Result->Data.Add(ResultMeshData);

			return Result;
		};
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_GenerateHaltonPositions, Positions)
{
	FindVoxelQueryData(FVoxelBoundsQueryData, BoundsQueryData);

	const TValue<float> DistanceBetweenPositions = Get(DistanceBetweenPositionsPin, Query);
	const TValue<int32> Seed = Get(SeedPin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, DistanceBetweenPositions, Seed)
	{
		FVoxelVector2DBuffer Result;
		if (!FVoxelFoliageUtilities::GenerateHaltonPositions(this, BoundsQueryData->Bounds, DistanceBetweenPositions, Seed, Result.X, Result.Y))
		{
			return {};
		}

		return Result;
	};
}