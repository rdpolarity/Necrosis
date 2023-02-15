// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "ActorFactoryVoxelHeightmap.h"
#include "VoxelLandmassHeightmapActor.h"

UActorFactoryVoxelHeightmap::UActorFactoryVoxelHeightmap()
{
	DisplayName = VOXEL_LOCTEXT("Voxel Actor");
	NewActorClass = AVoxelLandmassHeightmapActor::StaticClass();
}

bool UActorFactoryVoxelHeightmap::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	return AssetData.IsValid() && AssetData.GetClass() == UVoxelHeightmap::StaticClass();
}

void UActorFactoryVoxelHeightmap::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);

	AVoxelLandmassHeightmapActor* LandmassActor = CastChecked<AVoxelLandmassHeightmapActor>(NewActor);
	LandmassActor->Brush.Heightmap = CastChecked<UVoxelHeightmap>(Asset);
}

UObject* UActorFactoryVoxelHeightmap::GetAssetFromActorInstance(AActor* ActorInstance)
{
	return CastChecked<AVoxelLandmassHeightmapActor>(ActorInstance)->Brush.Heightmap;
}