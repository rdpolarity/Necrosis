// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelMetaGraphParameterNodeBase.h"
#include "VoxelMetaGraphLocalVariableNode.generated.h"

class UVoxelMetaGraphLocalVariableUsageNode;

UCLASS(Abstract)
class UVoxelMetaGraphLocalVariableNode : public UVoxelMetaGraphParameterNodeBase
{
	GENERATED_BODY()

public:
	//~ Begin UVoxelGraphNode Interface
	virtual void PostPasteNode() override;
	//~ End UVoxelGraphNode Interface
};

UCLASS()
class UVoxelMetaGraphLocalVariableDeclarationNode : public UVoxelMetaGraphLocalVariableNode
{
	GENERATED_BODY()

public:
	//~ Begin UVoxelGraphLocalVariableNode Interface
	virtual void AllocateParameterPins(const FVoxelMetaGraphParameter& Parameter) override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual bool CanJumpToDefinition() const override { return true; }
	//~ End UVoxelGraphLocalVariableNode Interface

	UVoxelGraphNode* IsInLoop();
};

UCLASS()
class UVoxelMetaGraphLocalVariableUsageNode : public UVoxelMetaGraphLocalVariableNode
{
	GENERATED_BODY()

public:
	//~ Begin UVoxelGraphLocalVariableNode Interface
	virtual void AllocateParameterPins(const FVoxelMetaGraphParameter& Parameter) override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	
	virtual bool CanJumpToDefinition() const override { return true; }
	virtual void JumpToDefinition() const override;
	//~ End UVoxelGraphLocalVariableNode Interface

	UVoxelMetaGraphLocalVariableDeclarationNode* FindDeclaration() const;
};