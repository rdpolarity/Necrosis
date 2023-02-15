// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

class VOXELCOREEDITOR_API SVoxelDetailText : public SCompoundWidget
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_ATTRIBUTE(FText, Text);
		SLATE_ATTRIBUTE(FSlateColor, ColorAndOpacity);
		SLATE_ATTRIBUTE(FSlateFontInfo, Font);
	};

	void Construct(const FArguments& Args);
};

class VOXELCOREEDITOR_API SVoxelDetailButton : public SCompoundWidget
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_ATTRIBUTE(FText, Text);
		SLATE_EVENT(FOnClicked, OnClicked);
	};

	void Construct(const FArguments& Args);
};