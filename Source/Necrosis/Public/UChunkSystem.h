// Copyright 2022 Andras Ketzer, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Chunk.h"
#include "GameFramework/Actor.h"
#include "UChunkSystem.generated.h"


UCLASS()
class NECROSIS_API AUChunkSystem : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AUChunkSystem();

	UPROPERTY(EditAnywhere)
	float ChunkSize = 1000;

	UPROPERTY(EditAnywhere)
	float ChunkSpawnRadius = 5;

	UPROPERTY(EditAnywhere)
	float ChunkDespawnRadius = 6;

	UPROPERTY(EditAnywhere)
	bool bDebugRenderChunks = false;

	UPROPERTY(EditAnywhere)
	FVector PlayerChunkPosition = {0, 0, 0};

	UPROPERTY(EditAnywhere)
	FVector PlayerLastEnteredChunkPosition = {0, 0, 0};

	UPROPERTY(EditAnywhere)
	TMap<FVector, UChunk*> SpawnedChunks;
	
	UFUNCTION(CallInEditor)
	FVector RoundVector(FVector VectorToRound) const;

	UFUNCTION()
	void OnChangeChunk(FVector NewChunkPosition);

	UFUNCTION()
	void SpawnChunksAround(FVector Point);

	UFUNCTION()
	void DespawnChunksAround(FVector Point);

	UFUNCTION()
	void DebugRenderChunks();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
