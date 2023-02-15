// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelNavmeshNodes.h"

void FVoxelChunkExecObject_CreateNavmeshComponent::Create(FVoxelRuntime& Runtime) const
{
	if (!ensure(Navmesh))
	{
		return;
	}

	ensure(!NavmeshId.IsValid());
	NavmeshId = Runtime.GetSubsystem<FVoxelNavmeshProcessor>().CreateNavmesh(Position, Navmesh.ToSharedRef());
}

void FVoxelChunkExecObject_CreateNavmeshComponent::Destroy(FVoxelRuntime& Runtime) const
{
	ensure(NavmeshId.IsValid());
	Runtime.GetSubsystem<FVoxelNavmeshProcessor>().DestroyNavmesh(NavmeshId);
	NavmeshId = {};
}

TVoxelFutureValue<FVoxelChunkExecObject> FVoxelChunkExecNode_CreateNavmeshComponent::Execute(const FVoxelQuery& Query) const
{
	FindVoxelQueryData(FVoxelBoundsQueryData, BoundsQueryData);
	const TValue<FVoxelNavmesh> Navmesh = GetNodeRuntime().Get(NavmeshPin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, Navmesh)
	{
		if (Navmesh->Vertices.Num() == 0)
		{
			return {};
		}

		const TSharedRef<FVoxelChunkExecObject_CreateNavmeshComponent> Object = MakeShared<FVoxelChunkExecObject_CreateNavmeshComponent>();
		Object->Position = BoundsQueryData->Bounds.Min;
		Object->Navmesh = Navmesh;
		return Object;
	};
}