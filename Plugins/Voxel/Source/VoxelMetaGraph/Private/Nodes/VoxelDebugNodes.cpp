// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelDebugNodes.h"

void FVoxelChunkExecObject_DrawDebugPoints::Create(FVoxelRuntime& Runtime) const
{
	ensure(!DebugDrawId.IsValid());
	DebugDrawId = Runtime.GetSubsystem<FVoxelDebugDrawSubsystem>().DrawPoints(Points);
}

void FVoxelChunkExecObject_DrawDebugPoints::Destroy(FVoxelRuntime& Runtime) const
{
	ensure(DebugDrawId.IsValid());
	Runtime.GetSubsystem<FVoxelDebugDrawSubsystem>().DestroyDraw(DebugDrawId);
	DebugDrawId = {};
}

TVoxelFutureValue<FVoxelChunkExecObject> FVoxelChunkExecNode_DrawDebugPoints::Execute(const FVoxelQuery& Query) const
{
	const TValue<TBufferView<FVector>> Positions = GetNodeRuntime().GetBufferView(PositionPin, Query);
	const TValue<TBufferView<FLinearColor>> Colors = GetNodeRuntime().GetBufferView(ColorPin, Query);
	const TValue<TBufferView<float>> Sizes = GetNodeRuntime().GetBufferView(SizePin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, Positions, Colors, Sizes)
	{
		const int32 Num = ComputeVoxelBuffersNum(Positions, Colors, Sizes);

		const TSharedRef<FVoxelChunkExecObject_DrawDebugPoints> Object = MakeShared<FVoxelChunkExecObject_DrawDebugPoints>();
		FVoxelUtilities::SetNumFast(Object->Points, Num);

		const FMatrix LocalToWorld = GetNodeRuntime().GetRuntime().LocalToWorld();
		for (int32 Index = 0; Index < Num; Index++)
		{
			Object->Points[Index] = FBatchedPoint(
				LocalToWorld.TransformPosition(FVector(Positions[Index])),
				Colors[Index],
				Sizes[Index],
				0,
				SDPG_World);
		}

		return Object;
	};
}