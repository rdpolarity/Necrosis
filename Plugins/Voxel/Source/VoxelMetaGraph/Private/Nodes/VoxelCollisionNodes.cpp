// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelCollisionNodes.h"

void FVoxelChunkExecObject_CreateCollisionComponent::Create(FVoxelRuntime& Runtime) const
{
	if (!ensure(Collider))
	{
		return;
	}

	ensure(!CollisionId.IsValid());
	CollisionId = Runtime.GetSubsystem<FVoxelCollisionProcessor>().CreateCollision(Position, *BodyInstance, Collider.ToSharedRef());
}

void FVoxelChunkExecObject_CreateCollisionComponent::Destroy(FVoxelRuntime& Runtime) const
{
	ensure(CollisionId.IsValid());
	Runtime.GetSubsystem<FVoxelCollisionProcessor>().DestroyCollision(CollisionId);
	CollisionId = {};
}

TVoxelFutureValue<FVoxelChunkExecObject> FVoxelChunkExecNode_CreateCollisionComponent::Execute(const FVoxelQuery& Query) const
{
	FindVoxelQueryData(FVoxelBoundsQueryData, BoundsQueryData);

	const TValue<FVoxelCollider> Collider = GetNodeRuntime().Get(ColliderPin, Query);
	const TValue<FBodyInstance> BodyInstance = GetNodeRuntime().Get(BodyInstancePin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, BodyInstance, Collider)
	{
		if (Collider->GetStruct() == FVoxelCollider::StaticStruct())
		{
			return {};
		}

		const TSharedRef<FVoxelChunkExecObject_CreateCollisionComponent> Object = MakeShared<FVoxelChunkExecObject_CreateCollisionComponent>();
		Object->Position = BoundsQueryData->Bounds.Min;
		Object->BodyInstance = BodyInstance;
		Object->Collider = Collider;
		return Object;
	};
}