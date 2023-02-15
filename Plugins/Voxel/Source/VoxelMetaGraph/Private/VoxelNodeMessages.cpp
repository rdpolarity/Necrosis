// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelNodeMessages.h"
#include "VoxelNode.h"
#include "VoxelMetaGraph.h"
#include "VoxelMetaGraphGraph.h"

void TVoxelMessageArgProcessor<FVoxelNode>::ProcessArg(FVoxelMessageBuilder& Builder, const FVoxelNode* Node)
{
	if (!ensure(Node) ||
		!ensure(Node->NodeRuntime))
	{
		FVoxelMessageArgProcessor::ProcessArg(Builder, "Null");
		return;
	}

	{
		FVoxelScopeLock Lock(Node->NodeRuntime->CriticalSection);
		if (Node->NodeRuntime->Errors.Contains(Builder.Format))
		{
			Builder.Silence();
		}
		else
		{
			Node->NodeRuntime->Errors.Add(Builder.Format);
		}
	}

	FVoxelMessageArgProcessor::ProcessArg(Builder, Node->NodeRuntime->SourceNode);
}

void TVoxelMessageArgProcessor<FVoxelQuery::FCallstack>::ProcessArg(FVoxelMessageBuilder& Builder, const FVoxelQuery::FCallstack& Callstack)
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	FString Format;
	for (int32 Index = 0; Index < Callstack.Num(); Index++)
	{
		if (!Format.IsEmpty())
		{
			Format += " -> ";
		}
		Format += FString::Printf(TEXT("{%d}"), Index);
	}

	const TSharedRef<FVoxelMessageBuilder> ChildBuilder = MakeShared<FVoxelMessageBuilder>(Builder.Severity, Format);
	for (const FVoxelNode* Node : Callstack)
	{
		FVoxelMessageArgProcessor::ProcessArg(*ChildBuilder, Node);
	}

	FVoxelMessageArgProcessor::ProcessArg(Builder, ChildBuilder);
}

void TVoxelMessageArgProcessor<FVoxelMetaGraphCompiledNode>::ProcessArg(FVoxelMessageBuilder& Builder, const FVoxelMetaGraphCompiledNode* Node)
{
	FVoxelMessageArgProcessor::ProcessArg(Builder, Node ? Node->SourceGraphNode : nullptr);
}

void TVoxelMessageArgProcessor<Voxel::MetaGraph::FNode>::ProcessArg(FVoxelMessageBuilder& Builder, const Voxel::MetaGraph::FNode* Node)
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	if (!ensure(Node))
	{
		FVoxelMessageArgProcessor::ProcessArg(Builder, "Null");
		return;
	}

	if (Node->Source.GraphNodes.Num() == 0)
	{
		FVoxelMessageArgProcessor::ProcessArg(Builder, "<no source node>");
		return;
	}

	FString Format;
	for (int32 NodeIndex = 0; NodeIndex < Node->Source.GraphNodes.Num(); NodeIndex++)
	{
		if (NodeIndex != 0)
		{
			Format += ".";
		}

		Format += FString::Printf(TEXT("{%d}"), NodeIndex);
	}

	const TSharedRef<FVoxelMessageBuilder> ChildBuilder = MakeShared<FVoxelMessageBuilder>(Builder.Severity, Format);
	for (const TWeakObjectPtr<UEdGraphNode> GraphNode : Node->Source.GraphNodes)
	{
		FVoxelMessageArgProcessor::ProcessArg(*ChildBuilder, GraphNode);
	}

	FVoxelMessageArgProcessor::ProcessArg(Builder, ChildBuilder);
}

void TVoxelMessageArgProcessor<Voxel::MetaGraph::FPin>::ProcessArg(FVoxelMessageBuilder& Builder, const Voxel::MetaGraph::FPin* Pin)
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	if (!ensure(Pin))
	{
		FVoxelMessageArgProcessor::ProcessArg(Builder, "Null");
		return;
	}

	const TSharedRef<FVoxelMessageBuilder> ChildBuilder = MakeShared<FVoxelMessageBuilder>(Builder.Severity, "{0}.{1}");
	FVoxelMessageArgProcessor::ProcessArg(*ChildBuilder, Pin->Node);

	FString PinName = Pin->Name.ToString();
	if (Pin->Node.Type == ENodeType::Struct)
	{
		if (const TSharedPtr<FVoxelPin> VoxelPin = Pin->Node.Struct()->FindPin(Pin->Name))
		{
			PinName = VoxelPin->Metadata.DisplayName;
		}
	}
	FVoxelMessageArgProcessor::ProcessArg(*ChildBuilder, PinName);

	FVoxelMessageArgProcessor::ProcessArg(Builder, ChildBuilder);
}