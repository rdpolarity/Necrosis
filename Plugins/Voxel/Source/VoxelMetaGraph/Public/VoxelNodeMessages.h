// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelQuery.h"

struct FVoxelNodeInterface;
struct FVoxelMetaGraphCompiledNode;

VOXEL_FWD_DECLARE_NAMESPACE_CLASS(MetaGraph, FPin);
VOXEL_FWD_DECLARE_NAMESPACE_CLASS(MetaGraph, FNode);
VOXEL_FWD_DECLARE_NAMESPACE_CLASS(MetaGraph, FGraph);

template<>
struct VOXELMETAGRAPH_API TVoxelMessageArgProcessor<FVoxelNode>
{
	static void ProcessArg(FVoxelMessageBuilder& Builder, const FVoxelNode* Node);
	static void ProcessArg(FVoxelMessageBuilder& Builder, const FVoxelNode& Node)
	{
		ProcessArg(Builder, &Node);
	}
};

template<>
struct VOXELMETAGRAPH_API TVoxelMessageArgProcessor<FVoxelQuery::FCallstack>
{
	static void ProcessArg(FVoxelMessageBuilder& Builder, const FVoxelQuery::FCallstack& Callstack);
};

template<typename T>
struct VOXELMETAGRAPH_API TVoxelMessageArgProcessor<T, typename TEnableIf<TIsDerivedFrom<T, FVoxelNodeInterface>::Value && !TIsSame<T, FVoxelNode>::Value>::Type>
{
	static void ProcessArg(FVoxelMessageBuilder& Builder, const T* Node)
	{
		FVoxelMessageArgProcessor::ProcessArg(Builder, Node ? &Node->GetNode() : nullptr);
	}
	static void ProcessArg(FVoxelMessageBuilder& Builder, const T& Node)
	{
		FVoxelMessageArgProcessor::ProcessArg(Builder, Node.GetNode());
	}
};

template<>
struct VOXELMETAGRAPH_API TVoxelMessageArgProcessor<FVoxelMetaGraphCompiledNode>
{
	static void ProcessArg(FVoxelMessageBuilder& Builder, const FVoxelMetaGraphCompiledNode* Node);
	static void ProcessArg(FVoxelMessageBuilder& Builder, const FVoxelMetaGraphCompiledNode& Node)
	{
		ProcessArg(Builder, &Node);
	}
};

template<>
struct VOXELMETAGRAPH_API TVoxelMessageArgProcessor<Voxel::MetaGraph::FNode>
{
	static void ProcessArg(FVoxelMessageBuilder& Builder, const Voxel::MetaGraph::FNode* Node);
	static void ProcessArg(FVoxelMessageBuilder& Builder, const Voxel::MetaGraph::FNode& Node)
	{
		ProcessArg(Builder, &Node);
	}
};

template<>
struct VOXELMETAGRAPH_API TVoxelMessageArgProcessor<Voxel::MetaGraph::FPin>
{
	static void ProcessArg(FVoxelMessageBuilder& Builder, const Voxel::MetaGraph::FPin* Pin);
	static void ProcessArg(FVoxelMessageBuilder& Builder, const Voxel::MetaGraph::FPin& Pin)
	{
		ProcessArg(Builder, &Pin);
	}
};