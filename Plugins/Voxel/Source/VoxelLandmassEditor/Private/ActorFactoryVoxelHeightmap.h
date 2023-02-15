// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "ActorFactoryVoxelHeightmap.generated.h"

UCLASS()
class UActorFactoryVoxelHeightmap : public UActorFactory
{
	GENERATED_BODY()

public:
	UActorFactoryVoxelHeightmap();

	//~ Begin UActorFactory Interface
	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
	virtual UObject* GetAssetFromActorInstance(AActor* ActorInstance) override;
	//~ End UActorFactory Interface
};