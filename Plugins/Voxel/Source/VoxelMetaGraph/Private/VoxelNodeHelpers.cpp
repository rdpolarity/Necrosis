// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelNodeHelpers.h"
#include "VoxelNode.h"

const FVoxelNode& FVoxelNodeInterface::GetNode() const
{
	check(false);
	return *new FVoxelNode();
}

void FVoxelNodeHelpers::RaiseQueryError(const FVoxelNodeInterface& Node, const FVoxelQuery& Query, const UScriptStruct* QueryType)
{
	VOXEL_MESSAGE(Error, "{0}: {1} is required but not provided by callee. Callstack: {2}",
		Node,
		QueryType->GetName(),
		Query.Callstack);
}

void FVoxelNodeHelpers::RaiseBufferError(const FVoxelNodeInterface& Node)
{
	VOXEL_MESSAGE(Error, "{0}: Inputs have different buffer sizes", Node);
}