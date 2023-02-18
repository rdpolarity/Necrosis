// Copyright 2022 Andras Ketzer, All rights reserved.


#include "UChunkSystem.h"

// Sets default values
AUChunkSystem::AUChunkSystem()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

FVector AUChunkSystem::RoundVector(FVector VectorToRound) const
{
	VectorToRound.X = FMath::RoundToInt(VectorToRound.X);
	VectorToRound.Y = FMath::RoundToInt(VectorToRound.Y);
	VectorToRound.Z = FMath::RoundToInt(VectorToRound.Z);

	return VectorToRound;
}

void AUChunkSystem::OnChangeChunk(FVector NewChunkPosition)
{
	// screen debug log
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Player has entered a new chunk!")));
	SpawnChunksAround(NewChunkPosition);
	DespawnChunksAround(NewChunkPosition);
}

void AUChunkSystem::SpawnChunksAround(FVector Point)
{
	for (double x = ChunkSpawnRadius * -1; x < ChunkSpawnRadius; ++x)
	{
		for (double y = ChunkSpawnRadius * -1; y < ChunkSpawnRadius; ++y)
		{
			FVector ChunkIndex = {x, y, 0};
			FVector ChunkPositionToSpawn = RoundVector(ChunkIndex * ChunkSize) + Point;
			// Is it within a circular radius of the point?
			if (FVector::Dist(ChunkPositionToSpawn, Point) > ChunkSpawnRadius * ChunkSize) continue;
			// Is there already a chunk at this position?
			if (SpawnedChunks.Contains(ChunkPositionToSpawn))
			{
				if (SpawnedChunks[ChunkPositionToSpawn]->ChunkState == EChunkState::DE_SPAWNED)
				{
					SpawnedChunks[ChunkPositionToSpawn]->ChunkState = EChunkState::SPAWNED;
				} else continue;
			}
			UChunk* NewChunk = NewObject<UChunk>();
			SpawnedChunks.Add(ChunkPositionToSpawn, NewChunk);
		}
	}
}

void AUChunkSystem::DespawnChunksAround(FVector Point)
{
	for (const auto& Chunk : SpawnedChunks)
	{
		FVector ChunkPosition = Chunk.Key;
		UChunk* ChunkObject = Chunk.Value;
		// Is it within a circular radius of the point?
		if (FVector::Dist(ChunkPosition, Point) > ChunkDespawnRadius * ChunkSize)
		{
			ChunkObject->ChunkState = EChunkState::DE_SPAWNED;
		}
	}
}


void AUChunkSystem::DebugRenderChunks()
{
	for (const auto& Chunk : SpawnedChunks)
	{
		FVector ChunkPosition = Chunk.Key;
		const UChunk* ChunkObject = Chunk.Value;
		FColor ChunkColor = FColor::Black;
		if (ChunkObject->ChunkState == EChunkState::DE_SPAWNED) ChunkColor = FColor::Red;
		DrawDebugPoint(GetWorld(), ChunkPosition, 10, ChunkColor, false, -1, 0);
	}
	DrawDebugBox(GetWorld(), PlayerChunkPosition, FVector(ChunkSize / 2), FColor::Green, false, -1, 0, 5);
	DrawDebugCircle(GetWorld(), PlayerChunkPosition, ChunkDespawnRadius * ChunkSize, ChunkSpawnRadius * 100,
	                FColor::Red, false, -1, 0, 5,
	                FVector(0, 1, 0), FVector(1, 0, 0), false);
}

// Called when the game starts or when spawned
void AUChunkSystem::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AUChunkSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const FVector ActorLocation = GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation();
	PlayerChunkPosition = RoundVector(ActorLocation / ChunkSize) * ChunkSize;
	PlayerChunkPosition.Z = GetActorLocation().Z;

	if (PlayerChunkPosition != PlayerLastEnteredChunkPosition)
	{
		PlayerLastEnteredChunkPosition = PlayerChunkPosition;
		OnChangeChunk(PlayerChunkPosition);
	}

	if (bDebugRenderChunks) DebugRenderChunks();
}
