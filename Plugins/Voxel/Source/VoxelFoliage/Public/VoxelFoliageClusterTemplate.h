// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFoliageInstanceTemplate.h"
#include "VoxelFoliageClusterTemplate.generated.h"

UCLASS()
class VOXELFOLIAGE_API UVoxelFoliageClusterEntry : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	TObjectPtr<UVoxelFoliageInstanceTemplate> Instance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel", meta = (UIMin = "0", ClampMin = "0"))
	FVoxelFloatInterval SpawnRadius = { 0.f, 0.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel", meta = (UIMin = "1", ClampMin = "1"))
	FVoxelInt32Interval InstancesCount = {1, 1};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel", meta = (UIMin = "0", ClampMin = "0", UIMax = "360", ClampMax = "360", Units = "Degrees"))
	float RadialOffset = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel", meta = (UIMin = "0", ClampMin = "0"))
	float CullDistanceMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel", meta = (UIMin = "0", ClampMin = "0"))
	float ScaleMultiplier = 1.f;

	// Enables masks checking for each spawned instance
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	bool bCheckInstancesMasks = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Restrictions", meta = (ShowOnlyInnerProperties, Override))
	FVoxelFoliageSpawnRestriction SpawnRestriction;

	UStaticMesh* GetFirstInstanceMesh() const;
};

struct VOXELFOLIAGE_API FVoxelFoliageClusterEntryProxy
{
	const FVoxelFoliageInstanceTemplateProxy Instance;
	const FVoxelFloatInterval SpawnRadius;
	const FVoxelInt32Interval InstancesCount;
	const float RadialOffset;
	const float CullDistanceMultiplier;
	const float ScaleMultiplier;
	const bool bCheckInstancesMasks;
	FVoxelFoliageSpawnRestriction SpawnRestriction;

	explicit FVoxelFoliageClusterEntryProxy(const UVoxelFoliageClusterEntry& ClusterEntry)
		: Instance(*ClusterEntry.Instance)
		, SpawnRadius(ClusterEntry.SpawnRadius)
		, InstancesCount(ClusterEntry.InstancesCount)
		, RadialOffset(ClusterEntry.RadialOffset)
		, CullDistanceMultiplier(ClusterEntry.CullDistanceMultiplier)
		, ScaleMultiplier(ClusterEntry.ScaleMultiplier)
		, bCheckInstancesMasks(ClusterEntry.bCheckInstancesMasks)
		, SpawnRestriction(Instance.SpawnRestriction, ClusterEntry.SpawnRestriction)
	{
	}
};

UCLASS(BlueprintType, meta = (VoxelAssetType, AssetSubMenu=Foliage, AssetColor=LightGreen))
class VOXELFOLIAGE_API UVoxelFoliageClusterTemplate : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<TObjectPtr<UVoxelFoliageClusterEntry>> Entries;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base", meta = (UIMin = "0.000001", ClampMin = "0.000001"))
	float DistanceBetweenPoints = 2000.f;

	// Applied on top of all the foliage types restrictions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Restrictions", meta = (ShowOnlyInnerProperties, NoOverrideCustomization))
	FVoxelFoliageSpawnRestriction SpawnRestriction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Restrictions", meta = (InlineEditConditionToggle))
	bool bEnableHeightDifferenceRestriction = false;

	// Max height difference between cluster center and instance
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Restrictions", meta = (EditCondition = "bEnableHeightDifferenceRestriction", UIMin = "0", ClampMin = "0"))
	float MaxHeightDifference = 2000.f;

	// Despawns the cluster if average mask input from all instances is below this value. The despawned cluster will still cost the same, since all values need to be calculated.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Restrictions", meta = (UIMin = "0", UIMax = "100", ClampMin = "0", ClampMax = "100", Units = "Percent", DisplayName = "Minimum Weight to Spawn"))
	float MaskOccupancy = 0.f;

	TArray<FVoxelFoliageClusterEntryProxy> MakeEntryProxies() const
	{
		TArray<FVoxelFoliageClusterEntryProxy> Result;
		for (const UVoxelFoliageClusterEntry* Entry : Entries)
		{
			if (!Entry ||
				!Entry->Instance)
			{
				continue;
			}

			Result.Add(FVoxelFoliageClusterEntryProxy(*Entry));
		}
		return Result;
	}
};


struct VOXELFOLIAGE_API FVoxelFoliageClusterTemplateProxy
{
public:
	const TArray<FVoxelFoliageClusterEntryProxy> Entries;
	const float DistanceBetweenPoints;
	const bool bEnableHeightDifferenceRestriction;
	const float MaxHeightDifference;
	const float MaskOccupancy;
	const FVoxelFoliageSpawnRestriction SpawnRestriction;

	explicit FVoxelFoliageClusterTemplateProxy(const UVoxelFoliageClusterTemplate& Cluster)
		: Entries(Cluster.MakeEntryProxies())
		, DistanceBetweenPoints(Cluster.DistanceBetweenPoints)
		, bEnableHeightDifferenceRestriction(Cluster.bEnableHeightDifferenceRestriction)
		, MaxHeightDifference(Cluster.MaxHeightDifference)
		, MaskOccupancy(Cluster.MaskOccupancy)
		, SpawnRestriction(Cluster.SpawnRestriction)
	{
	}
};