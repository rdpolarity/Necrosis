// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelPinType.h"

class SGraphPin;

struct FVoxelMetaGraphVisuals
{
	static FSlateIcon GetPinIcon(const FVoxelPinType& Type);
	static FLinearColor GetPinColor(const FVoxelPinType& Type);
	static TSharedPtr<SGraphPin> GetPinWidget(UEdGraphPin* Pin);
};