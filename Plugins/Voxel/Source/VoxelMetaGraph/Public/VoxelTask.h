// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelQuery.h"
#include "VoxelFutureValue.h"
#include "VoxelRuntime/VoxelRuntime.h"

class FVoxelTaskStat;
class FVoxelTaskProcessor;
struct FVoxelNode;
struct FVoxelPinRef;
struct FVoxelChunkExecObject;

BEGIN_VOXEL_NAMESPACE(MetaGraph)
DECLARE_VOXEL_SHADER_NAMESPACE(MetaGraph, "MetaGraph");
END_VOXEL_NAMESPACE(MetaGraph)

BEGIN_VOXEL_NAMESPACE(MetaGraph, Generated)
DECLARE_VOXEL_SHADER_NAMESPACE(MetaGraph_Generated, "MetaGraph/Generated");
END_VOXEL_NAMESPACE(MetaGraph, Generated)

class IVoxelNodeOuter
{
public:
	IVoxelNodeOuter() = default;
	virtual ~IVoxelNodeOuter() = default;
};

extern VOXELMETAGRAPH_API FVoxelTaskProcessor* GVoxelTaskProcessor;

struct VOXELMETAGRAPH_API FVoxelChunkTask : public TSharedFromThis<FVoxelChunkTask>
{
	const FVoxelBox Bounds;
	const FVoxelQuery Query;
	const FVoxelNode& Node;
	const FName Pin;
	const TSharedPtr<FVoxelTaskStat> Stat;
	const TSharedRef<FVoxelRuntime> Runtime;
	const TSharedRef<IVoxelNodeOuter> NodeOuter;
	const TSharedRef<FThreadSafeBool> IsCancelled = MakeShared<FThreadSafeBool>();

	TVoxelFunction<void(const TArray<TSharedPtr<const FVoxelChunkExecObject>>&)> OnComplete;

	FVoxelChunkTask(
		const FVoxelBox& Bounds,
		const FVoxelQuery& Query,
		const FVoxelNode& Node,
		const FName Pin,
		const TSharedPtr<FVoxelTaskStat>& Stat,
		const TSharedRef<FVoxelRuntime>& Runtime,
		const TSharedRef<IVoxelNodeOuter>& NodeOuter)
		: Bounds(Bounds)
		, Query(Query)
		, Node(Node)
		, Pin(Pin)
		, Stat(Stat)
		, Runtime(Runtime)
		, NodeOuter(NodeOuter)
	{
	}

	void Execute();
};

class VOXELMETAGRAPH_API FVoxelTaskStat : public TSharedFromThis<FVoxelTaskStat>
{
public:
	static bool bStaticRecordStats;
	static bool bStaticDetailedStats;
	static bool bStaticInclusiveStats;

	enum EType : int32
	{
		CopyCpuToGpu,
		CopyGpuToCpu,

		CpuGameThread,
		CpuRenderThread,
		CpuAsyncThread,

		GpuCompute,

		Count
	};
	struct FData
	{
		TVoxelStaticArray<FThreadSafeCounter64, Count> SelfTimeNanoseconds{ ForceInit };

		const FVoxelNode* Node = nullptr;
		TSharedPtr<IVoxelNodeOuter> NodeOuter;

		static FVoxelCriticalSection CriticalSection;
		TSet<TSharedPtr<FData>> Parents_RequiresLock;
	};
	const TSharedPtr<FData> Data = bStaticRecordStats ? MakeShared<FData>() : TSharedPtr<FData>();

	FVoxelTaskStat();
	explicit FVoxelTaskStat(const FVoxelNode& Node);
	~FVoxelTaskStat();

	void AddTime(const EType Type, const int64 Nanoseconds) const
	{
		ensure(Nanoseconds >= 0);
		Data->SelfTimeNanoseconds[Type].Add(Nanoseconds);
	}
	void AddTime(const EType Type, const double Seconds) const
	{
		AddTime(Type, FMath::CeilToInt64(FMath::Max(0, Seconds * 1e9)));
	}
	template<typename T>
	void AddTime(EType Type, T) const = delete;

	FORCEINLINE bool IsRecording() const
	{
		return Data.IsValid();
	}

public:
	static TSharedPtr<FVoxelTaskStat> GetScopeStat();
	static void ClearStats();
	static void WriteStats(const TSharedPtr<FData>& Data);
};

class VOXELMETAGRAPH_API FVoxelTaskStatScope
{
public:
	explicit FVoxelTaskStatScope(const TSharedPtr<FVoxelTaskStat>& Stat);
	~FVoxelTaskStatScope();

private:
	const TSharedPtr<FVoxelTaskStat> Stat;
	TSharedPtr<FVoxelTaskStat> OldStat;
};

class VOXELMETAGRAPH_API FVoxelTask : public TSharedFromThis<FVoxelTask>
{
public:
	const TSharedRef<FVoxelTaskStat> Stat;
	const FName StatName;
	const EVoxelTaskThread Thread;
	const TVoxelFunction<void()> Lambda;

