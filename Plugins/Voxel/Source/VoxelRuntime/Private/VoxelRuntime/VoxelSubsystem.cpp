// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelRuntime/VoxelSubsystem.h"
#include "VoxelRuntime/VoxelSubsystemStatScope.h"

DEFINE_UNIQUE_VOXEL_ID(FVoxelSubsystemId);
DEFINE_VOXEL_COUNTER(STAT_VoxelNumSubsystems);

TSharedRef<IVoxelSubsystem> UVoxelSubsystemProxy::GetSubsystem() const
{
	check(false);
	return TSharedPtr<IVoxelSubsystem>().ToSharedRef();
}

void UVoxelSubsystemProxy::PostCDOContruct()
{
	VOXEL_FUNCTION_COUNTER_LLM();
	
	Super::PostCDOContruct();

	if (GetClass() == StaticClass())
	{
		return;
	}

#if WITH_EDITOR
	ensure(GetClassFunctions(GetClass()).Num() == 0);

	{
		FString Name = GetClass()->GetName();
		ensureAlways(Name.RemoveFromEnd("Proxy"));

		ensureAlways(GetSubsystemCppName().IsNone() || GetSubsystemCppName().ToString() == "F" + Name);
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString UVoxelSubsystemProxy::GetSubsystemClassDisplayName(const TSubclassOf<UVoxelSubsystemProxy> Class)
{
	if (!Class)
	{
		return "NULL";
	}
	
	FString Name = Class->GetName();
	ensure(Name.RemoveFromEnd("Proxy"));
	return Name;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

IVoxelSubsystem::~IVoxelSubsystem()
{
	ensure(IsInGameThread());
	ensure(bIsInitialized);
	ensure(bIsDestroyed);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void IVoxelSubsystem::Initialize()
{
	check(FunctionBeingCalled == EFunction::Initialize);
	check(!bSuperCalled);
	bSuperCalled = true;
}

void IVoxelSubsystem::Destroy()
{
	check(FunctionBeingCalled == EFunction::Destroy);
	check(!bSuperCalled);
	bSuperCalled = true;
}

void IVoxelSubsystem::Tick()
{
	check(FunctionBeingCalled == EFunction::Tick);
	check(!bSuperCalled);
	bSuperCalled = true;
}

void IVoxelSubsystem::AddReferencedObjects(FReferenceCollector& Collector)
{
	check(FunctionBeingCalled == EFunction::AddReferencedObjects);
	check(!bSuperCalled);
	bSuperCalled = true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


void IVoxelSubsystem::CallInitialize()
{
	check(IsInGameThread());

	if (!ensure(!bIsInitialized))
	{
		return;
	}
	bIsInitialized = true;

	FVoxelSubsystemStatScope Scope(this);

	check(FunctionBeingCalled == EFunction::None);
	FunctionBeingCalled = EFunction::Initialize;

	check(!bSuperCalled);
	bSuperCalled = false;

	Initialize();

	check(bSuperCalled);
	bSuperCalled = false;

	check(FunctionBeingCalled == EFunction::Initialize);
	FunctionBeingCalled = EFunction::None;
}

void IVoxelSubsystem::CallDestroy()
{
	check(IsInGameThread());

	FVoxelSubsystemStatScope Scope(this);
	
	check(FunctionBeingCalled == EFunction::None);
	FunctionBeingCalled = EFunction::Destroy;

	check(!bSuperCalled);
	bSuperCalled = false;

	Destroy();

	check(bSuperCalled);
	bSuperCalled = false;

	check(FunctionBeingCalled == EFunction::Destroy);
	FunctionBeingCalled = EFunction::None;
}

void IVoxelSubsystem::CallTick()
{
	FVoxelSubsystemStatScope Scope(this);

	check(FunctionBeingCalled == EFunction::None);
	FunctionBeingCalled = EFunction::Tick;

	check(!bSuperCalled);
	bSuperCalled = false;

	Tick();

	check(bSuperCalled);
	bSuperCalled = false;

	check(FunctionBeingCalled == EFunction::Tick);
	FunctionBeingCalled = EFunction::None;
}

void IVoxelSubsystem::CallAddReferencedObjects(FReferenceCollector& Collector)
{
	FVoxelSubsystemStatScope Scope(this);

	check(FunctionBeingCalled == EFunction::None);
	FunctionBeingCalled = EFunction::AddReferencedObjects;

	check(!bSuperCalled);
	bSuperCalled = false;

	AddReferencedObjects(Collector);

	check(bSuperCalled);
	bSuperCalled = false;

	check(FunctionBeingCalled == EFunction::AddReferencedObjects);
	FunctionBeingCalled = EFunction::None;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void IVoxelSubsystem::Internal_Initialize(FVoxelRuntime& Runtime)
{
	WeakRuntime = Runtime.AsShared();
	RawRuntime = &Runtime;

	CachedProxyClass = Internal_GetProxyClass();
	check(CachedProxyClass);

	CachedCppName = CachedProxyClass.GetDefaultObject()->GetSubsystemCppName();
	
#if STATS
	StatId = FDynamicStats::CreateStatId<FStatGroup_STATGROUP_VoxelSubsystems>(CachedCppName.ToString());
#endif
}