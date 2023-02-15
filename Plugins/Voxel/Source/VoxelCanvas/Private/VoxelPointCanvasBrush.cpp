// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelPointCanvasBrush.h"
#include "VoxelPointCanvasData.h"
#include "VoxelPointCanvasAsset.h"
#include "Nodes/VoxelPositionNodes.h"

DEFINE_VOXEL_BRUSH(FVoxelPointCanvasBrush);

bool FVoxelPointCanvasBrush::IsValid() const
{
	return
		CanvasData.IsValid() &&
		CanvasData->LODs.Num() > 0;
}

void FVoxelPointCanvasBrush::CacheData_GameThread()
{
	check(IsInGameThread());

	if (!Canvas)
	{
		return;
	}

	CanvasData = Canvas->GetData();
	Canvas = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelPointCanvasBrushImpl::FVoxelPointCanvasBrushImpl(const FVoxelPointCanvasBrush& Brush, const FMatrix& WorldToLocal)
	: TVoxelBrushImpl(Brush, WorldToLocal)
	, Canvas(Brush.CanvasData.ToSharedRef())
{
	const FMatrix44f DataToLocal =
		FMatrix44f(
			FScaleMatrix(Brush.Scale) *
			Brush.BrushToWorld *
			WorldToLocal);

	LocalToData = DataToLocal.Inverse();
	DataToLocalScale = DataToLocal.GetScaleVector().GetAbsMax();

	SetBounds(FVoxelBox(
		Canvas->GetFirstLOD().GetBounds().ToVoxelBox()
		.Extend(1)
		.TransformBy(DataToLocal)));
}

float FVoxelPointCanvasBrushImpl::GetDistance(const FVector& InLocalPosition) const
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_USE_NAMESPACE(PointCanvas);

	const FLODData& LOD = Canvas->GetFirstLOD();
	FVoxelScopeLock_Read Lock(LOD.CriticalSection);

	const FVector3f Position = LocalToData.TransformPosition(FVector3f(InLocalPosition));

	float Distance = 0.f;
	const FChunk* Chunk = nullptr;
	FIntVector ChunkKey = FIntVector(MAX_int32);
	if (!FUtilities::FindClosestPosition(Distance, LOD, Position, Chunk, ChunkKey))
	{
		return 1e6;
	}

	return Distance * DataToLocalScale;
}

