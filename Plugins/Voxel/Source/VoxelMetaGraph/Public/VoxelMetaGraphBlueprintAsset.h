// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelRuntime/VoxelSubsystem.h"
#include "VoxelMetaGraphBlueprintAsset.generated.h"

UCLASS(BlueprintType, Blueprintable, meta = (VoxelAssetType, AssetColor=LightBlue))
class VOXELMETAGRAPH_API UVoxelMetaGraphBlueprintAsset : public UObject
{
	GENERATED_BODY()
};

struct FVoxelMetaGraphBlueprintAssetPropertyData
{
	TMap<FName, float> FloatProperties;
};

UCLASS()
class VOXELMETAGRAPH_API UVoxelMetaGraphBlueprintAssetSubsystemProxy : public UVoxelSubsystemProxy
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(FVoxelMetaGraphBlueprintAssetSubsystem);
};

class VOXELMETAGRAPH_API FVoxelMetaGraphBlueprintAssetSubsystem : public IVoxelSubsystem
{
public:
	GENERATED_VOXEL_SUBSYSTEM_BODY(UVoxelMetaGraphBlueprintAssetSubsystemProxy);

public:
	int32 RegisterAsset(const UVoxelMetaGraphBlueprintAsset* Asset);

	FORCEINLINE const FVoxelMetaGraphBlueprintAssetPropertyData* GetData(int32 Index) const
	{
		return Datas[Index].Get();
	}

private:
	TVoxelArray<TSharedPtr<const FVoxelMetaGraphBlueprintAssetPropertyData>> Datas;
	TMap<const UVoxelMetaGraphBlueprintAsset*, int32> AssetToData;
};