// Copyright 2022 Andras Ketzer, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ChunkRenderer.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class NECROSIS_API UChunkRenderer : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UChunkRenderer();
	// Chunk Size Property
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunk")
	int32 ChunkSize = 1000;
	// Chunk Radius Property
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunk")
	int32 ChunkRadius = 10;
	// Chunk Actor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunk")
	TSubclassOf<AActor> ChunkActor;
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
