// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelFoliageCollisionNode.h"
#include "Nodes/VoxelPositionNodes.h"
#include "Collision/VoxelFoliageCollision.h"

void FVoxelChunkExecObject_CreateFoliageCollision::Create(FVoxelRuntime& Runtime) const
{
	if (!ensure(TemplatesData.Num() > 0) ||
		!ensure(CollisionIds.Num() == 0))
	{
		return;
	}
	
	FVoxelFoliageCollision& FoliageCollision = Runtime.GetSubsystem<FVoxelFoliageCollision>();

	for (const FVoxelFoliageCollisionData& Data : TemplatesData)
	{
		if (Data.FoliageData->Transforms->Num() == 0)
		{
			continue;
		}

		CollisionIds.Add(FoliageCollision.CreateMesh(
			Position,
			Data.StaticMesh.Get(),
			Data.BodyInstance,
			Data.FoliageData.ToSharedRef()));
	}
}

void FVoxelChunkExecObject_CreateFoliageCollision::Destroy(FVoxelRuntime& Runtime) const
{
	FVoxelFoliageCollision& FoliageCollision = Runtime.GetSubsystem<FVoxelFoliageCollision>();

	for (FVoxelFoliageCollisionId& Id : CollisionIds)
	{
		ensure(Id.IsValid());
		FoliageCollision.DestroyMesh(Id);
	}

	CollisionIds = {};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelFutureValue<FVoxelChunkExecObject> FVoxelChunkExecNode_CreateFoliageCollision::Execute(const FVoxelQuery& Query) const
{
	const TValue<FVoxelFoliageChunkData> ChunkData = GetNodeRuntime().Get(ChunkDataPin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, ChunkData)
	{
		if (ChunkData->Data.Num() == 0 ||
			ChunkData->InstancesCount == 0)
		{
			return {};
		}

		const TSharedRef<FVoxelChunkExecObject_CreateFoliageCollision> Object = MakeShared<FVoxelChunkExecObject_CreateFoliageCollision>();
		Object->Position = ChunkData->ChunkPosition;

		for (const TSharedPtr<FVoxelFoliageChunkMeshData>& MeshData : ChunkData->Data)
		{
			if (MeshData->Transforms->Num() == 0 ||
				MeshData->BodyInstance.GetCollisionEnabled() == ECollisionEnabled::NoCollision)
			{
				continue;
			}

			const TSharedRef<FVoxelFoliageData> FoliageData = MakeShared<FVoxelFoliageData>();
			FoliageData->Transforms = MeshData->Transforms;

			FoliageData->UpdateStats();

			Object->TemplatesData.Add({
				MeshData->StaticMesh,
				MeshData->BodyInstance,
				FoliageData
			});
		}

		if (Object->TemplatesData.Num() == 0)
		{
			return {};
		}

		return Object;
	};
}