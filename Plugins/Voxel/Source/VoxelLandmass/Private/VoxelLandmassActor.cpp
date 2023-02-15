// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelLandmassActor.h"
#include "VoxelLandmassSettings.h"

AVoxelLandmassActor::AVoxelLandmassActor()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>("Root");
}

void AVoxelLandmassActor::UpdateSelectionStatus(bool bForceVisible)
{
#if WITH_EDITOR
	for (UActorComponent* Component : GetComponents())
	{
		if (!IsValid(Component) ||
			!Component->HasAnyFlags(RF_Transient) ||
			!Component->IsSelectedInEditor())
		{
			continue;
		}

		GEditor->SelectComponent(Component, false, false);
		GEditor->SelectActor(this, true, false);
	}
#endif

	const bool bVisible = bForceVisible || bIsSelected
#if WITH_EDITOR
		|| IsSelectedInEditor()
#endif
		;

	const UVoxelLandmassSettings* Settings = GetDefault<UVoxelLandmassSettings>();
	UMaterialInterface* Material = Brush.bInvert ? Settings->InvertedMeshMaterial.LoadSynchronous() : Settings->MeshMaterial.LoadSynchronous();

	if (!StaticMeshComponent)
	{
		if (!bVisible)
		{
			return;
		}

		StaticMeshComponent = CreateMeshComponent<UStaticMeshComponent>();
	}

	StaticMeshComponent->SetVisibility(bVisible);

	if (bVisible)
	{
		StaticMeshComponent->SetStaticMesh(Brush.Mesh);
		StaticMeshComponent->SetMaterial(0, Material);
	}
}

UStaticMeshComponent* AVoxelLandmassActor::CreateMeshComponent(const UClass* Class)
{
	UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(this, Class, NAME_None, RF_Transient | RF_TextExportTransient);
	MeshComponent->CastShadow = false;
	MeshComponent->SetHiddenInGame(!bShowMeshesInGame);
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->RegisterComponent();
	return MeshComponent;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void AVoxelLandmassActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive)
	{
		return;
	}

	UpdateSelectionStatus();
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
VOXEL_RUN_ON_STARTUP_GAME(RegisterLandmassActorSelectionEvent)
{
	USelection::SelectObjectEvent.AddLambda([](UObject*)
	{
		AVoxelLandmassActor::OnSelectionChanged();
	});
	USelection::SelectionChangedEvent.AddLambda([](UObject*)
	{
		AVoxelLandmassActor::OnSelectionChanged();
	});
	USelection::SelectNoneEvent.AddLambda([]
	{
		AVoxelLandmassActor::OnSelectionChanged();
	});
}

void AVoxelLandmassActor::OnSelectionChanged()
{
	VOXEL_FUNCTION_COUNTER_LLM();

	for (AVoxelLandmassActor* Actor : TObjectRange<AVoxelLandmassActor>())
	{
		Actor->UpdateSelectionStatus();
	}
}
#endif