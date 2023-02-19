// Copyright 2022 Andras Ketzer, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "ChunkActor.h"
#include "UObject/NoExportTypes.h"
#include "ScatterTemplate.generated.h"

UCLASS(Blueprintable)
class NECROSIS_API UScatterTemplate : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	float SpawnChance = 0.0f;
	UPROPERTY(EditAnywhere)
	FColor DebugColor = FColor::Blue;
	UPROPERTY(EditAnywhere)
	TSubclassOf<AChunkActor> ActorToSpawn;
};
