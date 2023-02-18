// Copyright 2022 Andras Ketzer, All rights reserved.

#pragma once

#include "CoreMinimal.h"
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
	int32 ChunkSize = 1000;

	UPROPERTY(EditAnywhere)
	int32 ChunkSpawnRadius = 3;

	// vector position
	UPROPERTY(EditAnywhere)
	FVector PlayerPosition = {0, 0, 0};

	UPROPERTY(EditAnywhere)
	FVector PlayerLastEnteredChunkPosition = {0, 0, 0};

	UFUNCTION(CallInEditor)
	void CheckIfPlayerHasChangedChunk();

	UFUNCTION(CallInEditor)
	void RoundVector(FVector& VectorToRound);

	UFUNCTION()
	void OnChangeChunk();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
