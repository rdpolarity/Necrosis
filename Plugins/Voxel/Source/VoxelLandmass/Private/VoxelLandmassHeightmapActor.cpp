// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelLandmassHeightmapActor.h"
#include "VoxelLandmassHeightmapPreviewMesh.h"
#include "VoxelMesh/VoxelMeshComponent.h"

#if WITH_EDITOR
#include "EditorReimportHandler.h"
#endif

#if WITH_EDITOR
VOXEL_RUN_ON_STARTUP_GAME(RegisterLandmassHeightmapActorDelegates)
{
	FReimportManager::Instance()->OnPostReimport().AddLambda([](UObject* Asset, bool bSuccess)
	{
		for (const AVoxelLandmassHeightmapActor* Actor : TObjectRange<AVoxelLandmassHeightmapActor>())
		{
			if (Actor->MeshComponent)
			{
				Actor->MeshComponent->SetMesh({});
			}
		}
	});

	const auto OnSelectionChanged = []
	{
		VOXEL_FUNCTION_COUNTER_LLM();

		for (AVoxelLandmassHeightmapActor* Actor : TObjectRange<AVoxelLandmassHeightmapActor>())
		{
			if (Actor->MeshComponent &&
				Actor->MeshComponent->IsSelectedInEditor())
			{
				GEditor->SelectComponent(Actor->MeshComponent, false, false);
				GEditor->SelectActor(Actor, true, false);
			}

			const bool bVisible = Actor->IsSelectedInEditor();
			if (!bVisible)
			{
				if (Actor->MeshComponent)
				{
					Actor->MeshComponent->SetMesh({});
				}
				continue;
			}

			Actor->UpdatePreviewMesh();
		}
	};

	USelection::SelectObjectEvent.AddLambda([=](UObject*)
	{
		OnSelectionChanged();
	});
	USelection::SelectionChangedEvent.AddLambda([=](UObject*)
	{
		OnSelectionChanged();
	});
	USelection::SelectNoneEvent.AddLambda([=]
	{
		OnSelectionChanged();
	});
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

AVoxelLandmassHeightmapActor::AVoxelLandmassHeightmapActor()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>("Root");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void AVoxelLandmassHeightmapActor::UpdatePreviewMesh()
{
	if (!MeshComponent)
	{
		MeshComponent = NewObject<UVoxelMeshComponent>(this, NAME_None, RF_Transient | RF_TextExportTransient);
		MeshComponent->CastShadow = false;
		MeshComponent->SetHiddenInGame(true);
		MeshComponent->SetupAttachment(RootComponent);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->RegisterComponent();
	}

	if (!Brush.Heightmap ||
		!Brush.Heightmap->Heightmap)
	{
		MeshComponent->SetMesh({});
		return;
	}

	const TSharedRef<FVoxelLandmassHeightmapPreviewMesh> PreviewMesh = MakeVoxelMesh<FVoxelLandmassHeightmapPreviewMesh>();
	PreviewMesh->Initialize(*Brush.Heightmap);
	MeshComponent->SetMesh(PreviewMesh);
}