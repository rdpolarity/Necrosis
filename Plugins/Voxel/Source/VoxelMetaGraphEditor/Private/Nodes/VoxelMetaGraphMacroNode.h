// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMetaGraphNode.h"
#include "VoxelNodeDefinition.h"
#include "VoxelMetaGraphMacroNode.generated.h"

class UVoxelMetaGraph;

UCLASS()
class UVoxelMetaGraphMacroNode : public UVoxelMetaGraphNode
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UVoxelMetaGraph> MetaGraph;

	TSet<FName> CachedDynamicPins;

	//~ Begin UVoxelGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual bool ShowPaletteIconOnNode() const override { return true; }

	virtual bool ShouldHidePinDefaultValue(const UEdGraphPin& Pin) const override;

	virtual bool CanJumpToDefinition() const override { return true; }
	virtual void JumpToDefinition() const override;

	virtual FName GetPinCategory(const UEdGraphPin& Pin) const override;

	virtual TSharedRef<IVoxelNodeDefinition> GetNodeDefinition() override;

	virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const override;
	//~ End UVoxelGraphNode Interface
};

class FVoxelMetaGraphMacroNodeDefinition : public IVoxelNodeDefinition
{
public:
	explicit FVoxelMetaGraphMacroNodeDefinition(UVoxelMetaGraphMacroNode& Node);

	TObjectPtr<UVoxelMetaGraph> GetMetaGraph() const;

	virtual TSharedPtr<const FNode> GetInputs() const override;
	virtual TSharedPtr<const FNode> GetOutputs() const override;
	TSharedPtr<const FNode> GetPins(const bool bInput) const;
	virtual bool OverridePinsOrder() const override
	{
		return true;
	}

private:
	UVoxelMetaGraphMacroNode& Node;
};