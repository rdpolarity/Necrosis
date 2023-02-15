// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelTransaction.h"
#include "VoxelMetaGraphNode.h"
#include "VoxelMetaGraphStructNode.generated.h"

UCLASS()
class UVoxelMetaGraphStructNode : public UVoxelMetaGraphNode
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Voxel", meta = (ShowOnlyInnerProperties))
#if CPP
	TVoxelInstancedStruct<FVoxelNode> Struct;
#else
	FVoxelInstancedStruct Struct;
#endif

	UPROPERTY()
	FString CachedName;

	//~ Begin UVoxelGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual void PostPasteNode() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual bool ShowPaletteIconOnNode() const override { return true; }
	virtual bool IsCompact() const override;
	virtual bool GetOverlayInfo(FString& Type, FString& Tooltip, FString& Color) override;

	virtual bool ShowAsPromotableWildcard(const UEdGraphPin& Pin) const override;
	virtual FName GetPinCategory(const UEdGraphPin& Pin) const override;

	virtual TSharedRef<IVoxelNodeDefinition> GetNodeDefinition() override;

	virtual bool CanRemovePin_ContextMenu(const UEdGraphPin& Pin) const override;
	virtual void RemovePin_ContextMenu(UEdGraphPin& Pin) override;

	virtual bool CanPromotePin(const UEdGraphPin& Pin, FVoxelPinTypeSet& OutTypes) const override;
	virtual void PromotePin(UEdGraphPin& Pin, const FVoxelPinType& NewType) override;

	virtual bool CanSplitPin(const UEdGraphPin& Pin) const override;
	virtual void SplitPin(UEdGraphPin& Pin) override;

	virtual bool TryMigratePin(UEdGraphPin* OldPin, UEdGraphPin* NewPin) const override;
	virtual bool TryMigrateDefaultValue(const UEdGraphPin* OldPin, UEdGraphPin* NewPin) const override;

	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;

	virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const override;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UVoxelGraphNode Interface

private:
	void ReconstructFromVoxelNode();
	void InitializeDefaultValue(const FVoxelPin& VoxelPin, UEdGraphPin& GraphPin);

	friend class FVoxelMetaGraphStructNodeDefinition;
};

class FVoxelMetaGraphStructNodeDefinition : public IVoxelNodeDefinition
{
	struct FScope
	{
		FVoxelMetaGraphStructNodeDefinition* Definition;
		TUniquePtr<FVoxelTransaction> Transaction;

		explicit FScope(FVoxelMetaGraphStructNodeDefinition* Definition, const FString& Text)
			: Definition(Definition)
			, Transaction(MakeUnique<FVoxelTransaction>(&Definition->Node, Text))
		{
		}
		~FScope()
		{
			Definition->Node.ReconstructFromVoxelNode();
		}
	};

public:
	explicit FVoxelMetaGraphStructNodeDefinition(UVoxelMetaGraphStructNode& Node, const TSharedRef<IVoxelNodeDefinition>& NodeDefinition) : Node(Node), NodeDefinition(NodeDefinition)
	{
	}

	virtual TSharedPtr<const FNode> GetInputs() const override;
	virtual TSharedPtr<const FNode> GetOutputs() const override;
	virtual bool OverridePinsOrder() const override { return true; }

	virtual FString GetAddPinLabel() const override;
	virtual FString GetAddPinTooltip() const override;
	virtual FString GetRemovePinTooltip() const override;

	virtual bool CanAddInputPin() const override;
	virtual void AddInputPin() override;

	virtual bool CanRemoveInputPin() const override;
	virtual void RemoveInputPin() override;

	virtual bool CanAddToCategory(FName Category) const override;
	virtual void AddToCategory(FName Category) override;

	virtual bool CanRemoveFromCategory(FName Category) const override;
	virtual void RemoveFromCategory(FName Category) override;

	virtual bool CanRemoveSelectedPin(FName PinName) const override;
	virtual void RemoveSelectedPin(FName PinName) override;

	virtual void InsertPinBefore(FName PinName) override;
	virtual void DuplicatePin(FName PinName) override;

	virtual FString GetPinTooltip(FName PinName) const override;
	virtual FString GetCategoryTooltip(FName CategoryName) const override;

private:
	UVoxelMetaGraphStructNode& Node;
	TSharedRef<IVoxelNodeDefinition> NodeDefinition;
};