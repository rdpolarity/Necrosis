// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphBlueprintAsset.h"

DEFINE_VOXEL_BLUEPRINT_FACTORY(UVoxelMetaGraphBlueprintAsset);
DEFINE_VOXEL_SUBSYSTEM(FVoxelMetaGraphBlueprintAssetSubsystem);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int32 FVoxelMetaGraphBlueprintAssetSubsystem::RegisterAsset(const UVoxelMetaGraphBlueprintAsset* Asset)
{
	check(IsInGameThread());

	if (const int32* Index = AssetToData.Find(Asset))
	{
		return *Index;
	}

	if (!Asset)
	{
		Asset = GetDefault<UVoxelMetaGraphBlueprintAsset>();
	}

	const TSharedRef<FVoxelMetaGraphBlueprintAssetPropertyData> Data = MakeShared<FVoxelMetaGraphBlueprintAssetPropertyData>();
	
	for (const FProperty& Property : GetStructProperties(Asset->GetClass()))
	{
		if (!Property.HasAllPropertyFlags(CPF_BlueprintVisible))
		{
			continue;
		}

		if (Property.IsA<FFloatProperty>())
		{
			Data->FloatProperties.Add(Property.GetFName(), *Property.ContainerPtrToValuePtr<float>(Asset));
		}
		else if (Property.IsA<FDoubleProperty>())
		{
			Data->FloatProperties.Add(Property.GetFName(), *Property.ContainerPtrToValuePtr<double>(Asset));
		}
	}

	const int32 Index = Datas.Add(Data);
	AssetToData.Add(Asset, Index);
	return Index;
}