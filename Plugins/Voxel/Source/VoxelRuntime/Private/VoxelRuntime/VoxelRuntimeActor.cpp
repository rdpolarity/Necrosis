// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelRuntime/VoxelRuntimeActor.h"
#include "VoxelRuntime/VoxelSubsystem.h"
#include "VoxelComponentSubsystem.h"
#include "EditorSupportDelegates.h"

void AVoxelRuntimeActor::BeginPlay()
{
	VOXEL_FUNCTION_COUNTER_LLM();

	Super::BeginPlay();

	if (!PrivateRuntime.IsValid() && bCreateRuntimeOnBeginPlay)
	{
		CreateRuntime();
	}
}

void AVoxelRuntimeActor::BeginDestroy()
{
	VOXEL_FUNCTION_COUNTER_LLM();

	if (PrivateRuntime.IsValid())
	{
		DestroyRuntime();
	}

	Super::BeginDestroy();
}

void AVoxelRuntimeActor::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	VOXEL_FUNCTION_COUNTER_LLM();

	if (PrivateRuntime.IsValid())
	{
		DestroyRuntime();
	}
	
	Super::EndPlay(EndPlayReason);
}

void AVoxelRuntimeActor::Destroyed()
{
	VOXEL_FUNCTION_COUNTER_LLM();
	
	if (PrivateRuntime.IsValid())
	{
		DestroyRuntime();
	}
	
	Super::Destroyed();
}

void AVoxelRuntimeActor::OnConstruction(const FTransform& Transform)
{
	VOXEL_FUNCTION_COUNTER_LLM();

	Super::OnConstruction(Transform);

#if WITH_EDITOR
	if (!PrivateRuntime.IsValid() &&
		GetWorld() &&
		!GetWorld()->IsGameWorld() &&
		!HasAnyFlags(RF_ClassDefaultObject) &&
		!IsRunningCommandlet())
	{
		CreateRuntime();
	}
#endif
}

void AVoxelRuntimeActor::PostEditImport()
{
	VOXEL_FUNCTION_COUNTER_LLM();

	Super::PostEditImport();

	if (PrivateRuntime.IsValid())
	{
		DestroyRuntime();
		CreateRuntime();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void AVoxelRuntimeActor::PreEditChange(FProperty* PropertyThatWillChange)
{
	// BEGIN - AActor::PreEditChange bypass to avoid extremely expensive component re-registration

	UObject::PreEditChange(PropertyThatWillChange);

	const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(PropertyThatWillChange);
	UBlueprintGeneratedClass* BlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(GetClass());
	if (ObjectProperty && BlueprintGeneratedClass)
	{
		BlueprintGeneratedClass->UnbindDynamicDelegatesForProperty(this, ObjectProperty);
	}

	PreEditChangeDataLayers.Reset();

	if (!PropertyThatWillChange)
	{
		return;
	}
	
#if VOXEL_ENGINE_VERSION >= 501
	if (PropertyThatWillChange->GetFName() == GET_MEMBER_NAME_CHECKED(AVoxelRuntimeActor, DataLayerAssets))
	{
		PreEditChangeDataLayers = DataLayerAssets;
	}
#else
	if (PropertyThatWillChange->GetFName() == GET_MEMBER_NAME_CHECKED(AVoxelRuntimeActor, DataLayers) ||
		PropertyThatWillChange->GetFName() == GET_MEMBER_NAME_CHECKED(FActorDataLayer, Name))
	{
		PreEditChangeDataLayers = DataLayers;
	}
#endif

	// END - AActor::PreEditChange
}

void AVoxelRuntimeActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// BEGIN - AActor::PostEditChangeProperty bypass to avoid extremely expensive component re-registration
	
	if (IsPropertyChangedAffectingDataLayers(PropertyChangedEvent))
	{
		FixupDataLayers(true);
	}

	if (PropertyChangedEvent.GetPropertyName() == USceneComponent::GetRelativeLocationPropertyName() ||
		PropertyChangedEvent.GetPropertyName() == USceneComponent::GetRelativeRotationPropertyName() ||
		PropertyChangedEvent.GetPropertyName() == USceneComponent::GetRelativeScale3DPropertyName())
	{
		GEngine->BroadcastOnActorMoved(this);
	}

	FEditorSupportDelegates::UpdateUI.Broadcast();

	UObject::PostEditChangeProperty(PropertyChangedEvent);

	// END - AActor::PostEditChangeChainProperty
}

bool AVoxelRuntimeActor::Modify(bool bAlwaysMarkDirty)
{
	if (FVoxelComponentSubsystem::bDisableModify)
	{
		return false;
	}

	return Super::Modify(bAlwaysMarkDirty);
}

void AVoxelRuntimeActor::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (bFinished && PrivateRuntime.IsValid())
	{
		PrivateRuntime->OnPostEditMove.Broadcast();
	}
}
#endif

TSharedRef<IVoxelMetaGraphRuntime> AVoxelRuntimeActor::MakeMetaGraphRuntime(FVoxelRuntime& Runtime) const
{
	unimplemented();
	return {};
}

void AVoxelRuntimeActor::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	// Can't use FGCObject because it causes a reference loop
	// VoxelRuntimeActor -> VoxelRuntime -> Settings -> VoxelRuntimeActor
	//
	// This isn't an issue in the general case as when the VoxelRuntime actor is destroyed the runtime is too,
	// but it breaks reference detection when deleting a (map containing a) voxel actor

	VOXEL_FUNCTION_COUNTER_LLM();

	const AVoxelRuntimeActor* This = CastChecked<AVoxelRuntimeActor>(InThis);
	if (!This->PrivateRuntime)
	{
		return;
	}

	This->PrivateRuntime->AddReferencedObjects(Collector);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool AVoxelRuntimeActor::CreateRuntime()
{
	VOXEL_FUNCTION_COUNTER();
	
	if (!ensure(!PrivateRuntime))
	{
		return false;
	}

	PrivateRuntime = FVoxelRuntime::CreateRuntime(FVoxelRuntimeSettings(this, GetRootComponent()));

	return PrivateRuntime != nullptr;
}

void AVoxelRuntimeActor::DestroyRuntime()
{
	VOXEL_FUNCTION_COUNTER();
	
	if (!ensure(PrivateRuntime))
	{
		return;
	}

	PrivateRuntime->Destroy();
	PrivateRuntime.Reset();
}