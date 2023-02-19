// Copyright 2022 Andras Ketzer, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "ChunkActor.h"
#include "ScatterTemplate.h"
#include "UObject/NoExportTypes.h"
#include "Chunk.generated.h"

UENUM(BlueprintType)
enum class EChunkState : uint8
{
	SPAWNED = 0 UMETA(DisplayName = "Spawned"),
	DE_SPAWNED = 1 UMETA(DisplayName = "Not Spawned")
};

UCLASS()
class NECROSIS_API UChunk : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	EChunkState ChunkState = EChunkState::SPAWNED;

	UPROPERTY(EditAnywhere)
	FVector ChunkPosition;

	UPROPERTY(EditAnywhere)
	class AUChunkSystem* ChunkSystem;
	
	UPROPERTY(EditAnywhere)
	TArray<AChunkActor*> SpawnedActors;

	UFUNCTION(BlueprintCallable)
	void ScatterObjects();

	UFUNCTION(BlueprintCallable)
	void Spawn();

	UFUNCTION(BlueprintCallable)
	void DeSpawn();

	UFUNCTION(BlueprintCallable)
	void ReSpawn();
};
