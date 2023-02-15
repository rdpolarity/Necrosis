// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelDebugDrawSubsystem.h"
#include "VoxelComponentSubsystem.h"

DEFINE_UNIQUE_VOXEL_ID(FVoxelDebugDrawId);
DEFINE_VOXEL_SUBSYSTEM(FVoxelDebugDrawSubsystem);

void FVoxelDebugDrawSubsystem::Destroy()
{
	Super::Destroy();

	LineBatchComponents.Reset();
}

FVoxelDebugDrawId FVoxelDebugDrawSubsystem::DrawPoints(TConstArrayView<FBatchedPoint> Points)
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelDebugDrawId DrawId = FVoxelDebugDrawId::New();

	ULineBatchComponent* LineBatchComponent = GetSubsystem<FVoxelComponentSubsystem>().CreateComponent<ULineBatchComponent>();
	LineBatchComponent->PrimaryComponentTick.bCanEverTick = false;
	LineBatchComponent->RegisterComponent();

	if (NumPointsDrawn + Points.Num() > 100000)
	{
		VOXEL_MESSAGE(Error, "More than 100000 debug points being drawn, aborting");
	}
	else
	{
		NumPointsDrawn += Points.Num();

		LineBatchComponent->BatchedPoints = Points;
		LineBatchComponent->MarkRenderStateDirty();
	}

	LineBatchComponents.Add(DrawId, LineBatchComponent);

	return DrawId;
}

void FVoxelDebugDrawSubsystem::DestroyDraw(const FVoxelDebugDrawId DebugDrawId)
{
	TWeakObjectPtr<ULineBatchComponent> LineBatchComponent;
	if (IsDestroyed() ||
		!ensure(LineBatchComponents.RemoveAndCopyValue(DebugDrawId, LineBatchComponent)) ||
		!ensure(LineBatchComponent.IsValid()))
	{
		return;
	}

	NumPointsDrawn -= LineBatchComponent->BatchedPoints.Num();

	GetSubsystem<FVoxelComponentSubsystem>().DestroyComponent(LineBatchComponent.Get());
}