TSharedPtr<FVoxelDistanceField> FVoxelPointCanvasBrushImpl::GetDistanceField() const
{
	const TSharedRef<FVoxelPointCanvasDistanceField> DistanceField = MakeShared<FVoxelPointCanvasDistanceField>();
	DistanceField->Brush = SharedThis(this);
	DistanceField->Bounds = GetBounds();
	DistanceField->Smoothness = Brush.Smoothness;
	DistanceField->bIsSubtractive = Brush.bSubtractive;
	DistanceField->Priority = Brush.Priority;
	return DistanceField;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE_CPU(FVoxelNode_FVoxelPointCanvasDistanceField_GetDistances, Distance)
{
	VOXEL_USE_NAMESPACE(PointCanvas);
	FindVoxelQueryData(FVoxelLODQueryData, LODQueryData);
	FindVoxelQueryData(FVoxelPositionQueryData, PositionQueryData);
	FindVoxelQueryData(FVoxelGradientStepQueryData, GradientStepQueryData);

	const TValue<TBufferView<FVector>> Positions = PositionQueryData->GetPositions().MakeView();

	return VOXEL_ON_COMPLETE(AsyncThread, LODQueryData, GradientStepQueryData, Positions)
	{
		const TVoxelArray<TSharedPtr<FLODData>>& LODs = Brush->Canvas->LODs;
		if (LODs.Num() == 0)
		{
			return FVoxelFloatBuffer::Constant(1e6);
		}

		const int32 LOD = FMath::Clamp(LODQueryData->LOD, 0, 0); // LODs.Num() - 1); TODO
		const FLODData& Data = *LODs[LOD];
		const float DistanceScale = (1 << LOD) * Brush->DataToLocalScale;
		const FMatrix44f LocalToData = Brush->LocalToData * FScaleMatrix44f(1.f / (1 << LOD));

		const FChunk* Chunk = nullptr;
		FIntVector ChunkKey = FIntVector(MAX_int32);

		if (DistanceScale <= GradientStepQueryData->Step)
		{
			VOXEL_SCOPE_COUNTER_FORMAT("LOD=%d Num=%d Interpolate=false", LOD, Positions.Num());

			FVoxelScopeLock_Read Lock(Data.CriticalSection);

			TVoxelArray<float> Distances = FVoxelFloatBuffer::Allocate(Positions.Num());
			for (int32 Index = 0; Index < Positions.Num(); Index++)
			{
				const FVector3f Position = LocalToData.TransformPosition(Positions[Index]);

				float Distance = 0.f;
				if (!FUtilities::FindClosestPosition(Distance, Data, Position, Chunk, ChunkKey))
				{
					Distances[Index] = 1e6;
					continue;
				}

				Distances[Index] = Distance * DistanceScale;
			}

			return FVoxelFloatBuffer::MakeCpu(Distances);
		}
		else
		{
			VOXEL_SCOPE_COUNTER_FORMAT("LOD=%d Num=%d Interpolate=true", LOD, Positions.Num());

			FVoxelScopeLock_Read Lock(Data.CriticalSection);

			TVoxelArray<float> Distances = FVoxelFloatBuffer::Allocate(Positions.Num());
			for (int32 Index = 0; Index < Positions.Num(); Index++)
			{
				const FVector3f Position = LocalToData.TransformPosition(Positions[Index]);
				const FIntVector Min = FVoxelUtilities::FloorToInt(Position);
				const FIntVector Max = Min + 1;
				const FVector3f Alpha = Position - FVector3f(Min);

				float Distance000 = 0.f;
				float Distance001 = 0.f;
				float Distance010 = 0.f;
				float Distance011 = 0.f;
				float Distance100 = 0.f;
				float Distance101 = 0.f;
				float Distance110 = 0.f;
				float Distance111 = 0.f;
				if (!FUtilities::FindClosestPosition(Distance000, Data, FVector3f(Min.X, Min.Y, Min.Z), Chunk, ChunkKey) ||
					!FUtilities::FindClosestPosition(Distance001, Data, FVector3f(Max.X, Min.Y, Min.Z), Chunk, ChunkKey) ||
					!FUtilities::FindClosestPosition(Distance010, Data, FVector3f(Min.X, Max.Y, Min.Z), Chunk, ChunkKey) ||
					!FUtilities::FindClosestPosition(Distance011, Data, FVector3f(Max.X, Max.Y, Min.Z), Chunk, ChunkKey) ||
					!FUtilities::FindClosestPosition(Distance100, Data, FVector3f(Min.X, Min.Y, Max.Z), Chunk, ChunkKey) ||
					!FUtilities::FindClosestPosition(Distance101, Data, FVector3f(Max.X, Min.Y, Max.Z), Chunk, ChunkKey) ||
					!FUtilities::FindClosestPosition(Distance110, Data, FVector3f(Min.X, Max.Y, Max.Z), Chunk, ChunkKey) ||
					!FUtilities::FindClosestPosition(Distance111, Data, FVector3f(Max.X, Max.Y, Max.Z), Chunk, ChunkKey))
				{
					Distances[Index] = 1e6;
					continue;
				}

				Distances[Index] = FVoxelUtilities::TrilinearInterpolation(
					Distance000,
					Distance001,
					Distance010,
					Distance011,
					Distance100,
					Distance101,
					Distance110,
					Distance111,
					Alpha.X,
					Alpha.Y,
					Alpha.Z) * DistanceScale;
			}

			return FVoxelFloatBuffer::MakeCpu(Distances);
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelFutureValue<FVoxelFloatBuffer> FVoxelPointCanvasDistanceField::GetDistances(const FVoxelQuery& Query) const
{
	return VOXEL_CALL_NODE(FVoxelNode_FVoxelPointCanvasDistanceField_GetDistances, DistancePin)
	{
		Node.Brush = Brush;
	};
}