// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBlockAsset.h"
#include "VoxelRuntime/VoxelSubsystem.h"
#include "VoxelBlockRegistry.generated.h"

UCLASS()
class VOXELBLOCK_API UVoxelBlockRegistryProxy : public UVoxelSubsystemProxy
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(FVoxelBlockRegistry);
};

class VOXELBLOCK_API FVoxelBlockRegistry : public IVoxelSubsystem
{
public:
	GENERATED_VOXEL_SUBSYSTEM_BODY(UVoxelBlockRegistryProxy);

	//~ Begin IVoxelRenderer Interface
	virtual void Initialize() override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	//~ End IVoxelRenderer Interface

public:
	FORCEINLINE const TMap<FVoxelBlockId, TObjectPtr<UVoxelBlockAsset>>& GetBlockAssets() const
	{
		return BlockAssets;
	}

	FORCEINLINE FVoxelBlockId GetBlockId(const UVoxelBlockAsset* Asset) const
	{
		checkVoxelSlow(IsInGameThread());
		ensureVoxelSlowNoSideEffects(BlockAssetsIds.Contains(Asset));
		return BlockAssetsIds.FindRef(Asset);
	}
	FORCEINLINE FVoxelBlockData GetBlockData(const UVoxelBlockAsset* Asset) const
	{
		return GetBlockData(GetBlockId(Asset));
	}
	FORCEINLINE UVoxelBlockAsset* GetBlockAsset(const FVoxelBlockId Id) const
	{
		checkVoxelSlow(IsInGameThread());
		ensureVoxelSlowNoSideEffects(BlockAssets.Contains(Id));
		return BlockAssets.FindRef(Id);
	}
	FORCEINLINE FVoxelBlockData GetBlockData(const FVoxelBlockId Id) const
	{
		ensureVoxelSlowNoSideEffects(BlockDatas.Contains(Id));
		return BlockDatas.FindRef(Id);
	}

private:
	TMap<FVoxelBlockId, FVoxelBlockData> BlockDatas;
	TMap<FVoxelBlockId, TObjectPtr<UVoxelBlockAsset>> BlockAssets;
	TMap<UVoxelBlockAsset*, FVoxelBlockId> BlockAssetsIds;
};