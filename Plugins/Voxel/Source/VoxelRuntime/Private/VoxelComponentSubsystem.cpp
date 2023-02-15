// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelComponentSubsystem.h"

DEFINE_VOXEL_SUBSYSTEM(FVoxelComponentSubsystem);

bool FVoxelComponentSubsystem::bDisableModify = false;

void FVoxelComponentSubsystem::Destroy()
{
	Super::Destroy();

	if (GExitPurge)
	{
		return;
	}

	const TSet<FObjectKey> ComponentsCopy = Components;
	for (const FObjectKey Component : ComponentsCopy)
	{
		UObject* Object = Component.ResolveObjectPtr();
		if (!Object ||
			!Object->IsA<UActorComponent>())
		{
			// Will happen on level teardown
			continue;
		}

		DestroyComponent(CastChecked<UActorComponent>(Object));
	}
	ensure(Components.Num() == 0);
}

UActorComponent* FVoxelComponentSubsystem::CreateComponent(const UClass* Class)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(Class) ||
		!ensure(GetRootComponent()))
	{
		return nullptr;
	}

	ensure(!bDisableModify);
	bDisableModify = true;

	UActorComponent* Component = NewObject<UActorComponent>(GetRootComponent(), Class, NAME_None, RF_Transient);

	ensure(bDisableModify);
	bDisableModify = false;

	Components.Add(Component);

	return Component;
}

void FVoxelComponentSubsystem::DestroyComponent(UActorComponent* Component)
{
	if (GExitPurge ||
		!ensure(Component))
	{
		return;
	}
	
	if (!ensure(GetActor()) ||
		!ensure(GetActor() == Component->GetOwner()) ||
		!ensure(Components.Contains(Component)))
	{
		Component->DestroyComponent();
		return;
	}

	Components.Remove(Component);

	ensure(!bDisableModify);
	bDisableModify = true;

	Component->DestroyComponent();

	ensure(bDisableModify);
	bDisableModify = false;
}

void FVoxelComponentSubsystem::SetupSceneComponent(USceneComponent& Component) const
{
	if (ensure(GetRootComponent()))
	{
		Component.SetupAttachment(GetRootComponent(), NAME_None);
	}

	Component.SetRelativeTransform(FTransform::Identity);
}

void FVoxelComponentSubsystem::SetComponentPosition(USceneComponent& Component, const FVector3d& Position) const
{
	VOXEL_FUNCTION_COUNTER();

	Component.SetRelativeLocation_Direct(FVector(Position));
	Component.SetRelativeRotation_Direct(FRotator::ZeroRotator);
	Component.SetRelativeScale3D_Direct(FVector::OneVector);
	Component.UpdateComponentToWorld(EUpdateTransformFlags::None, ETeleportType::TeleportPhysics);
}