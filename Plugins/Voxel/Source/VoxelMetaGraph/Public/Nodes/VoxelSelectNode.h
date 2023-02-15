// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelSelectNode.generated.h"

USTRUCT(Category = "Flow Control")
struct VOXELMETAGRAPH_API FVoxelNode_Select : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY_IMPL(FVoxelNode)

public:
	VOXEL_GENERIC_INPUT_PIN(Index, DisplayLast);
	VOXEL_GENERIC_OUTPUT_PIN(Result);

	FVoxelNode_Select();

	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;

	virtual void PreSerialize() override;
	virtual void PostSerialize() override;

#if WITH_EDITOR
	virtual void GetExternalPinsData(TArray<FName>& OutPinNames, TArray<FName>& OutCategoryNames) const override;
#endif

public:
	TArray<FVoxelPinRef> ValuePins;

	UPROPERTY()
	int32 NumIntegerOptions = 2;

	UPROPERTY()
	FVoxelPinType SerializedIndexType;

	void FixupValuePins();

public:
	class FDefinition : public Super::FDefinition
	{
	public:
		GENERATED_VOXEL_NODE_DEFINITION_BODY(FVoxelNode_Select);

		virtual FString GetAddPinLabel() const override;
		virtual FString GetAddPinTooltip() const override;
		virtual FString GetRemovePinTooltip() const override;

		virtual bool CanAddInputPin() const override;
		virtual void AddInputPin() override;

		virtual bool CanRemoveInputPin() const override;
		virtual void RemoveInputPin() override;
	};
};