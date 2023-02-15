// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphRuntime.h"
#include "VoxelMetaGraphCompilerUtilities.h"
#include "VoxelExecNode.h"
#include "VoxelGraphMessages.h"
#include "Nodes/VoxelEventNodes.h"

void FVoxelMetaGraphRuntime::Create()
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_USE_NAMESPACE(MetaGraph);

	FVoxelTaskStat::ClearStats();

	if (!MetaGraph.IsValid() ||
		bGetSubsystemsFailed)
	{
		return;
	}

	ensure(!NodeOuter);
	ensure(Objects.Num() == 0);

	const TSharedPtr<FGraph> Graph = FCompilerUtilities::Compile(*MetaGraph, VariableCollection, &Runtime);

	if (!Graph)
	{
		VOXEL_MESSAGE(Error, "Failed to compile {0}", MetaGraph);
		return;
	}
	
	struct FVoxelNodeOuter : public IVoxelNodeOuter
	{
		const TSharedRef<FGraph> Graph;

		explicit FVoxelNodeOuter(const TSharedRef<FGraph>& Graph)
			: Graph(Graph)
		{
		}
	};
	NodeOuter = MakeShared<FVoxelNodeOuter>(Graph.ToSharedRef());
	
	const TArray<FNode*> Nodes = Graph->GetNodesArray();
	for (const FNode* Node : Nodes)
	{
		check(Node->Type == ENodeType::Struct);
	}

	{
		const FVoxelGraphMessages Messages(MetaGraph->GetMainGraph());
		for (FNode* Node : Nodes)
		{
			Node->Struct()->InitializeNodeRuntime(&Runtime, NodeOuter, MetaGraph, Node);

			if (Messages.HasError())
			{
				return;
			}
		}
	}

	for (FNode* Node : Nodes)
	{
		FVoxelNode& NodeStruct = *Node->Struct();

		TArray<TSharedPtr<FVoxelPin>> Pins;
		NodeStruct.GetPinsMap().GenerateValueArray(Pins);

		ensure(Pins.Num() == Node->GetPins().Num());

		// Setup OutputPinData/DefaultValue for input pins
		for (const TSharedPtr<FVoxelPin>& Pin : Pins)
		{
			if (!Pin->bIsInput ||
				Pin->GetType().IsDerivedFrom<FVoxelExecBase>())
			{
				continue;
			}
			const FPin& InputPin = Node->FindInputChecked(Pin->Name);

			TSharedPtr<FVoxelNodeRuntime::FPinData> OutputPinData;
			if (InputPin.GetLinkedTo().Num() > 0)
			{
				check(InputPin.GetLinkedTo().Num() == 1);
				const FPin& OutputPin = InputPin.GetLinkedTo()[0];
				check(OutputPin.Direction == EPinDirection::Output);

				OutputPinData = OutputPin.Node.Struct()->GetNodeRuntime().GetPinData(OutputPin.Name).AsShared();
				check(!OutputPinData->bIsInput);
			}

			FVoxelNodeRuntime::FPinData& PinData = NodeStruct.GetNodeRuntime().GetPinData(Pin->Name);
			if (OutputPinData)
			{
				PinData.OutputPinData = OutputPinData;
			}
			else
			{
				PinData.DefaultValue = FVoxelSharedPinValue(InputPin.GetDefaultValue());
			}
		}

		// Setup Exec_LinkedTo for output pins
		for (const TSharedPtr<FVoxelPin>& Pin : Pins)
		{
			if (Pin->bIsInput ||
				!Pin->GetType().IsDerivedFrom<FVoxelExecBase>())
			{
				continue;
			}
			const FPin& OutputPin = Node->FindOutputChecked(Pin->Name);

			FVoxelNodeRuntime::FPinData& PinData = NodeStruct.GetNodeRuntime().GetPinData(Pin->Name);

			for (const FPin& LinkedTo : OutputPin.GetLinkedTo())
			{
				PinData.Exec_LinkedTo.Add(&LinkedTo.Node.Struct().Get());
			}
		}
	}

	for (FNode* Node : Nodes)
	{
		Node->Struct()->Initialize();
	}

	TArray<TVoxelFutureValue<FVoxelExecObject>> ObjectFutures;
	for (FNode* Node : Nodes)
	{
		const FVoxelExecNode_OnConstruct* OnConstruct = Cast<FVoxelExecNode_OnConstruct>(Node->Struct().Get());
		if (!OnConstruct)
		{
			continue;
		}

		OnConstruct->Execute(ObjectFutures);
	}

	FVoxelTask::New(
		MakeShared<FVoxelTaskStat>(),
		"OnConstruct",
		EVoxelTaskThread::GameThread,
		ReinterpretCastArray<FVoxelFutureValue>(ObjectFutures),
		[this, WeakThis = MakeWeakPtr(AsShared()), WeakRuntime = MakeWeakPtr(Runtime.AsShared()), ObjectFutures]
		{
			ensure(IsInGameThread());

			const TSharedPtr<IVoxelMetaGraphRuntime> This = WeakThis.Pin();
			const TSharedPtr<FVoxelRuntime> PinnedRuntime = WeakRuntime.Pin();
			if (!This.IsValid() || !PinnedRuntime.IsValid())
			{
				return;
			}

			for (const TVoxelFutureValue<FVoxelExecObject>& ObjectFuture : ObjectFutures)
			{
				const TSharedRef<FVoxelExecObject> Object = ConstCastSharedRef<FVoxelExecObject>(ObjectFuture.GetShared_CheckCompleted());
				if (Object->GetStruct() == FVoxelExecObject::StaticStruct())
				{
					continue;
				}

				Object->Create(*PinnedRuntime);
				Objects.Add(Object);
			}
		});
}

void FVoxelMetaGraphRuntime::Destroy()
{
	VOXEL_FUNCTION_COUNTER();

	for (const TSharedPtr<FVoxelExecObject>& Object : Objects)
	{
		Object->Destroy(Runtime);
	}

	NodeOuter.Reset();
	Objects.Reset();
}

void FVoxelMetaGraphRuntime::Tick()
{
	VOXEL_FUNCTION_COUNTER();

	for (const TSharedPtr<FVoxelExecObject>& Object : Objects)
	{
		Object->Tick(Runtime);
	}
}