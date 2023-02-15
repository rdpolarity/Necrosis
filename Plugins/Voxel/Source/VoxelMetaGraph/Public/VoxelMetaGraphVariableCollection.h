// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelPinValue.h"
#include "VoxelMetaGraph.h"
#include "VoxelMetaGraphVariableCollection.generated.h"

class FVoxelRuntime;

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelMetaGraphVariable
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	FName Name;

	UPROPERTY(EditAnywhere, Category = "Config")
	bool bIsOrphan = false;
	
	UPROPERTY(EditAnywhere, Category = "Config")
	FVoxelPinValue Value;
	
	UPROPERTY(EditAnywhere, Category = "Config")
	FVoxelPinValue DefaultValue;

	UPROPERTY(EditAnywhere, Category = "Config")
	FString Category;

	UPROPERTY(EditAnywhere, Category = "Config")
	FString Description;

	UPROPERTY(EditAnywhere, Category = "Config")
	int32 SortIndex = -1;

public:
	const FVoxelPinType& GetType() const
	{
		ensure(Value.GetType() == DefaultValue.GetType());
		return Value.GetType();
	}
	bool IsDefault() const
	{
		ensure(Value.GetType() == DefaultValue.GetType());
		return Value == DefaultValue;
	}
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelMetaGraphVariableCollection
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Config")
	TMap<FGuid, FVoxelMetaGraphVariable> Variables;

	UPROPERTY(EditAnywhere, Category = "Config")
	TArray<FString> Categories;

	FSimpleMulticastDelegate RefreshDetails;

	void Fixup(const TArray<FVoxelMetaGraphParameter>& Parameters);
	void CheckOrphans();
};