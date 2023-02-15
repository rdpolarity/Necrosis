// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelSpawnChunksByRange.h"

TVoxelFutureValue<FVoxelExecObject> FVoxelExecNode_SpawnChunksByRange::Execute(const FVoxelQuery& Query) const
{
	const TValue<float> ChunkSize = GetNodeRuntime().Get(ChunkSizePin, Query);
	const TValue<int32> RenderDistanceInChunks = GetNodeRuntime().Get(RenderDistanceInChunksPin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, ChunkSize, RenderDistanceInChunks)
	{
		const TSharedRef<FVoxelExecObject_SpawnChunksByRange> Object = MakeShared<FVoxelExecObject_SpawnChunksByRange>();
		Object->Initialize(*this);
		Object->ChunkSize = ChunkSize;
		Object->RenderDistanceInChunks = FMath::Clamp(RenderDistanceInChunks, 1, 128);
		return Object;
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelExecObject_SpawnChunksByRange::Tick(FVoxelRuntime& Runtime)
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
		const FIntVector CameraChunkPosition = FVoxelUtilities::FloorToInt(CameraPosition / ChunkSize);

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

		TVoxelArray<FIntVector> ChunksToAdd;
		{
			VOXEL_SCOPE_COUNTER("Find ChunksToAdd");

			const uint64 RenderDistanceSquared = FMath::Square<uint64>(RenderDistanceInChunks);
			for (int32 Z = 0; Z < Size; Z++)
			{
				for (int32 Y = 0; Y < Size; Y++)
				{
					for (int32 X = 0; X < Size; X++)
					{
						const FIntVector NewPosition = FIntVector(X, Y, Z);
						const FIntVector OldPosition = NewPosition + NewVisibleChunks->Start - OldVisibleChunks->Start;
						const FIntVector ChunkPosition = NewVisibleChunks->Start + NewPosition;

						const uint64 DistanceSquared = FVoxelUtilities::SquaredSize(NewPosition - Size / 2);
						if (DistanceSquared > RenderDistanceSquared)
						{
							continue;
						}

						NewVisibleChunks->VisibleChunks[FVoxelUtilities::Get3DIndex(Size, NewPosition)] = true;

						if (OldPosition.X < 0 ||
							OldPosition.Y < 0 ||
							OldPosition.Z < 0 ||
							OldPosition.X >= Size ||
							OldPosition.Y >= Size ||
							OldPosition.Z >= Size ||
							!OldVisibleChunks->VisibleChunks[FVoxelUtilities::Get3DIndex(Size, OldPosition)])
						{
							ChunksToAdd.Add(ChunkPosition);
							continue;
						}

						// Chunk is still visible, clear it to not be removed
						OldVisibleChunks->VisibleChunks[FVoxelUtilities::Get3DIndex(Size, OldPosition)] = false;
					}
				}
			}
		}
		
		TVoxelArray<FIntVector> ChunksToRemove;
		{
			VOXEL_SCOPE_COUNTER("Find ChunksToRemove");

			for (int32 Z = 0; Z < Size; Z++)
			{
				for (int32 Y = 0; Y < Size; Y++)
				{
					for (int32 X = 0; X < Size; X++)
					{
						const FIntVector OldPosition = FIntVector(X, Y, Z);

						if (OldVisibleChunks->VisibleChunks[FVoxelUtilities::Get3DIndex(Size, OldPosition)])
						{
							ChunksToRemove.Add(OldVisibleChunks->Start + OldPosition);
						}
					}
				}
			}
		}
		
		FVoxelScopeLock Lock(CriticalSection);

		for (const FIntVector& ChunkKey : ChunksToAdd)
		{
			FVoxelQuery Query;
			Query.Add<FVoxelBoundsQueryData>().Bounds = FVoxelBox(FVector3d(ChunkKey) * ChunkSize, FVector3d(ChunkKey + 1) * ChunkSize);
			Query.Add<FVoxelLODQueryData>().LOD = 0;

			const TSharedRef<FVoxelChunkRef> ChunkRef = CreateChunk(Query);
			ChunkRef->Update();

			ensure(!Chunks.Contains(ChunkKey));
			Chunks.Add(ChunkKey, ChunkRef);
		}

		for (const FIntVector& ChunkKey : ChunksToRemove)
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