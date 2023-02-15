// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "SVoxelDetailWidgets.h"
#include "VoxelEditorMinimal.h"

void SVoxelDetailText::Construct(const FArguments& Args)
{
	ChildSlot
	[
		SNew(STextBlock)
		.Font(Args._Font.IsSet() ? Args._Font : FVoxelEditorUtilities::Font())
		.Text(Args._Text)
		.ColorAndOpacity(Args._ColorAndOpacity)
	];
}

void SVoxelDetailButton::Construct(const FArguments& Args)
{
	ChildSlot
	[
		SNew(SButton)
		.ContentPadding(2)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked(Args._OnClicked)
		[
			SNew(SVoxelDetailText)
			.Text(Args._Text)
		]
	];
}