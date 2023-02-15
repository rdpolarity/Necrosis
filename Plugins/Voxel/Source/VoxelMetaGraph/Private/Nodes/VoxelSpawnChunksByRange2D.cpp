// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelSpawnChunksByRange2D.h"

TVoxelFutureValue<FVoxelExecObject> FVoxelExecNode_SpawnChunksByRange2D::Execute(const FVoxelQuery& Query) const
{
	const TValue<float> ChunkSize = GetNodeRuntime().Get(ChunkSizePin, Query);
	const TValue<int32> RenderDistanceInChunks = GetNodeRuntime().Get(RenderDistanceInChunksPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, ChunkSize, RenderDistanceInChunks)
	{
		const TSharedRef<FVoxelExecObject_SpawnChunksByRange2D> Object = MakeShared<FVoxelExecObject_SpawnChunksByRange2D>();
		Object->Initialize(*this);
		Object->ChunkSize = ChunkSize;
		Object->RenderDistanceInChunks = FMath::Clamp(RenderDistanceInChunks, 1, 128);
		return Object;
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelExecObject_SpawnChunksByRange2D::Tick(FVoxelRuntime& Runtime)
{
	VOXEL_FUNCTION_COUNTER();

	Super::Tick(Runtime);

	if ((GVoxelChunkSpawnerFreeze && VisibleChunks) ||
		bUpdateInProgress)
	{
		return;
	}

	FVector CameraPosition;
	if (!FVoxelGameUtilities::GetCameraView(Runtime.GetWorld(), CameraPosition))
	{
		return;
	}
	CameraPosition = Runtime.WorldToLocal().TransformPosition(CameraPosition);

	if (VisibleChunks.IsValid() &&
		FVoxelUtilities::Abs(CameraPosition - LastCameraPosition).GetMax() < GVoxelChunkSpawnerCameraRefreshThreshold)
	{
		return;
	}

	LastCameraPosition = CameraPosition;
	bUpdateInProgress = true;

	Runtime.AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, MakeWeakPtrLambda(this, [=, &Runtime]
	{
		VOXEL_SCOPE_COUNTER("Find chunks");

		const int32 Size = 2 * RenderDistanceInChunks;
		const FIntPoint CameraChunkPosition = FVoxelUtilities::FloorToInt(FVector2D(CameraPosition) / ChunkSize);

		const TSharedRef<FVisibleChunks> NewVisibleChunks = MakeShared<FVisibleChunks>();
		NewVisibleChunks->Start = CameraChunkPosition - Size / 2;
		NewVisibleChunks->Size = Size;
		NewVisibleChunks->VisibleChunks.SetNumZeroed(Size * Size * Size);

		TSharedPtr<FVisibleChunks> OldVisibleChunks;
		if (VisibleChunks)
		{
			OldVisibleChunks = MakeSharedCopy(*VisibleChunks);
		}
		else
		{
			OldVisibleChunks = MakeSharedCopy(*NewVisibleChunks);
		}
		check(OldVisibleChunks->Size == NewVisibleChunks->Size);

		TVoxelArray<FIntPoint> ChunksToAdd;
		{
			VOXEL_SCOPE_COUNTER("Find ChunksToAdd");

			const uint64 RenderDistanceSquared = FMath::Square<uint64>(RenderDistanceInChunks);
			for (int32 Y = 0; Y < Size; Y++)
			{
				for (int32 X = 0; X < Size; X++)
				{
					const FIntPoint NewPosition = FIntPoint(X, Y);
					const FIntPoint OldPosition = NewPosition + NewVisibleChunks->Start - OldVisibleChunks->Start;
					const FIntPoint ChunkPosition = NewVisibleChunks->Start + NewPosition;

					const uint64 DistanceSquared = FVoxelUtilities::SquaredSize(NewPosition - Size / 2);
					if (DistanceSquared > RenderDistanceSquared)
					{
						continue;
					}

					NewVisibleChunks->VisibleChunks[FVoxelUtilities::Get2DIndex(Size, NewPosition)] = true;

					if (OldPosition.X < 0 ||
						OldPosition.Y < 0 ||
						OldPosition.X >= Size ||
						OldPosition.Y >= Size ||
						!OldVisibleChunks->VisibleChunks[FVoxelUtilities::Get2DIndex(Size, OldPosition)])
					{
						ChunksToAdd.Add(ChunkPosition);
						continue;
					}

					// Chunk is still visible, clear it to not be removed
					OldVisibleChunks->VisibleChunks[FVoxelUtilities::Get2DIndex(Size, OldPosition)] = false;
				}
			}
		}
		
		TVoxelArray<FIntPoint> ChunksToRemove;
		{
			VOXEL_SCOPE_COUNTER("Find ChunksToRemove");

			for (int32 Y = 0; Y < Size; Y++)
			{
				for (int32 X = 0; X < Size; X++)
				{
					const FIntPoint OldPosition = FIntPoint(X, Y);

					if (OldVisibleChunks->VisibleChunks[FVoxelUtilities::Get2DIndex(Size, OldPosition)])
					{
						ChunksToRemove.Add(OldVisibleChunks->Start + OldPosition);
					}
				}
			}
		}
		
		FVoxelScopeLock Lock(CriticalSection);

		for (const FIntPoint& ChunkKey : ChunksToAdd)
		{
			FVoxelQuery Query;
			Query.Add<FVoxelBoundsQueryData>().Bounds = FVoxelBox(FVector3d(ChunkKey) * ChunkSize, FVector3d(ChunkKey + 1) * ChunkSize);
			Query.Add<FVoxelLODQueryData>().LOD = 0;

			const TSharedRef<FVoxelChunkRef> ChunkRef = CreateChunk(Query);
			ChunkRef->Update();

			ensure(!Chunks.Contains(ChunkKey));
			Chunks.Add(ChunkKey, ChunkRef);
		}

		for (const FIntPoint& ChunkKey : ChunksToRemove)
		{
			ensure(Chunks.Remove(ChunkKey));
		}

		Runtime.AsyncTask(ENamedThreads::GameThread, MakeWeakPtrLambda(this, [=]
		{
			VisibleChunks = NewVisibleChunks;

			ensure(bUpdateInProgress);
			bUpdateInProgress = false;
		}));
	}));
}