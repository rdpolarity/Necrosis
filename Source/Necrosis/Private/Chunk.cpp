// Copyright 2022 Andras Ketzer, All rights reserved.


#include "Chunk.h"

#include "UChunkSystem.h"

void UChunk::ScatterObjects()
{
	for (TSubclassOf<UScatterTemplate> ScatterTemplate : ChunkSystem->ScatterTemplates)
	{
		// log to the screen the current scatter template
		float SpawnChance = ScatterTemplate.GetDefaultObject()->SpawnChance;
		FColor DebugColor = ScatterTemplate.GetDefaultObject()->DebugColor;
		TSubclassOf<AChunkActor> ActorToSpawn = ScatterTemplate.GetDefaultObject()->ActorToSpawn;

		// 20% change to spawn a scatter object
		if (FMath::RandRange(0, 100) > SpawnChance) return;
		// Get corners of Chunk
		FVector min = ChunkPosition - FVector(ChunkSystem->ChunkSize / 2);
		FVector max = ChunkPosition + FVector(ChunkSystem->ChunkSize / 2);
		min.Z = ChunkPosition.Z;
		max.Z = ChunkPosition.Z;

		// Get a random point inbetween min and max
		FVector RandomPoint = FMath::RandPointInBox(FBox(min, max));
	
		// From random point raycast down to get the ground position
		FHitResult HitResult;
		FCollisionQueryParams CollisionParams;
		CollisionParams.AddIgnoredActor(ChunkSystem);
		ChunkSystem->GetWorld()->LineTraceSingleByChannel(HitResult, RandomPoint, RandomPoint - FVector(0, 0, 10000), ECC_Visibility,
		                                     CollisionParams);

		// Log "Failed to find ground position" if HitResult is not valid
		if (!HitResult.IsValidBlockingHit())
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to find ground position"));
			return;
		} else
		{
			UE_LOG(LogTemp, Warning, TEXT("Found ground position"));
		}
		
		DrawDebugPoint(ChunkSystem->GetWorld(), HitResult.Location, 10, DebugColor, true, 2, 0);


		
		// AChunkActor* SpawnedActor = GetWorld()->SpawnActor<AChunkActor>(ActorToSpawn, HitResult.Location,
		                                                                // FRotator::ZeroRotator);
		// SpawnedActor->Spawn();
	}
}

void UChunk::Spawn()
{
	ChunkState = EChunkState::SPAWNED;
	ScatterObjects();
}

void UChunk::DeSpawn()
{
	ChunkState = EChunkState::DE_SPAWNED;
	// for (AChunkActor* SpawnedActor : SpawnedActors) SpawnedActor->DeSpawn();
}

void UChunk::ReSpawn()
{
	ChunkState = EChunkState::SPAWNED;
	// for (AChunkActor* SpawnedActor : SpawnedActors) SpawnedActor->ReSpawn();
}
