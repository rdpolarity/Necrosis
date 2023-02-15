// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelRuntime/VoxelRuntime.h"
#include "VoxelRuntime/VoxelRuntimeActor.h"
#include "VoxelRuntime/VoxelRuntimeUtilities.h"
#include "VoxelRuntime/VoxelSubsystemStatScope.h"
#include "Tickable.h"
	
class FVoxelRuntimeTickable : public FTickableGameObject
{
public:
	FVoxelRuntime* Runtime = nullptr;

	virtual TStatId GetStatId() const final override
	{
		return Runtime->GetStatId();
	}
	virtual bool IsTickableInEditor() const final override
	{
		return true;
	}
	virtual ETickableTickType GetTickableTickType() const override
	{
		return ETickableTickType::Always;
	}
	virtual void Tick(float DeltaTime) override
	{
		Runtime->Tick();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_UNIQUE_VOXEL_ID(FVoxelRuntimeId);

FVoxelRuntime::FVoxelRuntime()
{
}

FVoxelRuntime::~FVoxelRuntime()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	if (!ensure(!bIsCreated))
	{
		Destroy();
	}
	ensure(!Tickable);
	ensure(!bIsCreated);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelRuntime::CreateInternal()
{
	VOXEL_FUNCTION_COUNTER();
	FVoxelSubsystemStatScope Scope(this);
	check(IsInGameThread());

	LOG_VOXEL(Log, "Creating %s", *Settings.GetActor()->GetName());

	if (!ensure(!bIsCreated))
	{
		return false;
	}

	bIsCreated = true;

	check(!Tickable);
	Tickable = MakeUnique<FVoxelRuntimeTickable>();
	Tickable->Runtime = this;
	
	// Add subsystems
	for (const TSubclassOf<UVoxelSubsystemProxy> Class : GetDerivedClasses<UVoxelSubsystemProxy>())
	{
		if (Class->HasAnyClassFlags(CLASS_Abstract))
		{
			continue;
		}

		const TSharedRef<IVoxelSubsystem> Subsystem = Class.GetDefaultObject()->GetSubsystem();
		Subsystem->Internal_Initialize(*this);

		SubsystemsArray.Add(Subsystem);
		SubsystemsMap.Add(Class, Subsystem);
	}

	PrivateLocalToWorld = Settings.GetActor()->ActorToWorld().ToMatrixWithScale();
	PrivateWorldToLocal = PrivateLocalToWorld.Inverse();

	// Setup subsystems
	for (const TSharedPtr<IVoxelSubsystem>& Subsystem : SubsystemsArray)
	{
		if (!Subsystem->bIsInitialized)
		{
			Subsystem->CallInitialize();
		}
	}

	MetaGraphRuntime->Create();

	return true;
}

TSharedPtr<FVoxelRuntime> FVoxelRuntime::CreateRuntime(const FVoxelRuntimeSettings& Settings)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	const TSharedRef<FVoxelRuntime> Runtime = MakeShared_GameThread<FVoxelRuntime>();
	Runtime->Settings = Settings;
	Runtime->MetaGraphRuntime = CastChecked<AVoxelRuntimeActor>(Settings.GetActor())->MakeMetaGraphRuntime(*Runtime);
#if STATS
	Runtime->StatId = FDynamicStats::CreateStatId<FStatGroup_STATGROUP_VoxelSubsystems>("FVoxelRuntime " + Settings.GetActor()->GetPathName());
#endif

	if (!ensure(Runtime->CreateInternal()))
	{
		return nullptr;
	}

	FVoxelRuntimeUtilities::OnRuntimeCreated.Broadcast(*Runtime);
	
	return Runtime;
}

void FVoxelRuntime::Destroy()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	FVoxelSubsystemStatScope Scope(this);

	if (!ensure(bIsCreated))
	{
		return;
	}

	if (ensure(Settings.GetActor()))
	{
		LOG_VOXEL(Log, "Destroying %s", *Settings.GetActor()->GetName());
	}

	OnDestroy.Broadcast();

	MetaGraphRuntime->Destroy();

	for (const TSharedPtr<IVoxelSubsystem>& Subsystem : SubsystemsArray)
	{
		ensure(Subsystem->bIsInitialized);
		ensure(!Subsystem->bIsDestroyed);
		Subsystem->bIsDestroyed = true;
	}
	for (const TSharedPtr<IVoxelSubsystem>& Subsystem : SubsystemsArray)
	{
		Subsystem->CallDestroy();
	}

	bIsCreated = false;

	check(Tickable);
	Tickable.Reset();
}

void FVoxelRuntime::AddReferencedObjects(FReferenceCollector& Collector)
{
	VOXEL_FUNCTION_COUNTER();
	FVoxelSubsystemStatScope Scope(this);

	FVoxelRuntimeSettings::StaticStruct()->SerializeItem(Collector.GetVerySlowReferenceCollectorArchive(), &Settings, nullptr);

	for (const TSharedPtr<IVoxelSubsystem>& Subsystem : SubsystemsArray)
	{
		Subsystem->CallAddReferencedObjects(Collector);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

IVoxelSubsystem& FVoxelRuntime::GetSubsystem(TSubclassOf<UVoxelSubsystemProxy> Class) const
{
	IVoxelSubsystem& Subsystem = *SubsystemsMap[Class];
	if (!Subsystem.bIsInitialized)
	{
		Subsystem.CallInitialize();
	}
	return Subsystem;
}

TArray<IVoxelSubsystem*> FVoxelRuntime::GetAllSubsystems() const
{
	VOXEL_FUNCTION_COUNTER();

	TArray<IVoxelSubsystem*> Result;
	for (const TSharedPtr<IVoxelSubsystem>& Subsystem : SubsystemsArray)
	{
		if (!Subsystem->bIsInitialized)
		{
			Subsystem->CallInitialize();
		}
		Result.Add(Subsystem.Get());
	}
	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelRuntime::AsyncTask(ENamedThreads::Type Thread, TFunction<void()> Function)
{
	if (Thread == ENamedThreads::GameThread)
	{
		// Ensure tasks are run at a sane time
		QueuedGameThreadTasks.Enqueue(MoveTemp(Function));
		return;
	}

	::AsyncTask(Thread, MakeWeakLambda(MoveTemp(Function)));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelRuntime::Tick()
{
	VOXEL_FUNCTION_COUNTER_LLM();
	checkNoRecursion();
	
	if (ensure(Settings.GetActor()))
	{
		PrivateLocalToWorld = Settings.GetActor()->ActorToWorld().ToMatrixWithScale();
		PrivateWorldToLocal = PrivateLocalToWorld.Inverse();
	}

	FVector CameraPosition;
	if (FVoxelGameUtilities::GetCameraView(Settings.GetWorld(), CameraPosition))
	{
		PriorityPosition = CameraPosition;
	}

	for (const TSharedPtr<IVoxelSubsystem>& Subsystem : SubsystemsArray)
	{
		Subsystem->CallTick();
	}

	MetaGraphRuntime->Tick();

	{
		VOXEL_SCOPE_COUNTER("Process tasks");

		TFunction<void()> Task;
		while (QueuedGameThreadTasks.Dequeue(Task))
		{
			Task();
		}
	}
}