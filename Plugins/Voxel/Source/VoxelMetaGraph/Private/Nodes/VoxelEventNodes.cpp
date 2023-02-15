// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelEventNodes.h"
#include "VoxelActor.h"

void FVoxelExecNode_OnConstruct::Execute(TArray<TValue<FVoxelExecObject>>& OutObjects) const
{
	for (const FVoxelNode* LinkedTo : GetNodeRuntime().GetPinData(ThenPin).Exec_LinkedTo)
	{
		const FVoxelExecNode* Node = Cast<FVoxelExecNode>(*LinkedTo);
		if (!ensure(Node))
		{
			continue;
		}

		Node->Execute(OutObjects);
	}
}

TVoxelFutureValue<FVoxelExecObject> FVoxelExecNode_BroadcastEvent::Execute(const FVoxelQuery& Query) const
{
	const TValue<FName> DelegateName = GetNodeRuntime().Get(EventNamePin, Query);
	return VOXEL_ON_COMPLETE(GameThread, DelegateName)
	{
		AVoxelActor* Actor = Cast<AVoxelActor>(GetNodeRuntime().GetRuntime().GetSettings().GetActor());
		if (!ensure(Actor))
		{
			return {};
		}

		TArray<FVoxelDynamicGraphEvent>* Event = Actor->Events.Find(DelegateName);
		if (!Event)
		{
			return {};
		}

		for (auto It = Event->CreateIterator(); It; ++It)
		{
			if (!It->IsBound())
			{
				It.RemoveCurrent();
				continue;
			}

			It->Execute();
		}

		return {};
	};
}