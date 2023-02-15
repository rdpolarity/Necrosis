// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "ActorFactoryVoxelActor.h"
#include "VoxelActor.h"

DEFINE_VOXEL_PLACEABLE_ITEM_FACTORY(UActorFactoryVoxelActor);

UActorFactoryVoxelActor::UActorFactoryVoxelActor()
{
	DisplayName = VOXEL_LOCTEXT("Voxel Actor");
	NewActorClass = AVoxelActor::StaticClass();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UActorFactoryVoxelActorMetaGraph::UActorFactoryVoxelActorMetaGraph()
{
	DisplayName = VOXEL_LOCTEXT("Voxel Actor");
	NewActorClass = AVoxelActor::StaticClass();
}

bool UActorFactoryVoxelActorMetaGraph::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	bool bIsMacroGraph = false;

	return
		AssetData.IsValid() &&
		AssetData.GetClass()->IsChildOf<UVoxelMetaGraph>() &&
		AssetData.GetTagValue(GET_MEMBER_NAME_STRING_CHECKED(UVoxelMetaGraph, bIsMacroGraph), bIsMacroGraph) &&
		!bIsMacroGraph;
}

void UActorFactoryVoxelActorMetaGraph::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);

	AVoxelActor* VoxelActor = CastChecked<AVoxelActor>(NewActor);
	VoxelActor->MetaGraph = CastChecked<UVoxelMetaGraph>(Asset);
	VoxelActor->Fixup();
	VoxelActor->DestroyRuntime();
	VoxelActor->CreateRuntime();
}

UObject* UActorFactoryVoxelActorMetaGraph::GetAssetFromActorInstance(AActor* ActorInstance)
{
	return CastChecked<AVoxelActor>(ActorInstance)->MetaGraph.LoadSynchronous();
}