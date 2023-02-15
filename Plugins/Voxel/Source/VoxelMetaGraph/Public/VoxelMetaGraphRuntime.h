// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelRuntime/VoxelRuntime.h"
#include "VoxelMetaGraphVariableCollection.h"

class UVoxelMetaGraph;
struct FVoxelExecObject;

class VOXELMETAGRAPH_API FVoxelMetaGraphRuntime : public IVoxelMetaGraphRuntime
{
public:
	FVoxelRuntime& Runtime;
	const TWeakObjectPtr<const UVoxelMetaGraph> MetaGraph;
	const FVoxelMetaGraphVariableCollection VariableCollection;

	FVoxelMetaGraphRuntime(
		FVoxelRuntime& Runtime,
		const TWeakObjectPtr<const UVoxelMetaGraph> MetaGraph,
		const FVoxelMetaGraphVariableCollection& VariableCollection)
		: Runtime(Runtime)
		, MetaGraph(MetaGraph)
		, VariableCollection(VariableCollection)
	{
	}

	virtual void Create() override;
	virtual void Destroy() override;
	virtual void Tick() override;

private:
	bool bGetSubsystemsFailed = false;
	TSharedPtr<IVoxelNodeOuter> NodeOuter;
	TArray<TSharedPtr<FVoxelExecObject>> Objects;
};