// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

class AVoxelActor;
class FVoxelRuntime;
class IVoxelMetaGraphRuntime;
class IVoxelSubsystem;
class UVoxelSubsystemProxy;

struct VOXELRUNTIME_API FVoxelRuntimeUtilities
{
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnRuntimeCreated, FVoxelRuntime&);
	static FOnRuntimeCreated OnRuntimeCreated;

	static TSharedPtr<FVoxelRuntime> GetRuntime(const AActor* Actor);
	static void RecreateRuntime(const FVoxelRuntime& Runtime);

	static void ForeachRuntime(UWorld* World, TFunctionRef<void(FVoxelRuntime&)> Lambda);
	static void ForeachSubsystem(UWorld* World, TSubclassOf<UVoxelSubsystemProxy> Class, TFunctionRef<void(IVoxelSubsystem&)> Lambda);
};