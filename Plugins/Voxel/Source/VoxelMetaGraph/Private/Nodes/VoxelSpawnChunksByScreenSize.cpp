// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelSpawnChunksByScreenSize.h"

TVoxelFutureValue<FVoxelExecObject> FVoxelExecNode_SpawnChunksByScreenSize::Execute(const FVoxelQuery& Query) const
{
	const TValue<float> WorldSize = GetNodeRuntime().Get(WorldSizePin, Query);
	const TValue<float> ChunkSize = GetNodeRuntime().Get(ChunkSizePin, Query);
	const TValue<float> ChunkScreenSize = GetNodeRuntime().Get(ChunkScreenSizePin, Query);
	const TValue<int32> MaxChunks = GetNodeRuntime().Get(MaxChunksPin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, WorldSize, ChunkSize, ChunkScreenSize, MaxChunks)
	{
		const TSharedRef<FVoxelExecObject_SpawnChunksByScreenSize> Object = MakeShared<FVoxelExecObject_SpawnChunksByScreenSize>();
		Object->Initialize(*this);
		Object->WorldSize = FMath::Max(WorldSize, 2 * ChunkSize);
		Object->ChunkSize = ChunkSize;
		Object->ChunkScreenSize = ChunkScreenSize;
		Object->MaxChunks = MaxChunks;
		return Object;
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

BEGIN_VOXEL_NAMESPACE(SpawnChunksByScreenSize)

DEFINE_UNIQUE_VOXEL_ID(FChunkId);

END_VOXEL_NAMESPACE(SpawnChunksByScreenSize)

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelExecObject_SpawnChunksByScreenSize::Tick(FVoxelRuntime& Runtime)
{
	VOXEL_FUNCTION_COUNTER();

	Super::Tick(Runtime);

	if ((GVoxelChunkSpawnerFreeze && Octree) ||
		bTaskInProgress)
	{
		return;
	}
	
	FVector ViewOrigin;
	if (!FVoxelGameUtilities::GetCameraView(Runtime.GetWorld(), ViewOrigin))
	{
		return;
	}

	const FVector LocalViewOrigin = Runtime.WorldToLocal().TransformPosition(ViewOrigin);
	if (FVector::Distance(LocalViewOrigin, LastLocalViewOrigin) < GVoxelChunkSpawnerCameraRefreshThreshold)
	{
		return;
	}

	LastLocalViewOrigin = LocalViewOrigin;
	UpdateTree(Runtime, LocalViewOrigin);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelExecObject_SpawnChunksByScreenSize::UpdateTree(FVoxelRuntime& Runtime, const FVector& LocalViewOrigin)
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_USE_NAMESPACE(SpawnChunksByScreenSize);

	const int32 SizeInChunks = FMath::CeilToInt(WorldSize / ChunkSize);
	if (SizeInChunks <= 0)
	{
		return;
	}

	const int32 OctreeDepth = FMath::CeilLogTwo(SizeInChunks);
	if (OctreeDepth >= 30)
	{
		return;
	}

	ensure(!bTaskInProgress);
	bTaskInProgress = true;

	Runtime.AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, MakeWeakPtrLambda(this, [=, &Runtime, OldTree = Octree]
	{
		const TSharedRef<FOctree> NewTree = MakeShared<FOctree>(
			OctreeDepth,
			LocalViewOrigin,
			*this);

		if (OldTree)
		{
			VOXEL_SCOPE_COUNTER("CopyFrom");
			NewTree->CopyFrom(*OldTree);
		}
		
		TMap<FChunkId, FChunkInfo> ChunkInfos;
		TSet<FChunkId> ChunksToAdd;
		TSet<FChunkId> ChunksToRemove;
		TSet<FChunkId> ChunksToUpdate;
		NewTree->Update(ChunkInfos, ChunksToAdd, ChunksToRemove, ChunksToUpdate);

		{
			VOXEL_SCOPE_COUNTER("Sort");

			// Make sure that LOD 0 chunks are processed first
			ChunksToAdd.Sort([&](const FChunkId A, const FChunkId B)
			{
				return ChunkInfos[A].LOD < ChunkInfos[B].LOD;
			});
		}
		
		FVoxelScopeLock Lock(CriticalSection);
	
		for (const FChunkId ChunkId : ChunksToUpdate)
		{
			FChunk& Chunk = *Chunks.FindChecked(ChunkId);
			const FChunkInfo ChunkInfo = ChunkInfos[ChunkId];

			if (ensure(Chunk.ChunkRef))
			{
				Chunk.ChunkRef->SetTransitionMask(ChunkInfo.TransitionMask);
			}
		}

		// Set old chunks PreviousChunks to be themselves
		for (const FChunkId ChunkId : ChunksToRemove)
		{
			const TSharedPtr<FChunk> Chunk = Chunks.FindRef(ChunkId);
			if (!ensure(Chunk))
			{
				continue;
			}

			if (ensure(Chunk->ChunkRef))
			{
				Chunk->ChunkRef->BeginDestroy();
			}

			const TSharedRef<FPreviousChunksLeaf> NewPreviousChunks = MakeShared<FPreviousChunksLeaf>();
			NewPreviousChunks->ChunkRef = Chunk->ChunkRef;
			Chunk->ChunkRef.Reset();

			if (Chunk->PreviousChunks)
			{
				NewPreviousChunks->Children.Add(MoveTemp(Chunk->PreviousChunks));
			}

			ensure(!Chunk->ChunkRef);
			ensure(!Chunk->PreviousChunks);

			Chunk->PreviousChunks = NewPreviousChunks;
		}

		for (const FChunkId ChunkId : ChunksToAdd)
		{
			const FChunkInfo ChunkInfo = ChunkInfos[ChunkId];

			ensure(!Chunks.Contains(ChunkId));
			const TSharedPtr<FChunk> Chunk = Chunks.Add(ChunkId, MakeShared<FChunk>());

			FVoxelQuery Query;
			Query.Add<FVoxelBoundsQueryData>().Bounds = ChunkInfo.ChunkBounds;
			Query.Add<FVoxelLODQueryData>().LOD = ChunkInfo.LOD;
			Chunk->ChunkRef = CreateChunk(Query);

			const TSharedRef<FPreviousChunks> PreviousChunks = MakeShared<FPreviousChunks>();
			if (OldTree)
			{
				OldTree->TraverseBounds(ChunkInfos[ChunkId].NodeBounds, [&](const FOctree::FNode Node)
				{
					const FNodeData& NodeData = OldTree->GetNodeData(Node);
					if (!NodeData.bIsRendered ||
						!ChunksToRemove.Contains(NodeData.ChunkId))
					{
						return;
					}

					checkVoxelSlow(!OldTree->HasChildren(Node));

					const TSharedPtr<FChunk> OldChunk = Chunks.FindRef(NodeData.ChunkId);
					if (!ensure(OldChunk))
					{
						return;
					}

					ensure(!OldChunk->ChunkRef);
					ensure(OldChunk->PreviousChunks);
					PreviousChunks->Children.Add(OldChunk->PreviousChunks);
				});
			}

			if (PreviousChunks->Children.Num() > 0)
			{
				Chunk->PreviousChunks = PreviousChunks;
			}

			Chunk->ChunkRef->Update(MakeWeakPtrLambda(this, [this, ChunkId]
			{
				FVoxelScopeLock OnCompleteLock(CriticalSection);
				if (!Chunks.Contains(ChunkId))
				{
					return;
				}

				Chunks[ChunkId]->PreviousChunks.Reset();
			}));
			Chunk->ChunkRef->SetTransitionMask(ChunkInfo.TransitionMask);
		}

		for (const FChunkId ChunkId : ChunksToRemove)
		{
			TSharedPtr<FChunk> Chunk;
			if (!ensure(Chunks.RemoveAndCopyValue(ChunkId, Chunk)))
			{
				continue;
			}

			ensure(!Chunk->ChunkRef);
		}
		
		Runtime.AsyncTask(ENamedThreads::GameThread, MakeWeakPtrLambda(this, [=]
		{
			VOXEL_FUNCTION_COUNTER();
			check(IsInGameThread());
			
			ensure(bTaskInProgress);
			bTaskInProgress = false;

			if (NewTree->NumNodes() >= MaxChunks)
			{
				VOXEL_MESSAGE(Error, "{0}: MaxChunks reached", GetNode());
			}

			Octree = NewTree;
		}));
	}));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

BEGIN_VOXEL_NAMESPACE(SpawnChunksByScreenSize)

void FOctree::Update(
	TMap<FChunkId, FChunkInfo>& ChunkInfos,
	TSet<FChunkId>& ChunksToAdd,
	TSet<FChunkId>& ChunksToRemove,
	TSet<FChunkId>& ChunksToUpdate)
{
	VOXEL_FUNCTION_COUNTER();

	const auto ShowNode = [&](const FNode& Node)
	{
		FNodeData& NodeData = GetNodeData(Node);
		if (NodeData.bIsRendered)
		{
			return;
		}
		NodeData.bIsRendered = true;

		ChunkInfos.Add(NodeData.ChunkId,
		{
			GetChunkBounds(Node),
			GetHeight(Node),
			0,
			GetNodeBounds(Node)
		});
		ChunksToAdd.Add(NodeData.ChunkId);
	};

	const auto HideNode = [&](const FNode& Node)
	{
		FNodeData& NodeData = GetNodeData(Node);
		if (!NodeData.bIsRendered)
		{
			return;
		}
		NodeData.bIsRendered = false;
		
		ChunkInfos.Add(NodeData.ChunkId,
		{
			GetChunkBounds(Node),
			GetHeight(Node),
			0,
			GetNodeBounds(Node)
		});
		ChunksToRemove.Add(NodeData.ChunkId);
	};

	Traverse([&](const FNode& Node)
	{
		// Always create root to avoid zero-centered bounds
		if (Node == FNode::Root())
		{
			if (!HasChildren(Node))
			{
				CreateChildren(Node);
			}

			return true;
		}

		if (NumNodes() > Object.MaxChunks)
		{
			return false;
		}

		const FVoxelBox ChunkBounds = GetChunkBounds(Node);

		const double Distance = ChunkBounds.DistanceFromBoxToPoint(LocalViewOrigin);
		// Don't take the projection/FOV into account, as it leads to
		// unwanted/unstable results on different screen ratio or when zooming
		const double ScreenSize = ChunkBounds.Size().GetMax() / FMath::Max(1., Distance);

		if (ScreenSize > Object.ChunkScreenSize && GetHeight(Node) > 0)
		{
			if (!HasChildren(Node))
			{
				CreateChildren(Node);
			}
			HideNode(Node);

			return true;
		}

		if (HasChildren(Node))
		{
			TraverseChildren(Node, HideNode);
			DestroyChildren(Node);
		}

		ShowNode(Node);

		return false;
	});

	VOXEL_SCOPE_COUNTER("Update transitions");

	Traverse([&](const FNode& Node)
	{
		FNodeData& NodeData = GetNodeData(Node);
		if (!NodeData.bIsRendered)
		{
			return;
		}

		uint8 TransitionMask = 0;

		const int32 Height = GetHeight(Node);
		for (int32 Direction = 0; Direction < 6; Direction++)
		{
			if (AdjacentNodeHasHigherHeight(Node, Direction, Height))
			{
				TransitionMask |= (1 << Direction);
			}
		}

		if (TransitionMask == NodeData.TransitionMask)
		{
			return;
		}
		NodeData.TransitionMask = TransitionMask;

		if (ChunksToAdd.Contains(NodeData.ChunkId))
		{
			ChunkInfos[NodeData.ChunkId].TransitionMask = TransitionMask;
		}
		else
		{
			ChunksToUpdate.Add(NodeData.ChunkId);

			ensure(!ChunkInfos.Contains(NodeData.ChunkId));
			ChunkInfos.Add(NodeData.ChunkId,
			{
				GetChunkBounds(Node),
				GetHeight(Node),
				TransitionMask,
				GetNodeBounds(Node)
			});
		}
	});
}

FORCEINLINE bool FOctree::AdjacentNodeHasHigherHeight(FNode Node, int32 Direction, int32 Height) const
{
	const int32 NodeSize = GetNodeSize(Node);

	// Offsets that will put us in the neighboring nodes along the "depth" axis
	const int32 DepthOffset = 3 * NodeSize / 4;

	FIntVector Offset;
	switch (Direction)
	{
	default: VOXEL_ASSUME(false);
	case 0: Offset = FIntVector(-DepthOffset, 0, 0); break;
	case 1: Offset = FIntVector(+DepthOffset, 0, 0); break;
	case 2: Offset = FIntVector(0, -DepthOffset, 0); break;
	case 3: Offset = FIntVector(0, +DepthOffset, 0); break;
	case 4: Offset = FIntVector(0, 0, -DepthOffset); break;
	case 5: Offset = FIntVector(0, 0, +DepthOffset); break;
	}

	const FIntVector PositionToQuery = GetNodeCenter(Node) + Offset;
 
	if (!GetNodeBounds(FNode::Root()).Contains(PositionToQuery))
	{
		return {};
	}

	FNode Result = FNode::Root();

	while (
		Result.IsValid() &&
		HasChildren(Result) &&
		GetHeight(Result) > Height &&
		!GetNodeData(Result).bIsRendered)
	{
		Result = GetChild(Result, PositionToQuery);
	}

	checkVoxelSlow(!Result.IsValid() || GetNodeBounds(Result).Contains(PositionToQuery));

	return Result.IsValid() && GetHeight(Result) > Height;
}

END_VOXEL_NAMESPACE(SpawnChunksByScreenSize)