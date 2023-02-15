// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelHeightSplitterNode.generated.h"

USTRUCT(Category = "Math|Misc")
struct VOXELMETAGRAPH_API FVoxelNode_HeightSplitter : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	FVoxelNode_HeightSplitter();

	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Height, nullptr);

	virtual TVoxelFunction<FVoxelFutureValue(const FVoxelQuery&)> Compile(FName PinName) const override;

	virtual void PostSerialize() override;

#if WITH_EDITOR
	virtual void GetExternalPinsData(TArray<FName>& OutPinNames, TArray<FName>& OutCategoryNames) const override;
#endif

public:
	struct FLayerPin
	{
		TVoxelPinRef<float> Height;
		TVoxelPinRef<float> Falloff;
	};
	TArray<FLayerPin> LayerPins;
	TArray<TVoxelPinRef<FVoxelFloatBuffer>> ResultPins;

	UPROPERTY()
	int32 NumLayerPins = 1;

	void FixupLayerPins();

public:
	class FDefinition : public Super::FDefinition
	{
	public:
		GENERATED_VOXEL_NODE_DEFINITION_BODY(FVoxelNode_HeightSplitter);

		virtual FString GetAddPinLabel() const override;
		virtual FString GetAddPinTooltip() const override;

		virtual bool CanAddInputPin() const override { return true; }
		virtual void AddInputPin() override;

		virtual bool CanRemoveInputPin() const override;
		virtual void RemoveInputPin() override;
	};
};