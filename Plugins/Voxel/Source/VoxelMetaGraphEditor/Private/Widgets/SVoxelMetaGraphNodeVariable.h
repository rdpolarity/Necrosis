// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "SVoxelMetaGraphNode.h"

class SVoxelMetaGraphNodeVariable : public SVoxelMetaGraphNode
{
public:
	VOXEL_SLATE_ARGS()
	{
	};

	void Construct(const FArguments& InArgs, UVoxelGraphNode* InNode);

	//~ Begin SVoxelGraphNode Interface
	virtual void UpdateGraphNode() override;
	virtual const FSlateBrush* GetShadowBrush(bool bSelected) const override;
	//~ End SVoxelGraphNode Interface

private:
	FSlateColor GetVariableColor() const;
	TSharedRef<SWidget> UpdateTitleWidget(FText InTitleText) const;
};