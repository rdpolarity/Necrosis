// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelExecNode.h"

DECLARE_UNIQUE_VOXEL_ID(FVoxelChunkId);

extern VOXELMETAGRAPH_API bool GVoxelChunkSpawnerFreeze;
extern VOXELMETAGRAPH_API float GVoxelChunkSpawnerCameraRefreshThreshold;

enum class EVoxelChunkAction
{
	Update,
	SetTransitionMask,
	StartDithering,
	StopDithering,
	BeginDestroy,
	Destroy
};
struct FVoxelChunkAction
{
	EVoxelChunkAction Action = {};
	FVoxelChunkId ChunkId;

	uint8 TransitionMask = 0;
	TSharedPtr<const TFunction<void()>> OnUpdateComplete;

	FVoxelChunkAction() = default;
	FVoxelChunkAction(const EVoxelChunkAction Action, const FVoxelChunkId ChunkId)
		: Action(Action)
		, ChunkId(ChunkId)
	{
	}
};

using FVoxelChunkActionQueue = TQueue<FVoxelChunkAction, EQueueMode::Mpsc>;

struct VOXELMETAGRAPH_API FVoxelChunkRef
{
public:
	~FVoxelChunkRef()
	{
		Queue->Enqueue(FVoxelChunkAction(EVoxelChunkAction::Destroy, ChunkId));
	}
	
	void BeginDestroy() const
	{
		Queue->Enqueue(FVoxelChunkAction(EVoxelChunkAction::BeginDestroy, ChunkId));
	}
	void Update(TFunction<void()>&& OnUpdateComplete = nullptr) const
	{
		FVoxelChunkAction Action(EVoxelChunkAction::Update, ChunkId);
		if (OnUpdateComplete)
		{
			Action.OnUpdateComplete = MakeSharedCopy(MoveTemp(OnUpdateComplete));
		}
		Queue->Enqueue(Action);
	}
	void SetTransitionMask(uint8 TransitionMask) const
	{
		FVoxelChunkAction Action(EVoxelChunkAction::SetTransitionMask, ChunkId);
		Action.TransitionMask = TransitionMask;
		Queue->Enqueue(Action);
	}

	void StartDithering() const
	{
		Queue->Enqueue(FVoxelChunkAction(EVoxelChunkAction::StartDithering, ChunkId));
	}
	void StopDithering() const
	{
		Queue->Enqueue(FVoxelChunkAction(EVoxelChunkAction::StopDithering, ChunkId));
	}

private:
	FVoxelChunkRef(const FVoxelChunkId ChunkId, const TSharedRef<FVoxelChunkActionQueue>& Queue)
		: ChunkId(ChunkId)
		, Queue(Queue)
	{
	}

	const FVoxelChunkId ChunkId;
	const TSharedRef<FVoxelChunkActionQueue> Queue;

	friend class FVoxelChunkManager;
};

class FVoxelPendingChunksCounter
{
private:
	FThreadSafeCounter Counter;
	const TWeakPtr<FVoxelChunkManager> ChunkManager;
	const TWeakPtr<FVoxelRuntime> Runtime;

public:
	FVoxelPendingChunksCounter(const TSharedPtr<FVoxelChunkManager>& ChunkManager, const TSharedPtr<FVoxelRuntime>& Runtime)
		: ChunkManager(ChunkManager)
		, Runtime(Runtime)
	{
	}

private:
	void Increment()
	{
		Counter.Increment();
	}

	void Decrement();

	friend struct FVoxelPendingChunk;
};

struct FVoxelPendingChunk
{
private:
	const TSharedRef<FVoxelPendingChunksCounter> Counter;

public:
	explicit FVoxelPendingChunk(const TSharedPtr<FVoxelPendingChunksCounter>& Counter) : Counter(Counter.ToSharedRef())
	{
		Counter->Increment();
	}

	~FVoxelPendingChunk()
	{
		Counter->Decrement();
	}

