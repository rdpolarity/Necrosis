// Copyright 2022 Andras Ketzer, All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoxelActor.h"
#include "BlueprintVoxelActor.generated.h"

/**
 * 
 */
UCLASS()
class NECROSIS_API ABlueprintVoxelActor : public AVoxelActor
{
	ABlueprintVoxelActor();

public:
	virtual void Tick(float DeltaTime) override;

private:
	GENERATED_BODY()

	UFUNCTION(CallInEditor, Category = "Voxel")
	void TriggerSomething();
};
