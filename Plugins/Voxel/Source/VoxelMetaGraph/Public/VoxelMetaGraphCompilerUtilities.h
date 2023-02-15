// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMetaGraphGraph.h"
#include "VoxelMetaGraphVariableCollection.h"

struct FVoxelQuery;
struct FVoxelResult;

BEGIN_VOXEL_NAMESPACE(MetaGraph)

struct FCompileInfo;

struct VOXELMETAGRAPH_API FCompilerUtilities
{
public:
	static TSharedPtr<FGraph> Compile(
		const UVoxelMetaGraph& MetaGraph,
		FVoxelMetaGraphVariableCollection Variables,
		FVoxelRuntime* Runtime);
	
public:
	static void CheckMetaGraph(const UVoxelMetaGraph& MetaGraph);

	static TSharedPtr<FGraph> TranslateCompiledGraph(
		const FVoxelMetaGraphCompiledGraph& CompiledGraph,
		const TArray<FVoxelMetaGraphParameter>& Parameters,
		FVoxelRuntime* Runtime,
		const FSourceNode& MetaGraphNodeSource);

	static void FlattenGraph(FGraph& Graph, FVoxelRuntime* Runtime);
	static bool FlattenGraphImpl(FGraph& Graph, FVoxelRuntime* Runtime);
	
	static void CheckWildcards(const FGraph& Graph);
	static void ReplaceTemplates(FGraph& Graph);
	static bool ReplaceTemplatesImpl(FGraph& Graph);
	static void InitializeTemplatesPassthroughNodes(FGraph& Graph, FNode& Node);
	
	static void RemovePassthroughs(FGraph& Graph);

	static void CheckOutputs(const FGraph& Graph);
	static void ReplaceParameters(FGraph& Graph, const FVoxelMetaGraphVariableCollection& Variables, FVoxelRuntime* Runtime);

	static void ReplaceCodeGenNodes(FGraph& Graph);
	static bool ReplaceCodeGenNodesImpl(FGraph& Graph);

public:
	static void RemoveUnusedNodes(FGraph& Graph);

	static TSet<const FNode*> FindNodesPredecessors(
		const TArray<const FNode*>& Nodes,
		const TFunction<bool(const FNode&)>& ShouldFindPredecessors = nullptr);
	
	static TArray<const FNode*> SortNodes(const TArray<const FNode*>& Nodes);

	static TArray<FNode*> SortNodes(const TArray<FNode*>& Nodes)
	{
		return ReinterpretCastArray<FNode*>(SortNodes(ReinterpretCastArray<const FNode*>(Nodes)));
	}

	static TSharedRef<FGraph> MakeSubGraph(
		const FGraph& Graph, const TArray<const FNode*>& Nodes,
		TMap<const FPin*, FPin*>& OldToNewPins);

private:
	static bool MakeDefaultValue(
		const FVoxelPinType& Type,
		const FVoxelPinValue& ExposedDefaultValue,
		FVoxelRuntime* Runtime,
		FVoxelPinValue& NewValue);
};

END_VOXEL_NAMESPACE(MetaGraph)