	friend class FVoxelChunkManager;
};

class VOXELMETAGRAPH_API FVoxelChunkManager : public TSharedFromThis<FVoxelChunkManager>
{
public:
	void Create(FVoxelRuntime& Runtime, const TSharedPtr<FVoxelExecObject>& OwnerExecObject);
	void Destroy(FVoxelRuntime& Runtime);
	void Tick(FVoxelRuntime& Runtime);

public:
	TSharedRef<FVoxelChunkRef> CreateChunk(
		FName PinName,
		const FVoxelNode& Node,
		const FVoxelQuery& Query);
	
private:
	void InvalidateDependencies(const TSet<TSharedPtr<FVoxelDependency>>& Dependencies);

private:
	struct FOnComplete;
	struct FChunkInfo
	{
		const FVoxelChunkId ChunkId = FVoxelChunkId::New();

		const FName PinName;
		const FVoxelNode& Node;
		const FVoxelQuery Query;
		const TSharedPtr<FVoxelTaskStat> Stat;
		const TSharedRef<IVoxelNodeOuter> NodeOuter;
		const FVoxelBox Bounds;

		FThreadSafeBool UpdateQueued = false;
		TSharedPtr<FVoxelChunkTask> Task_GameThread;
		TArray<TSharedPtr<FOnComplete>> OnComplete_GameThread;
		TArray<TSharedPtr<FVoxelDependency>> Dependencies_GameThread;
		TArray<TSharedPtr<const FVoxelChunkExecObject>> ChunkObjects_GameThread;
		TArray<TSharedPtr<FVoxelPendingChunk>> PendingChunks_GameThread;

		FChunkInfo(
			FName PinName,
			const FVoxelNode& Node,
			const FVoxelQuery& Query,
			const TSharedRef<IVoxelNodeOuter>& NodeOuter)
			: PinName(PinName)
			, Node(Node)
			, Query(Query)
			, Stat(FVoxelTaskStat::GetScopeStat())
			, NodeOuter(NodeOuter)
			, Bounds(Query.Find<FVoxelBoundsQueryData>() ? Query.Find<FVoxelBoundsQueryData>()->Bounds : FVoxelBox())
		{
			ensure(Query.Find<FVoxelBoundsQueryData>());
		}
		~FChunkInfo();

		void FlushOnComplete();
	};

	const TSharedRef<FVoxelChunkActionQueue> ActionQueue = MakeShared<FVoxelChunkActionQueue>();

	struct FTaskCompletion
	{
		void* TaskPtr = nullptr;
		TSharedPtr<FChunkInfo> ChunkInfo;
		TArray<TSharedPtr<FVoxelDependency>> Dependencies;
		TArray<TSharedPtr<const FVoxelChunkExecObject>> ChunkObjects;
	};
	TQueue<FTaskCompletion, EQueueMode::Mpsc> TaskCompletionQueue;
	
	struct FOnComplete
	{
		TSet<FVoxelChunkId> ChunkIds;
		TSharedPtr<const TFunction<void()>> OnComplete;
	};
	TQueue<TSharedPtr<FOnComplete>, EQueueMode::Mpsc> OnCompleteQueue;

	FVoxelCriticalSection CriticalSection;
	TMap<FVoxelChunkId, TSharedPtr<FChunkInfo>> ChunkInfos;
	TSharedPtr<FVoxelPendingChunksCounter> PendingChunksCounter;
	TWeakPtr<FVoxelExecObject> WeakOwner;

	void ProcessActions(FVoxelRuntime& Runtime);
	void ProcessAction(
		FVoxelRuntime& Runtime,
		const FVoxelChunkAction& Action,
		TVoxelArray<TSharedPtr<FVoxelChunkTask>>& OutTasks,
		TVoxelArray<TSharedPtr<FChunkInfo>>& OutChunksToFlush);

	friend class FVoxelPendingChunksCounter;
};