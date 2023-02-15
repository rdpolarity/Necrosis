// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelRuntime/VoxelRuntimeActor.h"
#include "VoxelMetaGraphVariableCollection.h"
#include "VoxelActor.generated.h"

DECLARE_DYNAMIC_DELEGATE(FVoxelDynamicGraphEvent);

UCLASS()
class VOXELMETAGRAPH_API UVoxelActorRootComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
};

UCLASS(HideCategories = ("Rendering", "Replication", "Input", "Actor", "Collision", "LOD", "HLOD", "Cooking", "WorldPartition", "DataLayers"))
class VOXELMETAGRAPH_API AVoxelActor : public AVoxelRuntimeActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Voxel")
	TSoftObjectPtr<UVoxelMetaGraph> MetaGraph;

	UPROPERTY(EditAnywhere, Category = "Voxel")
	FVoxelMetaGraphVariableCollection VariableCollection;

	TMap<FName, TArray<FVoxelDynamicGraphEvent>> Events;

public:
	AVoxelActor();

	void Fixup();

	//~ Begin AVoxelRuntimeActor Interface
	virtual TSharedRef<IVoxelMetaGraphRuntime> MakeMetaGraphRuntime(FVoxelRuntime& Runtime) const override;
	//~ End AVoxelRuntimeActor Interface
	
	//~ Begin UObject Interface
	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
	virtual void PostLoad() override;
	virtual void PostEditImport() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void BindGraphEvent(FName Name, FVoxelDynamicGraphEvent Delegate);

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void QueueRefresh();

private:
#if WITH_EDITOR
	TSet<FObjectKey> UsedObjects;
#endif

	bool bRefreshQueued = false;
};