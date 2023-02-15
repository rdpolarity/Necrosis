// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "ActorFactoryVoxelPointCanvas.generated.h"

UCLASS()
class UActorFactoryVoxelPointCanvas : public UActorFactory
{
	GENERATED_BODY()

public:
	UActorFactoryVoxelPointCanvas();

	//~ Begin UActorFactory Interface
	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
	virtual UObject* GetAssetFromActorInstance(AActor* ActorInstance) override;
	//~ End UActorFactory Interface
};