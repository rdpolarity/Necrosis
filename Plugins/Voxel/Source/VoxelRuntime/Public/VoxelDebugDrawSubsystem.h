// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelRuntime/VoxelSubsystem.h"
#include "Components/LineBatchComponent.h"
#include "VoxelDebugDrawSubsystem.generated.h"

DECLARE_UNIQUE_VOXEL_ID(FVoxelDebugDrawId);

UCLASS()
class VOXELRUNTIME_API UVoxelDebugDrawSubsystemProxy : public UVoxelSubsystemProxy
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(FVoxelDebugDrawSubsystem);
};

class VOXELRUNTIME_API FVoxelDebugDrawSubsystem : public IVoxelSubsystem
{
public:
	GENERATED_VOXEL_SUBSYSTEM_BODY(UVoxelDebugDrawSubsystemProxy);

	//~ Begin IVoxelSubsystem Interface
	virtual void Destroy() override;
	//~ End IVoxelSubsystem Interface

	// Points should be in world space
	FVoxelDebugDrawId DrawPoints(TConstArrayView<FBatchedPoint> Points);

	void DestroyDraw(FVoxelDebugDrawId DebugDrawId);

private:
	int32 NumPointsDrawn = 0;
	TMap<FVoxelDebugDrawId, TWeakObjectPtr<ULineBatchComponent>> LineBatchComponents;
};