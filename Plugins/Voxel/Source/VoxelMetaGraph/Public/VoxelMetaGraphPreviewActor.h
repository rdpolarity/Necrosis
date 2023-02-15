// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMetaGraphPreviewActor.generated.h"

UCLASS()
class VOXELMETAGRAPH_API AVoxelMetaGraphPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Voxel")
	EVoxelAxis Axis = EVoxelAxis::Z;

	UPROPERTY()
	TObjectPtr<UBoxComponent> BoxComponent;

	AVoxelMetaGraphPreviewActor();

public:
	FSimpleMulticastDelegate OnChanged;

#if WITH_EDITOR
	//~ Begin AActor Interface
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
	virtual void PostEditMove(bool bFinished) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End AActor Interface

	float GetAxisLocation() const;
	void SetAxisLocation(float NewAxisValue);
#endif
};