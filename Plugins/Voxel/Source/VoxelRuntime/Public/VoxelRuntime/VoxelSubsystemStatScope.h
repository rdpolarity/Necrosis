// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelRuntime/VoxelSubsystem.h"

class FVoxelSubsystemStatScope
#if STATS
	: public FCycleCounter
#endif
{
public:
	FORCEINLINE explicit FVoxelSubsystemStatScope(const IVoxelSubsystem* Subsystem)
	{
#if STATS
		if (Subsystem)
		{
			Start(Subsystem->GetStatId());
		}
#endif
	}
	FORCEINLINE explicit FVoxelSubsystemStatScope(const IVoxelSubsystem& Subsystem)
		: FVoxelSubsystemStatScope(&Subsystem)
	{
	}
	FORCEINLINE explicit FVoxelSubsystemStatScope(const TSharedPtr<IVoxelSubsystem>& Subsystem)
		: FVoxelSubsystemStatScope(Subsystem.Get())
	{
	}

public:
	FORCEINLINE explicit FVoxelSubsystemStatScope(const FVoxelRuntime* Runtime)
	{
#if STATS
		if (Runtime)
		{
			Start(Runtime->GetStatId());
		}
#endif
	}
	FORCEINLINE explicit FVoxelSubsystemStatScope(const FVoxelRuntime& Runtime)
		: FVoxelSubsystemStatScope(&Runtime)
	{
	}
	FORCEINLINE explicit FVoxelSubsystemStatScope(const TSharedPtr<FVoxelRuntime>& Runtime)
		: FVoxelSubsystemStatScope(Runtime.Get())
	{
	}

public:
	FORCEINLINE ~FVoxelSubsystemStatScope()
	{
#if STATS
		Stop();
#endif
	}
};