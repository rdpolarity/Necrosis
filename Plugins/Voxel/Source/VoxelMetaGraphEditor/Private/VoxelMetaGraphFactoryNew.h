// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Factories/Factory.h"
#include "VoxelMetaGraphFactoryNew.generated.h"

class UVoxelMetaGraph;

UCLASS(hidecategories = Object)
class UVoxelMetaGraphFactoryNew : public UFactory
{
	GENERATED_BODY()

	UVoxelMetaGraphFactoryNew(const FObjectInitializer& ObjectInitializer);
	
	UPROPERTY(Transient)
	TObjectPtr<UVoxelMetaGraph> GraphToCopy;

	//~ Begin UFactory Interface
	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	//~ Begin UFactory Interface	
};