// Copyright 2022 Andras Ketzer, All rights reserved.


#include "BlueprintVoxelActor.h"

#include "VoxelRuntime/VoxelRuntime.h"

ABlueprintVoxelActor::ABlueprintVoxelActor() = default;

void ABlueprintVoxelActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	auto pos = GetRuntime();
	// print a debug point at pos
	// pos.Z = 100.f;
	// DrawDebugPoint(GetWorld(), pos, 10.f, FColor::Red, true, 0.1f);
	
	
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("pos: %s"), *pos.ToString()));
}
