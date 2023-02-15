// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelRuntimeSettings.generated.h"

USTRUCT()
struct VOXELRUNTIME_API FVoxelRuntimeSettings
{
	GENERATED_BODY()
		
public:
	FVoxelRuntimeSettings() = default;
	FVoxelRuntimeSettings(
		AActor* Actor,
		USceneComponent* RootComponent)
		: World(Actor->GetWorld())
		, Actor(Actor)
		, RootComponent(RootComponent)
	{
		ensure(Actor);
		ensure(RootComponent);
	}

private:
	UPROPERTY()
	TObjectPtr<UWorld> World = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY()
	TObjectPtr<USceneComponent> RootComponent = nullptr;
	
public:
	FORCEINLINE UWorld* GetWorld() const
	{
		return World;
	}
	FORCEINLINE AActor* GetActor() const
	{
		checkVoxelSlow(IsInGameThread());
		return Actor;
	}
	FORCEINLINE USceneComponent* GetRootComponent() const
	{
		checkVoxelSlow(IsInGameThread());
		return RootComponent;
	}
};