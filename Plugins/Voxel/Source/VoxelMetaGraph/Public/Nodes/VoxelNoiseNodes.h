// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelNoiseNodes.generated.h"

USTRUCT(Category = "Math|Seed", meta = (CompactNodeTitle = "MIX"))
struct VOXELMETAGRAPH_API FVoxelNode_MixSeeds : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, A, nullptr, SeedPin);
	VOXEL_MATH_INPUT_PIN(int32, B, nullptr, SeedPin);
	VOXEL_MATH_OUTPUT_PIN(int32, ReturnValue, SeedPin);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{ReturnValue} = MurmurHash32(MurmurHash32({A}) ^ {B})";
	}
};

USTRUCT(Category = "Math|Seed")
struct VOXELMETAGRAPH_API FVoxelNode_MakeSeeds : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(int32, Seed, nullptr, SeedPin);

	FVoxelNode_MakeSeeds()
	{
		FixupSeedPins();
	}

	virtual TVoxelFunction<FVoxelFutureValue(const FVoxelQuery&)> Compile(FName PinName) const override;

	virtual void PostSerialize() override
	{
		FixupSeedPins();

		Super::PostSerialize();

		FixupSeedPins();
	}

#if WITH_EDITOR
	virtual void GetExternalPinsData(TArray<FName>& OutPinNames, TArray<FName>& OutCategoryNames) const override
	{
		OutPinNames.Add("Seed");
		OutPinNames.Add("ResultSeed");
	}
#endif

public:
	TArray<FVoxelPinRef> ResultPins;

	UPROPERTY()
	int32 NumNewSeeds = 1;

	void FixupSeedPins();

	class FDefinition : public Super::FDefinition
	{
	public:
		GENERATED_VOXEL_NODE_DEFINITION_BODY(FVoxelNode_MakeSeeds);

		virtual bool CanAddInputPin() const override
		{
			return true;
		}
		virtual void AddInputPin() override
		{
			Node.NumNewSeeds++;
			Node.FixupSeedPins();
		}

		virtual bool CanRemoveInputPin() const override
		{
			return Node.NumNewSeeds > 1;
		}
		virtual void RemoveInputPin() override
		{
			Node.NumNewSeeds--;
			Node.FixupSeedPins();
		}
	};
};

USTRUCT(Category = "Noise")
struct VOXELMETAGRAPH_API FVoxelNode_PerlinNoise2D : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector2D, Position, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, Seed, nullptr, SeedPin);
	VOXEL_MATH_OUTPUT_PIN(float, Value);

	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{Value} = GetPerlin2D({Seed}, {Position})";
	}
};

USTRUCT(Category = "Noise")
struct VOXELMETAGRAPH_API FVoxelNode_PerlinNoise3D : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector, Position, nullptr);
	VOXEL_MATH_INPUT_PIN(int32, Seed, nullptr, SeedPin);
	VOXEL_MATH_OUTPUT_PIN(float, Value);
	
	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{Value} = GetPerlin3D({Seed}, {Position})";
	}
};

USTRUCT(Category = "Noise")
struct VOXELMETAGRAPH_API FVoxelNode_CellularNoise2D : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector2D, Position, nullptr);
	VOXEL_MATH_INPUT_PIN(float, Jitter, 0.9f);
	VOXEL_MATH_INPUT_PIN(int32, Seed, nullptr, SeedPin);
	VOXEL_MATH_OUTPUT_PIN(float, Value);
	
	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{Value} = GetCellularNoise2D({Seed}, {Position}, {Jitter})";
	}
};

USTRUCT(Category = "Noise")
struct VOXELMETAGRAPH_API FVoxelNode_CellularNoise3D : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector, Position, nullptr);
	VOXEL_MATH_INPUT_PIN(float, Jitter, 0.9f);
	VOXEL_MATH_INPUT_PIN(int32, Seed, nullptr, SeedPin);
	VOXEL_MATH_OUTPUT_PIN(float, Value);
	
	virtual FString GenerateCode(bool bIsGpu) const override
	{
		return "{Value} = GetCellularNoise3D({Seed}, {Position}, {Jitter})";
	}
};

USTRUCT(Category = "Noise")
struct VOXELMETAGRAPH_API FVoxelNode_TrueDistanceCellularNoise2D : public FVoxelNode_CodeGen
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_MATH_INPUT_PIN(FVector2D, Position, nullptr);
	VOXEL_MATH_INPUT_PIN(float, Jitter, 0.9f);
	VOXEL_MATH_INPUT_PIN(int32, Seed, nullptr, SeedPin);
	VOXEL_MATH_OUTPUT_PIN(float, Value);
	VOXEL_MATH_OUTPUT_PIN(FVector2D, CellPosition);
	
	virtual FString GenerateCode(bool bIsGpu) const override
	{
		if (bIsGpu)
		{
			return "{Value} = GetTrueDistanceCellularNoise2D({Seed}, {Position}, {Jitter}, {CellPosition})";
		}
		else
		{
			return "{Value} = GetTrueDistanceCellularNoise2D({Seed}, {Position}, {Jitter}, &{CellPosition})";
		}
	}
};