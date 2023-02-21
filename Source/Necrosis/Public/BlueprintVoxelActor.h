// Copyright 2022 Andras Ketzer, All rights reserved.

#pragma once

#include <optional>

#include "CoreMinimal.h"
#include "VoxelActor.h"
#include "VoxelExecNode.h"
#include "VoxelNode.h"
#include "Nodes/VoxelFoliageNodes.h"
#include "BlueprintVoxelActor.generated.h"

typedef UObject BlueprintType;

USTRUCT(DisplayName= "Blueprint Class")
struct NECROSIS_API FVoxelBlueprintClassData 
{
	GENERATED_BODY()

	TSharedPtr<BlueprintType> BlueprintClassData;
};

USTRUCT()
struct NECROSIS_API FVoxelBlueprintClassDataPinType : public FVoxelExposedPinType
{
	GENERATED_BODY()

	DEFINE_VOXEL_EXPOSED_PIN_TYPE(FVoxelBlueprintClassData, TSoftObjectPtr<BlueprintType>)
	{
		const auto SharedActor = MakeShared<FVoxelBlueprintClassData>();
		return SharedActor;
	}
};

USTRUCT(Category = "RDPolarity")
struct NECROSIS_API FVoxelChunkExecNode_CreateBlueprintSpawnerComponent : public FVoxelChunkExecNode_Default
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelFoliageChunkData, ChunkData, nullptr);
	VOXEL_INPUT_PIN(FColor, DebugColour, FColor::White);
	VOXEL_INPUT_PIN(FVoxelBlueprintClassData, ActorClass, nullptr);
	
	virtual TValue<FVoxelChunkExecObject> Execute(const FVoxelQuery& Query) const override;
};

USTRUCT()
struct FVoxelChunkExecObject_CreateBlueprintSpawnerComponent : public FVoxelChunkExecObject
{
protected:
	virtual void Create(FVoxelRuntime& Runtime) const override;
	virtual void Destroy(FVoxelRuntime& Runtime) const override;

public:
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()
	TArray<FVector> SpawnPositions = TArray<FVector>();
	// std::optional<UBlueprint> ActorClass;
};
	
UCLASS()
class NECROSIS_API ABlueprintVoxelActor : public AVoxelActor
{
	ABlueprintVoxelActor();

public:
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

private:
	GENERATED_BODY()

	UFUNCTION(CallInEditor, Category = "Voxel")
	void TriggerSomething();
};