	FThreadSafeCounter NumDependencies;

	~FVoxelTask();

	void Execute() const;
	void OnDependencyComplete();

	static void New(
		const TSharedRef<FVoxelTaskStat>& Stat,
		const FName StatName,
		EVoxelTaskThread Thread,
		TConstArrayView<FVoxelFutureValue> Dependencies,
		TVoxelFunction<void()>&& Lambda);

	static FVoxelFutureValue New(
		const TSharedRef<FVoxelTaskStat>& Stat,
		const FVoxelPinType& Type,
		const FName StatName,
		EVoxelTaskThread Thread,
		TConstArrayView<FVoxelFutureValue> Dependencies,
		TVoxelFunction<FVoxelFutureValue()>&& Lambda);

	template<typename T>
	static TVoxelFutureValue<T> New(
		const TSharedRef<FVoxelTaskStat>& Stat,
		const FName StatName,
		const EVoxelTaskThread Thread,
		const TConstArrayView<FVoxelFutureValue> Dependencies,
		TVoxelFunction<TVoxelFutureValue<T>()>&& Lambda)
	{
		return TVoxelFutureValue<T>(FVoxelTask::New(
			Stat,
			FVoxelPinType::Make<T>(),
			StatName,
			Thread,
			Dependencies,
			::MoveTemp(Lambda)));
	}

private:
	FVoxelTask(
		const TSharedRef<FVoxelTaskStat>& Stat,
		const FName StatName,
		const EVoxelTaskThread Thread,
		TVoxelFunction<void()>&& Lambda)
		: Stat(Stat)
		, StatName(StatName)
		, Thread(Thread)
		, Lambda(MoveTemp(Lambda))
	{
		ensure(!StatName.IsNone());
	}

	mutable FThreadSafeBool LambdaCalled;
};

extern VOXELMETAGRAPH_API int32 GVoxelNumThreads;

class VOXELMETAGRAPH_API FVoxelTaskProcessor : public FVoxelTicker
{
public:
	FVoxelTaskProcessor();

	int32 NumTasks();

	FORCEINLINE bool IsExiting() const
	{
		return bIsExiting;
	}

	//~ Begin FVoxelTicker Interface
	virtual void Tick() override;
	//~ End FVoxelTicker Interface

	void Tick_RenderThread(FRDGBuilder& GraphBuilder);

	void ProcessTask(const TSharedRef<FVoxelTask>& Task);
	void EnqueueChunkTasks(const TVoxelArray<TSharedPtr<FVoxelChunkTask>>& Tasks);
	void OnChunkTaskDone(const TSharedRef<FVoxelChunkTask>& Task);

	void AddOnRenderThreadComplete(TFunction<void(FRDGBuilder&)> OnComplete)
	{
		check(IsInRenderingThread());
		OnRenderThreadCompleteQueue.Add(MoveTemp(OnComplete));
	}

private:
	struct FChunkTask
	{
		double Distance = 0;
		TSharedPtr<FVoxelChunkTask> Task;

		FChunkTask() = default;
		explicit FChunkTask(const TSharedPtr<FVoxelChunkTask>& InTask)
			: Task(InTask)
		{
			ComputeDistance();
		}

		FORCEINLINE void ComputeDistance()
		{
			Distance = Task->Bounds.ComputeSquaredDistanceFromBoxToPoint(Task->Runtime->GetPriorityPosition());
		}

		FORCEINLINE bool operator<(const FChunkTask& Other) const
		{
			return Distance < Other.Distance;
		}
	};

	class FThread : public FRunnable
	{
	public:
		FThread();
		virtual ~FThread() override;

		//~ Begin FRunnable Interface
		virtual uint32 Run() override;
		//~ End FRunnable Interface

	private:
		FThreadSafeBool bTimeToDie = false;
		FRunnableThread* Thread = nullptr;
	};

	FEvent& Event = *FPlatformProcess::GetSynchEventFromPool();
	FThreadSafeBool bIsExiting = false;

	FVoxelCriticalSection CriticalSection;
	double LastDistanceComputeTime = 0;
	TVoxelArray<TUniquePtr<FThread>> Threads;
	TVoxelArray<FChunkTask> ChunkTasks_Heapified;
	TSet<TSharedPtr<FVoxelChunkTask>> ChunkTasksInProgress;

	// Not sorted by priority, only chunk tasks are
	TQueue<TSharedPtr<FVoxelTask>, EQueueMode::Mpsc> AsyncTasks;
	TQueue<TSharedPtr<FVoxelTask>, EQueueMode::Mpsc> GameTasks;
	TQueue<TSharedPtr<FVoxelTask>, EQueueMode::Mpsc> RenderTasks;

	TVoxelArray<TFunction<void(FRDGBuilder&)>> OnRenderThreadCompleteQueue;
	
	TSharedPtr<FVoxelTask> GetNextAsyncTask();
	TSharedPtr<FVoxelChunkTask> GetNextChunkTask();
	void RecomputeDistances_AssumeLocked();
};