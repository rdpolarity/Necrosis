// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelChunkManager.h"
#include "VoxelExecNode.h"

VOXEL_CONSOLE_VARIABLE(
	VOXELMETAGRAPH_API, bool, GVoxelChunkSpawnerFreeze, false,
	"voxel.chunkspawner.Freeze",
	"");

VOXEL_CONSOLE_VARIABLE(
	VOXELMETAGRAPH_API, float, GVoxelChunkSpawnerCameraRefreshThreshold, 1.f,
	"voxel.chunkspawner.CameraRefreshThreshold",
	"");

DEFINE_UNIQUE_VOXEL_ID(FVoxelChunkId);

void FVoxelPendingChunksCounter::Decrement()
{
	const int32 NumChunksLeft = Counter.Decrement();
	if (NumChunksLeft > 0)
	{
		return;
	}

	const TSharedPtr<FVoxelChunkManager> PinnedChunkManager = ChunkManager.Pin();
	if (!PinnedChunkManager)
	{
		return;
	}

	const TSharedPtr<FVoxelExecObject> PinnedExecObject = PinnedChunkManager->WeakOwner.Pin();
	if (!PinnedExecObject)
	{
		return;
	}

	const TSharedPtr<FVoxelRuntime> PinnedRuntime = Runtime.Pin();
	if (!PinnedRuntime)
	{
		return;
	}

	PinnedExecObject->OnChunksComplete(*PinnedRuntime);
}

void FVoxelChunkManager::Create(FVoxelRuntime& Runtime, const TSharedPtr<FVoxelExecObject>& OwnerExecObject)
{
	FVoxelScopeLock Lock(FVoxelDependency::CriticalSection);
	FVoxelDependency::OnDependenciesInvalidated_RequiresLock.AddSP(this, &FVoxelChunkManager::InvalidateDependencies);

	WeakOwner = OwnerExecObject;
	PendingChunksCounter = MakeShared<FVoxelPendingChunksCounter>(AsShared(), Runtime.AsShared());
}

void FVoxelChunkManager::Destroy(FVoxelRuntime& Runtime)
{
	VOXEL_FUNCTION_COUNTER();

	{
		FVoxelScopeLock Lock(CriticalSection);
		for (const auto& It : ChunkInfos)
		{
			ActionQueue->Enqueue(FVoxelChunkAction{ EVoxelChunkAction::Destroy, It.Key });
		}
	}

	ProcessActions(Runtime);

	ensure(ChunkInfos.Num() == 0);
}

