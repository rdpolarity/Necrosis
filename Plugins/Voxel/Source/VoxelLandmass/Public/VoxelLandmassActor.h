// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLandmassBrush.h"
#include "VoxelLandmassActor.generated.h"

DECLARE_UNIQUE_VOXEL_ID(FVoxelLandmassBrushId);

UCLASS(meta = (VoxelPlaceableItem, PlaceableSubMenu = "Landmass"))
class VOXELLANDMASS_API AVoxelLandmassActor : public AVoxelBrushActor
{
	GENERATED_BODY()

public:
	AVoxelLandmassActor();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config", meta = (ShowOnlyInnerProperties))
	FVoxelLandmassBrush Brush;

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Runtime")
	bool bShowMeshesInGame = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Runtime")
	bool bIsSelected = false;

public:
	UFUNCTION(BlueprintCallable, Category = "Voxel|Landmass")
	void UpdateSelectionStatus(bool bForceVisible = false);

protected:
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

	UStaticMeshComponent* CreateMeshComponent(const UClass* Class);

	template<typename T>
	T* CreateMeshComponent()
	{
		return CastChecked<T>(this->CreateMeshComponent(T::StaticClass()));
	}

public:
	//~ Begin AActor Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End AActor Interface
	
public:
#if WITH_EDITOR
	static void OnSelectionChanged();
#endif
};