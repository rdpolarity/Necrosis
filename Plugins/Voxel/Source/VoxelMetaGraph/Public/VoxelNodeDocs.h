// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

#if WITH_EDITOR
class VOXELMETAGRAPH_API FVoxelNodeDocs
{
public:
	static FVoxelNodeDocs& Get();

	FString GetNodeTooltip(const UScriptStruct* Node)
	{
		return GetPinTooltip(Node, FName());
	}
	FString GetCategoryTooltip(const UScriptStruct* Node, FName CategoryName)
	{
		return GetPinTooltip(Node, "CAT_" + CategoryName);
	}
	FString GetPinTooltip(const UScriptStruct* Node, FName PinName);

	void Initialize();

private:
	TMap<TPair<FName, FName>, FString> Map;
};
#endif