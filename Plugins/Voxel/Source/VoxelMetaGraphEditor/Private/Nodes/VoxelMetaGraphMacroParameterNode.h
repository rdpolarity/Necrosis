// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMetaGraphParameterNodeBase.h"
#include "VoxelMetaGraphMacroParameterNode.generated.h"

UCLASS()
class UVoxelMetaGraphMacroParameterNode : public UVoxelMetaGraphParameterNodeBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
	EVoxelMetaGraphParameterType Type;

	//~ Begin UVoxelGraphNode Interface
	virtual void PostPasteNode() override;
	virtual bool CanPasteHere(const UEdGraph* TargetGraph) const override;

	virtual void AllocateParameterPins(const FVoxelMetaGraphParameter& Parameter) override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	//~ End UVoxelGraphNode Interface
};