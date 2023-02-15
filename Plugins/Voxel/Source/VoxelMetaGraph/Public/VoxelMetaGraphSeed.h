// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelExposedPinType.h"
#include "VoxelMetaGraphSeed.generated.h"

USTRUCT(BlueprintType, meta = (TypeCategory = "Default"))
struct VOXELMETAGRAPH_API FVoxelMetaGraphSeed
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Config")
	FString Seed;

	void Randomize()
	{
		const FRandomStream Stream(FMath::Rand());

		Seed.Reset(8);
		for (int32 Index = 0; Index < 8; Index++)
		{
			Seed += TCHAR(Stream.RandRange(TEXT('A'), TEXT('Z')));
		}
	}
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelMetaGraphSeedPinType : public FVoxelExposedPinType
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	virtual FVoxelPinType GetComputedType() const override
	{
		return FVoxelPinType::Make<int32>().WithTag("Seed");
	}
	virtual FVoxelPinType GetExposedType() const override
	{
		return FVoxelPinType::Make<FVoxelMetaGraphSeed>();
	}
	virtual void ComputeImpl(FVoxelPinValue& OutValue, const FVoxelPinValue& Value) const override
	{
		OutValue.Get<int32>() = FCrc::StrCrc32(*Value.Get<FVoxelMetaGraphSeed>().Seed);
	}
};