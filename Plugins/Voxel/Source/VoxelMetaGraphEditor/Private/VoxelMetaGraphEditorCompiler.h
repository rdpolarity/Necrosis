	// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelPinValue.h"
#include "EdGraph/EdGraph.h"

class UVoxelGraphNode;

BEGIN_VOXEL_NAMESPACE(MetaGraph)

struct FEditorNode;
struct FEditorGraph;

struct FEditorPin
{
	FEditorNode* Node = nullptr;
	FName PinName;
	FVoxelPinType PinType;
	FVoxelPinValue DefaultValue;
	TSet<FEditorPin*> LinkedTo;

	void MakeLinkTo(FEditorPin* Other)
	{
		LinkedTo.Add(Other);
		Other->LinkedTo.Add(this);
	}
	void BreakAllLinks()
	{
		for (FEditorPin* Other : LinkedTo)
		{
			ensure(Other->LinkedTo.Remove(this));
		}
		LinkedTo.Reset();
	}
	void Check(const FEditorGraph& Graph) const;
};

struct FEditorNode
{
	const UVoxelGraphNode* GraphNode = nullptr;

	// The non-transient node that was the source of this node creation
	// Might not match the tye of GraphNode
	UEdGraphNode* SourceGraphNode = nullptr;

	TArray<FEditorPin*> InputPins;
	TArray<FEditorPin*> OutputPins;
};

struct FEditorGraph
{
public:
	TSet<FEditorNode*> Nodes;

	FEditorGraph() = default;
	UE_NONCOPYABLE(FEditorGraph);

private:
	TArray<TUniquePtr<FEditorPin>> PinRefs;
	TArray<TUniquePtr<FEditorNode>> NodeRefs;

	FEditorPin* NewPin()
	{
		return PinRefs.Add_GetRef(MakeUnique<FEditorPin>()).Get();
	}
	FEditorNode* NewNode()
	{
		return NodeRefs.Add_GetRef(MakeUnique<FEditorNode>()).Get();
	}

	friend struct FEditorCompilerUtilities;
};

class FEditorCompiler
{
public:
	UE_NONCOPYABLE(FEditorCompiler);

	TArray<UEdGraphNode*> GetNodesCopy() const
	{
		return Graph->Nodes;
	}

	template<typename T>
	T& CreateNode(UEdGraphNode* SourceNode)
	{
		T* Node = NewObject<T>(Graph, NAME_None, RF_Transient);
		Graph->AddNode(Node);

		UEdGraphNode* ActualSourceNode = NodeSourceMap.FindRef(SourceNode);
		ensure(ActualSourceNode);
		NodeSourceMap.Add(Node, ActualSourceNode);

		return *Node;
	}
	UEdGraphNode* GetSourceNode(const UEdGraphNode* Node) const
	{
		UEdGraphNode* SourceNode = NodeSourceMap.FindRef(Node);
		ensure(SourceNode);
		return SourceNode;
	}
	UEdGraphNode* GetNodeFromSourceNode(const UEdGraphNode* SourceNode) const
	{
		UEdGraphNode* Node = nullptr;
		for (const auto& It : NodeSourceMap)
		{
			if (It.Value == SourceNode)
			{
				ensure(!Node);
				Node = It.Key;
			}
		}
		ensure(Node);
		return Node;
	}

private:
	FEditorCompiler() = default;

	UEdGraph* Graph = nullptr;
	TMap<UEdGraphNode*, UEdGraphNode*> NodeSourceMap;

	friend struct FEditorCompilerUtilities;
};

using FFEditorCompilerPass = TFunction<bool(FEditorCompiler& Compiler)>;

struct FEditorCompilerUtilities
{
public:
	static bool CompileGraph(
		const UEdGraph& EdGraph,
		FEditorGraph& OutGraph,
		const TArray<FFEditorCompilerPass>& Passes);

private:
	static bool TranslateGraph(
		const FEditorCompiler& Compiler,
		FEditorGraph& OutGraph);

	static void CheckGraph(const FEditorCompiler& Compiler);
};

END_VOXEL_NAMESPACE(MetaGraph)