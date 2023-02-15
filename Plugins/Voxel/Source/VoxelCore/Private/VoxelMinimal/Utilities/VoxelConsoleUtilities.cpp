// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMinimal.h"

static FAutoConsoleCommandWithWorldAndArgs ToggleNamedEventsCmd(
    TEXT("voxel.toggleNamedEvents"),
    TEXT("Toggle verbose named events (expensive!)"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        static bool bToggled = false;
        
        if (GCycleStatsShouldEmitNamedEvents == 0)
        {
            GCycleStatsShouldEmitNamedEvents++;
		}

		if (!bToggled)
		{
#if VOXEL_ENGINE_VERSION < 501
			StatsMasterEnableAdd();
#else
			StatsPrimaryEnableAdd();
#endif
		}
		else
		{
#if VOXEL_ENGINE_VERSION < 501
			StatsMasterEnableSubtract();
#else
			StatsPrimaryEnableSubtract();
#endif
		}
        
        bToggled = !bToggled;
    }));