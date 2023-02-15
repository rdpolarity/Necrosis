// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelRuntime/VoxelRuntimeUtilities.h"
#include "VoxelRuntime/VoxelRuntimeActor.h"
#include "VoxelRuntime/VoxelSubsystem.h"

FVoxelRuntimeUtilities::FOnRuntimeCreated FVoxelRuntimeUtilities::OnRuntimeCreated;

TSharedPtr<FVoxelRuntime> FVoxelRuntimeUtilities::GetRuntime(const AActor* Actor)
{
	if (!Actor ||
		!Actor->IsA<AVoxelRuntimeActor>())
	{
		return nullptr;
	}

	return CastChecked<AVoxelRuntimeActor>(Actor)->GetRuntime();
}

void FVoxelRuntimeUtilities::RecreateRuntime(const FVoxelRuntime& Runtime)
{
	AVoxelRuntimeActor* RuntimeActor = Cast<AVoxelRuntimeActor>(Runtime.GetSettings().GetActor());
	if (!ensure(RuntimeActor))
	{
		return;
	}

	if (RuntimeActor->GetRuntime().IsValid())
	{
		RuntimeActor->DestroyRuntime();
	}
	RuntimeActor->CreateRuntime();
}

void FVoxelRuntimeUtilities::ForeachRuntime(UWorld* World, TFunctionRef<void(FVoxelRuntime&)> Lambda)
{
	VOXEL_FUNCTION_COUNTER_LLM();

	if (World)
	{
		for (const AVoxelRuntimeActor* Actor : TActorRange<AVoxelRuntimeActor>(World))
		{
			const TSharedPtr<FVoxelRuntime> Runtime = Actor->GetRuntime();
			if (!Runtime)
			{
				continue;
			}

			Lambda(*Runtime);
		}
	}
	else
	{
		for (const AVoxelRuntimeActor* Actor : TObjectRange<AVoxelRuntimeActor>())
		{
			const TSharedPtr<FVoxelRuntime> Runtime = Actor->GetRuntime();
			if (!Runtime)
			{
				continue;
			}

			Lambda(*Runtime);
		}
	}
}

void FVoxelRuntimeUtilities::ForeachSubsystem(UWorld* World, TSubclassOf<UVoxelSubsystemProxy> Class, TFunctionRef<void(IVoxelSubsystem&)> Lambda)
{
	VOXEL_FUNCTION_COUNTER_LLM();

	ForeachRuntime(World, [&](const FVoxelRuntime& Runtime)
	{
		Lambda(Runtime.GetSubsystem(Class));
	});
}