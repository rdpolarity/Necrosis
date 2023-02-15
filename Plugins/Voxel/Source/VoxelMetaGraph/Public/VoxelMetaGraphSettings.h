// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelDeveloperSettings.h"
#include "VoxelMetaGraphSettings.generated.h"

class UTexture;

UENUM(BlueprintType)
enum class EVoxelThreadPriority : uint8
{
	Normal,
	AboveNormal,
	BelowNormal,
	Highest,
	Lowest,
	SlightlyBelowNormal,
	TimeCritical
};

UCLASS(config = Engine, defaultconfig, meta = (DisplayName = "Voxel Meta Graph"))
class VOXELMETAGRAPH_API UVoxelMetaGraphSettings : public UVoxelDeveloperSettings
{
	GENERATED_BODY()

public:
	UVoxelMetaGraphSettings()
	{
		SectionName = "Voxel Meta Graph";
	}

	// Number of threads allocated for the voxel background processing. Setting it too high may impact performance
	// The threads are shared across all voxel worlds
	// Can be set using voxel.NumThreads
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Performance", meta = (ClampMin = 1, ConsoleVariable = "voxel.NumThreads"))
	int32 NumberOfThreads;

	// Time, in seconds, during which a task priority is valid and does not need to be recomputed
	// Lowering this will increase async cost to recompute priorities, but will lead to more precise scheduling
	// Increasing this will decreasing async cost to recompute priorities, but might lead to imprecise scheduling if the invokers are moving fast
	// Can be set using voxel.threading.PriorityDuration
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Performance", meta = (ClampMin = 0, ConsoleVariable = "voxel.threading.PriorityDuration"))
	float PriorityDuration;

	// The priority of the voxel threads
	// Changing this requires a restart
	// Can be set using voxel.threading.ThreadPriority
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Performance", meta = (ConsoleVariable = "voxel.threading.ThreadPriority", ConfigRestartRequired = true))
	EVoxelThreadPriority ThreadPriority;
};