// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelGraphSchema.h"
#include "VoxelGraphSchemaAction.h"
#include "Nodes/VoxelMetaGraphParameterNode.h"
#include "VoxelMetaGraphSchema.generated.h"

struct FVoxelNode;
class UVoxelMetaGraph;
class FVoxelMetaGraphEditorToolkit;

USTRUCT()
struct FVoxelMetaGraphSchemaAction_NewMetaGraphNode : public FVoxelGraphSchemaAction
{
	GENERATED_BODY();

public:
	UPROPERTY()
	TObjectPtr<UVoxelMetaGraph> MetaGraph = nullptr;

	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	virtual void GetIcon(FSlateIcon& Icon, FLinearColor& Color) override;
};

USTRUCT()
struct FVoxelMetaGraphSchemaAction_NewParameterUsage : public FVoxelGraphSchemaAction
{
	GENERATED_BODY();

public:
	UPROPERTY()
	FGuid Guid;
	
	UPROPERTY()
	EVoxelMetaGraphParameterType ParameterType = {};

	UPROPERTY()
	FVoxelPinType PinType;

	UPROPERTY()
	bool bDeclaration = false;

	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	virtual void GetIcon(FSlateIcon& Icon, FLinearColor& Color) override;
};

USTRUCT()
struct FVoxelMetaGraphSchemaAction_NewParameter : public FVoxelGraphSchemaAction
{
	GENERATED_BODY();

public:
	UPROPERTY()
	EVoxelMetaGraphParameterType ParameterType = {};

	UPROPERTY()
	FString ParameterCategory;

	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
};

USTRUCT()
struct FVoxelMetaGraphSchemaAction_NewStructNode : public FVoxelGraphSchemaAction
{
	GENERATED_BODY();

public:
	UScriptStruct* Struct = nullptr;

	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	virtual void GetIcon(FSlateIcon& Icon, FLinearColor& Color) override;
};

USTRUCT()
struct FVoxelMetaGraphSchemaAction_NewPromotableStructNode : public FVoxelMetaGraphSchemaAction_NewStructNode
{
	GENERATED_BODY();

public:
	TArray<FVoxelPinType> PinTypes;

	using FVoxelMetaGraphSchemaAction_NewStructNode::FVoxelMetaGraphSchemaAction_NewStructNode;
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
};

USTRUCT()
struct FVoxelMetaGraphSchemaAction_NewKnotNode : public FVoxelGraphSchemaAction
{
	GENERATED_BODY();

public:
	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	virtual void GetIcon(FSlateIcon& Icon, FLinearColor& Color) override;
};

UCLASS()
class UVoxelMetaGraphSchema : public UVoxelGraphSchema
{
	GENERATED_BODY()

public:
	//~ Begin UVoxelGraphSchema Interface
	virtual TSharedPtr<FEdGraphSchemaAction> FindCastAction(const FEdGraphPinType& From, const FEdGraphPinType& To) const override;
	virtual TOptional<FPinConnectionResponse> GetCanCreateConnectionOverride(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const override;
	virtual bool CreatePromotedConnectionSafe(UEdGraphPin*& PinA, UEdGraphPin*& PinB) const override;

	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	virtual void GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;
	
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;
	virtual void OnPinConnectionDoubleCicked(UEdGraphPin* PinA, UEdGraphPin* PinB, const FVector2D& GraphPosition) const override;
	//~ End UVoxelGraphSchema Interface

	static TSharedPtr<FVoxelMetaGraphEditorToolkit> GetToolkit(const UEdGraph* Graph);

private:
	bool TryGetPromotionType(const UEdGraphPin& Pin, const FVoxelPinType& TargetType, FVoxelPinType& OutType) const;
	void PromoteToVariable(UEdGraphPin* Pin, EVoxelMetaGraphParameterType ParameterType) const;
	static TMap<FVoxelPinType, TSet<FVoxelPinType>> CollectOperatorPermutations(const TVoxelInstancedStruct<FVoxelNode>& Node, const UEdGraphPin& FromPin, const FVoxelPinTypeSet& PromotionTypes);
};