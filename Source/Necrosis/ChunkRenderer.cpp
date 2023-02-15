// Copyright 2022 Andras Ketzer, All rights reserved.


#include "ChunkRenderer.h"

#include "AITestsCommon.h"

// Sets default values for this component's properties
UChunkRenderer::UChunkRenderer()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

FVector CalculateChunkCentre(FVector WorldCoordinate, int32 ChunkSize)
{
	double X = FMath::RoundToDouble(WorldCoordinate.X / ChunkSize);
	double Y = FMath::RoundToDouble(WorldCoordinate.Y / ChunkSize);
	double Z = FMath::RoundToDouble(WorldCoordinate.Z / ChunkSize);

	return {X, Y, Z};
}

void PlaceActorsInChunks(UWorld* World, FVector From, int32 ChunkSize, int32 Radius, TSubclassOf<AActor> ActorClass)
{
	for (int32 X = -Radius; X <= Radius; ++X)
	{
		for (int32 Y = -Radius; Y <= Radius; ++Y)
		{
			FVector ChunkCenter = FVector(X * ChunkSize, Y * ChunkSize, 0);
			if (ChunkCenter.Size() <= Radius * ChunkSize)
			{
				// Find the chunk index for From
				FVector PlayerChunkIndex = CalculateChunkCentre(From, ChunkSize);
				// Find the chunk index for the current chunk
				AActor* Actor = World->SpawnActor<AActor>(ActorClass, ChunkCenter, FRotator::ZeroRotator);
			}
		}
	}
}

// Called when the game starts
void UChunkRenderer::BeginPlay()
{
	UE_LOG(LogTemp, Warning, TEXT("Chunk Renderer Begin Play"));
	Super::BeginPlay();
	PlaceActorsInChunks(GetWorld(), GetOwner()->GetActorLocation(), ChunkSize, ChunkRadius, ChunkActor);
}


// Called every frame
void UChunkRenderer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}
