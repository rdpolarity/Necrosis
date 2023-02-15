// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Nodes/Templates/VoxelTemplateNode.h"
#include "VoxelFilterBufferNodes.generated.h"

USTRUCT(meta = (Internal))
struct VOXELMETAGRAPH_API FVoxelNode_FilterBuffer : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_GENERIC_INPUT_PIN(Value);
	VOXEL_INPUT_PIN(FVoxelBoolBuffer, Condition, nullptr);
	VOXEL_GENERIC_OUTPUT_PIN(OutValue);

	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;
};

USTRUCT(Category = "Misc")
struct VOXELMETAGRAPH_API FVoxelTemplateNode_FilterBuffer : public FVoxelTemplateNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelBoolBuffer, Condition, nullptr, DisplayLast);

public:
	FVoxelTemplateNode_FilterBuffer();

	virtual void ExpandPins(FNode& Node, TArray<FPin*> Pins, const TArray<FPin*>& AllPins, TArray<FPin*>& OutPins) const override;

	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;

	virtual void PostSerialize() override;

	virtual bool IsPureNode() const override
	{
		return true;
	}

#if WITH_EDITOR
	virtual void GetExternalPinsData(TArray<FName>& OutPinNames, TArray<FName>& OutCategoryNames) const override;
#endif

public:
	TArray<FVoxelPinRef> BufferInputPins;
	TArray<FVoxelPinRef> BufferOutputPins;

	UPROPERTY()
	int32 NumBufferPins = 1;

	void FixupBufferPins();

public:
	class FDefinition : public Super::FDefinition
	{
	public:
		GENERATED_VOXEL_NODE_DEFINITION_BODY(FVoxelTemplateNode_FilterBuffer);

		virtual FString GetAddPinLabel() const override;
		virtual FString GetAddPinTooltip() const override;

		virtual bool CanAddInputPin() const override { return true; }
		virtual void AddInputPin() override;

		virtual bool CanRemoveInputPin() const override;
		virtual void RemoveInputPin() override;
	};
};