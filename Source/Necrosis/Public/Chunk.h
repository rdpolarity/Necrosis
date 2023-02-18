// Copyright 2022 Andras Ketzer, All rights reserved.

#pragma once

#include "CoreMinimal.h"
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
};