void FVoxelChunkManager::Tick(FVoxelRuntime& Runtime)
{
	VOXEL_FUNCTION_COUNTER();

	ProcessActions(Runtime);

	{
		VOXEL_SCOPE_COUNTER("Process OnComplete");

		TSharedPtr<FOnComplete> OnComplete;
		while (OnCompleteQueue.Dequeue(OnComplete))
		{
			for (auto It = OnComplete->ChunkIds.CreateIterator(); It; ++It)
			{
				const TSharedPtr<FChunkInfo> ChunkInfo = ChunkInfos.FindRef(*It);
				if (!ChunkInfo ||
					!ChunkInfo->Task_GameThread)
				{
					It.RemoveCurrent();
					continue;
				}

				ChunkInfo->OnComplete_GameThread.Add(OnComplete);
			}

			if (OnComplete->ChunkIds.Num() == 0)
			{
				(*OnComplete->OnComplete)();
			}
		}
	}

	{
		VOXEL_SCOPE_COUNTER("Process GameThreadTasks");

		TVoxelArray<TPair<FTaskCompletion, TSharedPtr<FChunkInfo>>> TaskCompletions;
		{
			VOXEL_SCOPE_LOCK(CriticalSection);

			FTaskCompletion TaskCompletion;
			while (TaskCompletionQueue.Dequeue(TaskCompletion))
			{
				const TSharedPtr<FChunkInfo> ChunkInfo = TaskCompletion.ChunkInfo;
				if (!ChunkInfos.Contains(ChunkInfo->ChunkId))
				{
					continue;
				}
				if (ChunkInfo->Task_GameThread.Get() != TaskCompletion.TaskPtr)
				{
					continue;
				}
				checkVoxelSlow(ChunkInfos[ChunkInfo->ChunkId] == ChunkInfo);

				TaskCompletions.Add({ MoveTemp(TaskCompletion), ChunkInfo });
			}
		}

		for (auto& It : TaskCompletions)
		{
			FTaskCompletion TaskCompletion = MoveTemp(It.Key);
			const TSharedPtr<FChunkInfo> ChunkInfo = It.Value;

			ChunkInfo->Task_GameThread.Reset();
			
			for (const TSharedPtr<const FVoxelChunkExecObject>& ChunkObject : ChunkInfo->ChunkObjects_GameThread)
			{
				ChunkObject->CallDestroy(Runtime);
			}

			ChunkInfo->ChunkObjects_GameThread = MoveTemp(TaskCompletion.ChunkObjects);

			for (const TSharedPtr<const FVoxelChunkExecObject>& ChunkObject : ChunkInfo->ChunkObjects_GameThread)
			{
				ChunkObject->CallCreate(Runtime);
			}

			ChunkInfo->Dependencies_GameThread = MoveTemp(TaskCompletion.Dependencies);

			{
				FVoxelScopeLock Lock(FVoxelDependency::CriticalSection);
				for (const TSharedPtr<FVoxelDependency>& Dependency : ChunkInfo->Dependencies_GameThread)
				{
					Dependency->Chunks_RequiresLock.FindOrAdd(AsShared()).Add(ChunkInfo->ChunkId);

					if (Dependency->IsInvalidated() &&
						!ChunkInfo->UpdateQueued)
					{
						ActionQueue->Enqueue(FVoxelChunkAction(EVoxelChunkAction::Update, ChunkInfo->ChunkId));
						ChunkInfo->UpdateQueued = true;
					}
				}
			}

			ChunkInfo->PendingChunks_GameThread.Reset();

			ChunkInfo->FlushOnComplete();
		}
	}

	// Process tasks queued by OnComplete
	ProcessActions(Runtime);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<FVoxelChunkRef> FVoxelChunkManager::CreateChunk(
	FName PinName,
	const FVoxelNode& Node,
	const FVoxelQuery& Query)
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedRef<FChunkInfo> ChunkInfo = MakeShared<FChunkInfo>(
		PinName,
		Node,
		Query,
		Node.GetOuter());

	ChunkInfo->PendingChunks_GameThread.Add(MakeShared<FVoxelPendingChunk>(PendingChunksCounter));

	FVoxelScopeLock Lock(CriticalSection);
	ChunkInfos.Add(ChunkInfo->ChunkId, ChunkInfo);

	return MakeShareable(new FVoxelChunkRef(ChunkInfo->ChunkId, ActionQueue));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelChunkManager::InvalidateDependencies(const TSet<TSharedPtr<FVoxelDependency>>& Dependencies)
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedRef<FOnComplete> OnComplete = MakeShared<FOnComplete>();
	{
		FVoxelScopeLock Lock(FVoxelDependency::CriticalSection);
		for (const TSharedPtr<FVoxelDependency>& Dependency : Dependencies)
		{
			TArray<FVoxelChunkId> ChunkIds;
			if (!Dependency->Chunks_RequiresLock.RemoveAndCopyValue(AsShared(), ChunkIds))
			{
				continue;
			}

			OnComplete->ChunkIds.Append(ChunkIds);
		}
	}

	{
		VOXEL_SCOPE_LOCK(CriticalSection);

		for (const FVoxelChunkId ChunkId : OnComplete->ChunkIds)
		{
			if (const TSharedPtr<FChunkInfo> ChunkInfo = ChunkInfos.FindRef(ChunkId))
			{
				ChunkInfo->UpdateQueued = true;
			}
		}
	}

	OnComplete->OnComplete = MakeShared<TFunction<void()>>([this, ChunksToUpdate = OnComplete->ChunkIds]
	{
		for (const FVoxelChunkId Chunk : ChunksToUpdate)
		{
			ActionQueue->Enqueue(FVoxelChunkAction(EVoxelChunkAction::Update, Chunk));
		}
	});

	OnCompleteQueue.Enqueue(OnComplete);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelChunkManager::FChunkInfo::~FChunkInfo()
{
	ensure(!Task_GameThread.IsValid());
	ensure(OnComplete_GameThread.Num() == 0);
	ensure(ChunkObjects_GameThread.Num() == 0);
}

void FVoxelChunkManager::FChunkInfo::FlushOnComplete()
{
	ensure(IsInGameThread());

	for (const TSharedPtr<FOnComplete>& OnComplete : OnComplete_GameThread)
	{
		ensure(OnComplete->ChunkIds.Remove(ChunkId));

		if (OnComplete->ChunkIds.Num() == 0)
		{
			(*OnComplete->OnComplete)();
		}
	}
	OnComplete_GameThread.Reset();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelChunkManager::ProcessActions(FVoxelRuntime& Runtime)
{
	VOXEL_FUNCTION_COUNTER();
	
	TVoxelArray<TSharedPtr<FVoxelChunkTask>> Tasks;
	TVoxelArray<TSharedPtr<FChunkInfo>> ChunksToFlush;
	{
		FVoxelScopeLock Lock(CriticalSection);

		FVoxelChunkAction Action;
		while (ActionQueue->Dequeue(Action))
		{
			ProcessAction(Runtime, Action, Tasks, ChunksToFlush);
		}
	}

	// Can't flush while CriticalSection is locked, will cause deadlocks
	for (const TSharedPtr<FChunkInfo>& ChunkToFlush : ChunksToFlush)
	{
		ChunkToFlush->FlushOnComplete();
	}

	GVoxelTaskProcessor->EnqueueChunkTasks(Tasks);
}

void FVoxelChunkManager::ProcessAction(
	FVoxelRuntime& Runtime,
	const FVoxelChunkAction& Action, 
	TVoxelArray<TSharedPtr<FVoxelChunkTask>>& OutTasks, 
	TVoxelArray<TSharedPtr<FChunkInfo>>& OutChunksToFlush)
{
	check(IsInGameThread());
	checkVoxelSlow(CriticalSection.IsLocked_Debug());

	const TSharedPtr<FChunkInfo> ChunkInfo = ChunkInfos.FindRef(Action.ChunkId);
	if (!ChunkInfo)
	{
		return;
	}

	switch (Action.Action)
	{
	default: ensure(false);
	case EVoxelChunkAction::Update:
	{
		const TSharedRef<FVoxelQuery::FDependenciesQueue> DependenciesQueue = MakeShared<FVoxelQuery::FDependenciesQueue>();

		FVoxelQuery Query = ChunkInfo->Query;
		Query.SetDependenciesQueue(DependenciesQueue);

		Query.Callstack.Add(&ChunkInfo->Node);

		const TSharedRef<FVoxelChunkTask> Task = MakeShared<FVoxelChunkTask>(
			ChunkInfo->Bounds,
			Query,
			ChunkInfo->Node,
			ChunkInfo->PinName,
			ChunkInfo->Stat,
			Runtime.AsShared(),
			ChunkInfo->NodeOuter);

		Task->OnComplete = MakeWeakPtrLambda(this, [TaskPtr = &Task.Get(), this, ChunkInfo, DependenciesQueue](const TArray<TSharedPtr<const FVoxelChunkExecObject>>& ChunkObjects)
		{
			TArray<TSharedPtr<FVoxelDependency>> Dependencies;
			{
				TSharedPtr<FVoxelDependency> Dependency;
				while (DependenciesQueue->Dequeue(Dependency))
				{
					Dependencies.Add(Dependency);
				}
			}

			TaskCompletionQueue.Enqueue({ TaskPtr, ChunkInfo, MoveTemp(Dependencies), ChunkObjects });
		});

		ChunkInfo->UpdateQueued = false;

		if (ChunkInfo->Task_GameThread)
		{
			*ChunkInfo->Task_GameThread->IsCancelled = true;
			ChunkInfo->Task_GameThread.Reset();
		}
		ChunkInfo->Task_GameThread = Task;

		if (Action.OnUpdateComplete)
		{
			ChunkInfo->OnComplete_GameThread.Add(MakeSharedCopy(FOnComplete
			{
				{ ChunkInfo->ChunkId },
				Action.OnUpdateComplete
			}));
		}

		OutTasks.Add(Task);
	}
	break;
	case EVoxelChunkAction::SetTransitionMask:
	{
		// TODO
		// Note: make sure to handle case where result is not ready yet
	}
	break;
	case EVoxelChunkAction::StartDithering:
	{
		// TODO
	}
	break;
	case EVoxelChunkAction::StopDithering:
	{
		// TODO
	}
	break;
	case EVoxelChunkAction::BeginDestroy:
	{
		ChunkInfo->Dependencies_GameThread.Reset();

		if (ChunkInfo->Task_GameThread)
		{
			*ChunkInfo->Task_GameThread->IsCancelled = true;
			ChunkInfo->Task_GameThread.Reset();
		}

		OutChunksToFlush.Add(ChunkInfo);
	}
	break;
	case EVoxelChunkAction::Destroy:
	{
		ChunkInfo->Dependencies_GameThread.Reset();

		if (ChunkInfo->Task_GameThread)
		{
			*ChunkInfo->Task_GameThread->IsCancelled = true;
			ChunkInfo->Task_GameThread.Reset();
		}

		OutChunksToFlush.Add(ChunkInfo);

		ChunkInfos.Remove(Action.ChunkId);

		for (const TSharedPtr<const FVoxelChunkExecObject>& ChunkObject : ChunkInfo->ChunkObjects_GameThread)
		{
			ChunkObject->CallDestroy(Runtime);
		}
		ChunkInfo->ChunkObjects_GameThread.Reset();
	}
	break;
	}
}