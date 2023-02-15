// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelBrush.h"

DEFINE_UNIQUE_VOXEL_ID(FVoxelBrushInternalId);

FVoxelBrushRegistry* GVoxelBrushRegistry = nullptr;

VOXEL_RUN_ON_STARTUP_GAME(CreateVoxelBrushRegistry)
{
	GVoxelBrushRegistry = new FVoxelBrushRegistry();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint64 FVoxelBrush::GetHash() const
{
	VOXEL_FUNCTION_COUNTER();

	uint64 Hash = 0;
	for (FProperty& Property : GetStructProperties(GetStruct()))
	{
		Hash = FVoxelUtilities::MurmurHash64(Hash ^ FVoxelObjectUtilities::HashProperty(Property, this));
	}
	return Hash;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelArray<TSharedPtr<const FVoxelBrush>> FVoxelBrushRegistry::GetBrushes(const UWorld* World, const UScriptStruct* Struct) const
{
	VOXEL_FUNCTION_COUNTER();
	ensure(Struct && Struct->IsChildOf(FVoxelBrush::StaticStruct()));

	TSharedPtr<FBrushes> Brushes;
	{
		FVoxelScopeLock Lock(CriticalSection);
		Brushes = BrushesMap.FindRef(FKey{ World, Struct });
	}

	if (!Brushes)
	{
		return {};
	}

	TVoxelArray<TSharedPtr<const FVoxelBrush>> OutBrushes;
	{
		FVoxelScopeLock Lock(Brushes->CriticalSection);
		Brushes->Brushes.GenerateValueArray(OutBrushes);
	}
	return OutBrushes;
}

void FVoxelBrushRegistry::AddListener(const UWorld* World, const UScriptStruct* Struct, const FSimpleDelegate& OnChanged)
{
	VOXEL_FUNCTION_COUNTER();
	ensure(Struct && Struct->IsChildOf(FVoxelBrush::StaticStruct()));

	const TSharedRef<FBrushes> Brushes = FindOrAddBrushes(FKey{ World, Struct });

	FVoxelScopeLock Lock(Brushes->CriticalSection);
	Brushes->OnChanged.Add(OnChanged);
}

void FVoxelBrushRegistry::UpdateBrush(const UWorld* World, FVoxelBrushId& BrushId, const TSharedPtr<const FVoxelBrush>& Brush)
{
	VOXEL_FUNCTION_COUNTER();

	if (VOXEL_DEBUG && Brush)
	{
		struct FCollector : public FReferenceCollector
		{
			virtual bool IsIgnoringArchetypeRef() const override { return false; }
			virtual bool IsIgnoringTransient() const override { return false; }

			virtual void HandleObjectReference(UObject*& Object, const UObject* ReferencingObject, const FProperty* ReferencingProperty) override
			{
				ensure(!Object || Object->IsA<UField>());
			}
		};

		FCollector Collector;
		FVoxelObjectUtilities::AddStructReferencedObjects(Collector, Brush->GetStruct(), VOXEL_CONST_CAST(Brush.Get()));
	}

	if (!Brush)
	{
		if (!BrushId.IsValid())
		{
			// Nothing to remove
			return;
		}
	}
	else
	{
		if (!BrushId.IsValid())
		{
			// Allocate new
			ensure(World);

			BrushId.World = World;
			BrushId.Struct = Brush->GetStruct();
			BrushId.Id = FVoxelBrushInternalId::New();
		}
	}
	ensure(BrushId.World);
	ensure(BrushId.World == World || !World);

	for (const UStruct* Struct = BrushId.Struct; Struct != FVoxelVirtualStruct::StaticStruct(); Struct = Struct->GetSuperStruct())
	{
		UpdateBrushImpl(FKey{ BrushId.World, Struct }, BrushId.Id, Brush);
	}

	if (!Brush)
	{
		BrushId = {};
	}
}

///////////////////////////////////////////////////////////////////////////////

TSharedRef<FVoxelBrushRegistry::FBrushes> FVoxelBrushRegistry::FindOrAddBrushes(const FKey& Key, const bool bMarkDirty)
{
	VOXEL_SCOPE_LOCK(CriticalSection);

	if (bMarkDirty)
	{
		DirtyBrushes.Add(Key);
	}

	TSharedPtr<FBrushes>& Brushes = BrushesMap.FindOrAdd(Key);
	if (!Brushes)
	{
		Brushes = MakeShared<FBrushes>();
	}
	return Brushes.ToSharedRef();
}

void FVoxelBrushRegistry::UpdateBrushImpl(const FKey& Key, const FVoxelBrushInternalId BrushId, const TSharedPtr<const FVoxelBrush>& Brush)
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedRef<FBrushes> Brushes = FindOrAddBrushes(Key, true);

	FVoxelScopeLock Lock(Brushes->CriticalSection);

	if (Brush)
	{
		Brushes->Brushes.Add(BrushId, Brush);
	}
	else
	{
		ensure(Brushes->Brushes.Remove(BrushId));
	}
}

///////////////////////////////////////////////////////////////////////////////

void FVoxelBrushRegistry::Tick()
{
	VOXEL_FUNCTION_COUNTER();

	if (FPlatformTime::Seconds() - LastGC > 30.f)
	{
		VOXEL_SCOPE_COUNTER("GC");

		LastGC = FPlatformTime::Seconds();

		TSet<void*> ValidWorlds;
		ForEachObjectOfClass<UWorld>([&](UWorld* World)
		{
			ValidWorlds.Add(World);
		}, false);

		FVoxelScopeLock Lock(CriticalSection);

		for (auto It = BrushesMap.CreateIterator(); It; ++It)
		{
			if (!ValidWorlds.Contains(It.Key().World))
			{
				It.RemoveCurrent();
			}
		}
	}

	TArray<FSimpleMulticastDelegate> Delegates;
	{
		VOXEL_SCOPE_COUNTER("Find updates");

		FVoxelScopeLock Lock(CriticalSection);

		for (const FKey& Key : DirtyBrushes)
		{
			const TSharedPtr<FBrushes> Brushes = BrushesMap.FindRef(Key);
			if (!ensure(Brushes))
			{
				continue;
			}

			Delegates.Add(Brushes->OnChanged);
		}
		DirtyBrushes.Reset();
	}

	VOXEL_SCOPE_COUNTER("Broadcast updates");

	for (const FSimpleMulticastDelegate& Delegate : Delegates)
	{
		Delegate.Broadcast();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

AVoxelBrushActor::~AVoxelBrushActor()
{
	ensure(!BrushId.IsValid());
}

///////////////////////////////////////////////////////////////////////////////

void AVoxelBrushActor::BeginPlay()
{
	VOXEL_FUNCTION_COUNTER_LLM();
	Super::BeginPlay();
	UpdateBrush();
}

void AVoxelBrushActor::BeginDestroy()
{
	VOXEL_FUNCTION_COUNTER_LLM();
	RemoveBrush();
	Super::BeginDestroy();
}

void AVoxelBrushActor::Destroyed()
{
	VOXEL_FUNCTION_COUNTER_LLM();
	RemoveBrush();
	Super::Destroyed();
}

void AVoxelBrushActor::OnConstruction(const FTransform& Transform)
{
	VOXEL_FUNCTION_COUNTER_LLM();
	Super::OnConstruction(Transform);
	UpdateBrush();
}

///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void AVoxelBrushActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	VOXEL_FUNCTION_COUNTER_LLM();

	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!PropertyChangedEvent.MemberProperty ||
		PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive)
	{
		return;
	}

	LOG_VOXEL(Log, "Updating %s: %s changed", *GetName(), *PropertyChangedEvent.MemberProperty->GetName());
	UpdateBrush();
}

void AVoxelBrushActor::PostEditMove(bool bFinished)
{
	VOXEL_FUNCTION_COUNTER_LLM();

	Super::PostEditMove(bFinished);

	if (bFinished ||
		!PostEditMove_LastActorToWorld.IsSet() ||
		!PostEditMove_LastActorToWorld.GetValue().Equals(ActorToWorld()))
	{
		PostEditMove_LastActorToWorld = ActorToWorld();
		UpdateBrush();
	}
}

void AVoxelBrushActor::PostEditUndo()
{
	VOXEL_FUNCTION_COUNTER_LLM();

	Super::PostEditUndo();

	if (IsValid(this))
	{
		UpdateBrush();
	}
	else
	{
		RemoveBrush();
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////

void AVoxelBrushActor::UpdateBrush()
{
	VOXEL_FUNCTION_COUNTER_LLM();
	GVoxelBrushRegistry->UpdateBrush(GetWorld(), BrushId, GetBrush());
}

void AVoxelBrushActor::RemoveBrush()
{
	VOXEL_FUNCTION_COUNTER_LLM();
	GVoxelBrushRegistry->UpdateBrush(GetWorld(), BrushId, nullptr);
}

TSharedPtr<const FVoxelBrush> AVoxelBrushActor::GetBrush() const
{
	const FProperty* Property = GetClass()->FindPropertyByName(STATIC_FNAME("Brush"));
	if (!Property)
	{
		return nullptr;
	}

	const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
	if (!ensure(StructProperty) ||
		!ensure(StructProperty->Struct->IsChildOf(FVoxelBrush::StaticStruct())))
	{
		return nullptr;
	}

	const TSharedRef<FVoxelBrush> Brush = StructProperty->ContainerPtrToValuePtr<FVoxelBrush>(this)->MakeSharedCopy();
	Brush->BrushToWorld = ActorToWorld().ToMatrixWithScale();
	Brush->ActorToSelect = VOXEL_CONST_CAST(this);
	Brush->CacheData_GameThread();
	return Brush;
}