// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelBlockRegistry.h"
#include "VoxelRuntime/VoxelRuntimeUtilities.h"
#include "Engine/AssetManager.h"

VOXEL_CONSOLE_COMMAND(
	LogBlockIds,
	"voxel.block.LogBlockIds",
	"")
{
	FVoxelRuntimeUtilities::ForeachRuntime(nullptr, [](const FVoxelRuntime& Runtime)
	{
		for (const auto& It : Runtime.GetSubsystem<FVoxelBlockRegistry>().GetBlockAssets())
		{
			UE_LOG(LogConsoleResponse, Display, TEXT("%d -> %s"), It.Key.GetIndex(), *It.Value->GetName());
		}
	});
}

DEFINE_VOXEL_SUBSYSTEM(FVoxelBlockRegistry);

void FVoxelBlockRegistry::Initialize()
{
	Super::Initialize();

	UAssetManager& AssetManager = UAssetManager::Get();

	AssetManager.ScanPathForPrimaryAssets(
		UVoxelBlockAsset::PrimaryAssetType,
		"/Game/",
		UVoxelBlockAsset::StaticClass(),
		false);

	TArray<FAssetData> AssetDataList;
	AssetManager.GetPrimaryAssetDataList(UVoxelBlockAsset::PrimaryAssetType, AssetDataList);

	for (const FAssetData& AssetData : AssetDataList)
	{
		UVoxelBlockAsset* BlockAsset = Cast<UVoxelBlockAsset>(AssetData.GetAsset());
		if (!BlockAsset)
		{
			continue;
		}

		const FVoxelBlockId BlockId(BlockAssets.Num());

		BlockDatas.Add(BlockId, FVoxelBlockData(
			BlockId,
			{},
			BlockAsset->bIsAir,
			BlockAsset->bIsMasked,
			BlockAsset->bAlwaysRenderInnerFaces,
			BlockAsset->bIsTwoSided));

		BlockAssets.Add(BlockId, BlockAsset);
		BlockAssetsIds.Add(BlockAsset, BlockId);
	}
}

void FVoxelBlockRegistry::AddReferencedObjects(FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(Collector);

	Collector.AddReferencedObjects(BlockAssets);
}