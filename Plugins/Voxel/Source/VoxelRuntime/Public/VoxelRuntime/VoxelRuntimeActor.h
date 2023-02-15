// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelRuntimeActor.generated.h"

class FVoxelRuntime;
class IVoxelMetaGraphRuntime;

UCLASS(Abstract)
class VOXELRUNTIME_API AVoxelRuntimeActor : public AActor
{
	GENERATED_BODY()

public:
	bool bCreateRuntimeOnBeginPlay = true;

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	// In the editor, Destroyed is called but EndPlay isn't
	virtual void Destroyed() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostEditImport() override;

#if WITH_EDITOR
	virtual void PreEditChange(FProperty* PropertyThatWillChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool Modify(bool bAlwaysMarkDirty = true) override;
	virtual void PostEditMove(bool bFinished) override;
#endif
	//~ End AActor Interface

	virtual TSharedRef<IVoxelMetaGraphRuntime> MakeMetaGraphRuntime(FVoxelRuntime& Runtime) const;

	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	
public:
	TSharedPtr<FVoxelRuntime> GetRuntime() const
	{
		return PrivateRuntime;
	}

	bool CreateRuntime();
	void DestroyRuntime();

private:
	TSharedPtr<FVoxelRuntime> PrivateRuntime;
};