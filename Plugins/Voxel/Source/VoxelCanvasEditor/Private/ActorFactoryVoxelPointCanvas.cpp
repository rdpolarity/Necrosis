// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "ActorFactoryVoxelPointCanvas.h"
#include "VoxelPointCanvasAsset.h"
#include "VoxelPointCanvasActor.h"

UActorFactoryVoxelPointCanvas::UActorFactoryVoxelPointCanvas()
{
	DisplayName = VOXEL_LOCTEXT("Voxel Actor");
	NewActorClass = AVoxelPointCanvasActor::StaticClass();
}

bool UActorFactoryVoxelPointCanvas::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	return AssetData.IsValid() && AssetData.GetClass() == UVoxelPointCanvasAsset::StaticClass();
}

void UActorFactoryVoxelPointCanvas::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);

	AVoxelPointCanvasActor* LandmassActor = CastChecked<AVoxelPointCanvasActor>(NewActor);
	LandmassActor->Brush.Canvas = CastChecked<UVoxelPointCanvasAsset>(Asset);
}

UObject* UActorFactoryVoxelPointCanvas::GetAssetFromActorInstance(AActor* ActorInstance)
{
	return CastChecked<AVoxelPointCanvasActor>(ActorInstance)->Brush.Canvas;
}