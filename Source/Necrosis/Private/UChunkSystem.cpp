// Copyright 2022 Andras Ketzer, All rights reserved.


#include "UChunkSystem.h"

// Sets default values
AUChunkSystem::AUChunkSystem()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

void AUChunkSystem::RoundVector(FVector& VectorToRound)
{
	VectorToRound.X = FMath::Floor(VectorToRound.X);
	VectorToRound.Y = FMath::Floor(VectorToRound.Y);
	VectorToRound.Z = FMath::Floor(VectorToRound.Z);
}

void AUChunkSystem::OnChangeChunk()
{
	// screen debug log
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Player has entered a new chunk!")));
}

void AUChunkSystem::CheckIfPlayerHasChangedChunk()
{
	FVector RoundedPlayerPosition = PlayerPosition / ChunkSize;
	RoundVector(RoundedPlayerPosition);
	RoundedPlayerPosition *= ChunkSize;
	
	if (RoundedPlayerPosition != PlayerPosition)
	{
		PlayerLastEnteredChunkPosition = RoundedPlayerPosition;
		OnChangeChunk();
	}
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
	PlayerPosition = GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation();
	CheckIfPlayerHasChangedChunk();
}

