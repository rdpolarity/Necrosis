// Copyright 2022 Andras Ketzer, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ChunkActor.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class NECROSIS_API AChunkActor : public AActor
{
	GENERATED_BODY()
public:
	AChunkActor();
	
	UPROPERTY(EditAnywhere)
	class UChunk* Chunk;

	UPROPERTY(EditAnywhere)
	FVector SpawnPosition;

	UFUNCTION(BlueprintNativeEvent ,BlueprintCallable)
	void Spawn();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void DeSpawn();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void ReSpawn();
};
