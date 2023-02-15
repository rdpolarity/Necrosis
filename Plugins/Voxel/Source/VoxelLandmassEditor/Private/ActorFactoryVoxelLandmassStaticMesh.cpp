// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "ActorFactoryVoxelLandmassStaticMesh.h"
#include "VoxelLandmassActor.h"
#include "LevelEditor.h"
#include "Engine/StaticMeshActor.h"

class FVoxelLandmassCommands : public TVoxelCommands<FVoxelLandmassCommands>
{
public:
	TSharedPtr<FUICommandInfo> ToggleLandmassMode;

	virtual void RegisterCommands() override;
};

DEFINE_VOXEL_COMMANDS(FVoxelLandmassCommands);

void FVoxelLandmassCommands::RegisterCommands()
{
	VOXEL_UI_COMMAND(ToggleLandmassMode, "Toggle Landmass Mode", "Toggles between spawning static meshes and voxel landmass actors", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Alt, EKeys::V));
}

VOXEL_RUN_ON_STARTUP_EDITOR(VoxelLandmassFactorySetup)
{
	for (UActorFactory* Factory : GEditor->ActorFactories)
	{
		if (Factory->GetClass() != UActorFactoryVoxelLandmassStaticMesh::StaticClass())
		{
			continue;
		}

		GEditor->ActorFactories.Insert(Factory, 0);
		break;
	}

	FVoxelLandmassCommands::Register();

	const FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetGlobalLevelEditorActions()->MapAction(
		FVoxelLandmassCommands::Get().ToggleLandmassMode,
		FExecuteAction::CreateLambda([]
		{
			UActorFactoryVoxelLandmassStaticMesh* Factory = Cast<UActorFactoryVoxelLandmassStaticMesh>(GEditor->ActorFactories[0]);
			if (!ensure(Factory))
			{
				return;
			}

			Factory->bSpawnStaticMesh = !Factory->bSpawnStaticMesh;
			Factory->NewActorClass = Factory->bSpawnStaticMesh ? AStaticMeshActor::StaticClass() : AVoxelLandmassActor::StaticClass();

			VOXEL_MESSAGE(Info, "Voxel Landmass Mode is {0}", Factory->bSpawnStaticMesh ? "OFF" : "ON");
		}));
}

class UActorFactoryDummy : public UActorFactory
{
public:
	using UActorFactory::PostSpawnActor;
	using UActorFactory::PostCreateBlueprint;
};

static UActorFactoryDummy* GetStaticMeshFactory()
{
	static UClass* Class = FindObject<UClass>(nullptr, TEXT("/Script/UnrealEd.ActorFactoryStaticMesh"));
	check(Class);
	return static_cast<UActorFactoryDummy*>(Class->GetDefaultObject<UActorFactory>());
}

UActorFactoryVoxelLandmassStaticMesh::UActorFactoryVoxelLandmassStaticMesh()
{
	DisplayName = VOXEL_LOCTEXT("Voxel Landmass Static Mesh");
	NewActorClass = AStaticMeshActor::StaticClass();
	bUseSurfaceOrientation = true;
}

bool UActorFactoryVoxelLandmassStaticMesh::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	return
		AssetData.IsValid() &&
		AssetData.GetClass() &&
		AssetData.GetClass()->IsChildOf<UStaticMesh>();
}

void UActorFactoryVoxelLandmassStaticMesh::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	if (bSpawnStaticMesh)
	{
		GetStaticMeshFactory()->PostSpawnActor(Asset, NewActor);
	}
	else
	{
		Super::PostSpawnActor(Asset, NewActor);

		AVoxelLandmassActor* LandmassActor = CastChecked<AVoxelLandmassActor>(NewActor);
		LandmassActor->Brush.Mesh = CastChecked<UStaticMesh>(Asset);

		if (LandmassActor->HasAllFlags(RF_Transient))
		{
			LandmassActor->UpdateSelectionStatus(true);
			LandmassActor->RemoveBrush();
		}
	}
}

void UActorFactoryVoxelLandmassStaticMesh::PostCreateBlueprint(UObject* Asset, AActor* CDO)
{
	if (bSpawnStaticMesh)
	{
		GetStaticMeshFactory()->PostCreateBlueprint(Asset, CDO);
	}
	else
	{
		Super::PostCreateBlueprint(Asset, CDO);
	}
}

UObject* UActorFactoryVoxelLandmassStaticMesh::GetAssetFromActorInstance(AActor* ActorInstance)
{
	if (bSpawnStaticMesh)
	{
		return GetStaticMeshFactory()->GetAssetFromActorInstance(ActorInstance);
	}
	else
	{
		return CastChecked<AVoxelLandmassActor>(ActorInstance)->Brush.Mesh;
	}
}

FQuat UActorFactoryVoxelLandmassStaticMesh::AlignObjectToSurfaceNormal(const FVector& InSurfaceNormal, const FQuat& ActorRotation) const
{
	if (bSpawnStaticMesh)
	{
		return GetStaticMeshFactory()->AlignObjectToSurfaceNormal(InSurfaceNormal, ActorRotation);
	}
	else
	{
		return Super::AlignObjectToSurfaceNormal(InSurfaceNormal, ActorRotation);
	}
}