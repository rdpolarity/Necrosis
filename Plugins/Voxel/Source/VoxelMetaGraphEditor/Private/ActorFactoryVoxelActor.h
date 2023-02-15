// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "ActorFactoryVoxelActor.generated.h"

UCLASS()
class UActorFactoryVoxelActor : public UActorFactory
{
	GENERATED_BODY()

public:
	UActorFactoryVoxelActor();
};

UCLASS()
class UActorFactoryVoxelActorMetaGraph : public UActorFactory
{
	GENERATED_BODY()

public:
	UActorFactoryVoxelActorMetaGraph();

	//~ Begin UActorFactory Interface
	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
	virtual UObject* GetAssetFromActorInstance(AActor* ActorInstance) override;
	//~ End UActorFactory Interface
};