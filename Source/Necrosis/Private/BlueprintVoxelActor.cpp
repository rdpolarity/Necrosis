// Copyright 2022 Andras Ketzer, All rights reserved.


#include "BlueprintVoxelActor.h"

#include "ActorFolder.h"
#include "AITestsCommon.h"
#include "VoxelChunkManager.h"
#include "VoxelTask.h"
#include "VoxelFoliage/Public/Nodes/VoxelFoliageNodes.h"
#include "VoxelRuntime/VoxelRuntime.h"

TVoxelFutureValue<FVoxelChunkExecObject> FVoxelChunkExecNode_CreateBlueprintSpawnerComponent::Execute(
	const FVoxelQuery& Query) const
{
	const TValue<FVoxelFoliageChunkData> ChunkData = GetNodeRuntime().Get(ChunkDataPin, Query);
	const auto ActorClass = GetNodeRuntime().Get(ActorClassPin, Query);
	const auto DebugColour = GetNodeRuntime().Get(DebugColourPin, Query);

	return VOXEL_ON_COMPLETE(GameThread, ChunkData, ActorClass)
	{
		const TSharedRef<FVoxelChunkExecObject_CreateBlueprintSpawnerComponent> ChunkExecObject =
			MakeShared<FVoxelChunkExecObject_CreateBlueprintSpawnerComponent>();

		// Do nothing if there's no data
		if (ChunkData->Data.Num() == 0 ||
			ChunkData->InstancesCount == 0)
		{
			return {};
		}
		// clear all debug lines
		// Spawn actor under folder
		int32 InstanceIndex = 0;
		for (int32 MeshIndex = 0; MeshIndex < ChunkData->Data.Num(); MeshIndex++)
		{
			const TSharedPtr<FVoxelFoliageChunkMeshData>& MeshData = ChunkData->Data[MeshIndex];
			for (int32 MeshInstanceIndex = 0; MeshInstanceIndex < MeshData->Transforms->Num(); MeshInstanceIndex++)
			{
				const auto InstancePosition = (*MeshData->Transforms)[MeshInstanceIndex].GetLocation() +
					FVector3f(ChunkData->ChunkPosition);
				const auto InstanceRotation = (*MeshData->Transforms)[MeshInstanceIndex].GetRotation().Rotator();
				const auto InstanceClass = ActorClass.BlueprintClassData.Get();

				// Convert to unreal values
				const FVector InstanceVectorPosition = FVector(InstancePosition.X, InstancePosition.Y,
				                                               InstancePosition.Z);
				const FRotator InstanceRotator = {InstanceRotation.Pitch, InstanceRotation.Yaw, InstanceRotation.Roll};
				const FBlueprintSpawnData SpawnData = {InstanceVectorPosition, InstanceRotator, InstanceClass};
				ChunkExecObject->SpawnData.Add(SpawnData);

				InstanceIndex++;
			}
		}

		return ChunkExecObject;
	};
}

void FVoxelChunkExecObject_CreateBlueprintSpawnerComponent::Create(FVoxelRuntime& Runtime) const
{
	FVoxelChunkExecObject::Create(Runtime);
	// draw debug point
	// For each SpawnPosition draw debug point
	for (FBlueprintSpawnData Data : SpawnData)
	{
		if (GEngine->IsEditor() && !Runtime.GetWorld()->IsPlayInEditor())
		{
			// DrawDebugPoint(Runtime.GetWorld(), SpawnPosition, 10.f, FColor::Red, true, 0.1f);
		}
		else
		{
			if (Data.ActorClass == nullptr) return;
			auto GeneratedClass = Data.ActorClass->GeneratedClass;

			if (GeneratedClass->IsChildOf(AActor::StaticClass()))
			{
				TSubclassOf<AActor> ActorStaticClass = TSubclassOf<AActor>(GeneratedClass);
				AActor* Actor = Runtime.GetWorld()->SpawnActor<AActor>(ActorStaticClass, Data.SpawnPosition,
				                                                       Data.SpawnRotation);
			}
		}
	}
}

void FVoxelChunkExecObject_CreateBlueprintSpawnerComponent::Destroy(FVoxelRuntime& Runtime) const
{
	FVoxelChunkExecObject::Destroy(Runtime);
	// For each SpawnPosition draw debug point
}

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

void ABlueprintVoxelActor::BeginPlay()
{
	Super::BeginPlay();
	// Log hello to the screen
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Hello"));
}

void ABlueprintVoxelActor::TriggerSomething()
{
	// Use CoreDelegates to run an event when a task is finished
	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("taskNumber: %d"), taskNumber));
}

TSharedRef<IVoxelMetaGraphRuntime> ABlueprintVoxelActor::MakeMetaGraphRuntime(FVoxelRuntime& Runtime) const
{
	return Super::MakeMetaGraphRuntime(Runtime);
	FlushPersistentDebugLines(Runtime.GetWorld());
}
