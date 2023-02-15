// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "VoxelFoliageComponent.generated.h"

struct FVoxelFoliageData;

DECLARE_UNIQUE_VOXEL_ID(FVoxelFoliageComponentTreeId);

struct FVoxelFoliageBuiltData
{
	TUniquePtr<FStaticMeshInstanceData> InstanceBuffer;
	TArray<FClusterNode> ClusterTree;
	int32 OcclusionLayerNum = 0;

	TVoxelArray<FInstancedStaticMeshInstanceData> InstanceDatas;
	TVoxelArray<float> CustomDatas;
	TVoxelArray<int32> InstanceReorderTable;
};

UCLASS()
class VOXELFOLIAGE_API UVoxelFoliageComponent : public UHierarchicalInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:
	TSharedPtr<const FVoxelFoliageData> FoliageData;

	UVoxelFoliageComponent();

	void StartTreeBuild();
	virtual void ClearInstances() override;
	virtual void DestroyComponent(bool bPromoteChildren) override;

	static int32 GetDesiredInstancesPerLeaf(const UStaticMesh& StaticMesh);

	static void AsyncTreeBuild(
		FVoxelFoliageBuiltData& OutBuiltData,
		const FBox& MeshBox,
		int32 InDesiredInstancesPerLeaf,
		const FVoxelFoliageData& InFoliageData);

private:
	FVoxelFoliageComponentTreeId QueuedTreeId;

	void FinishTreeBuild(FVoxelFoliageBuiltData&& BuiltData);
};