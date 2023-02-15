// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelTask.h"
#include "VoxelExecNode.h"
#include "VoxelMetaGraphGraph.h"
#include "VoxelMetaGraphNodeInterface.h"

VOXEL_CONSOLE_VARIABLE(
	VOXELMETAGRAPH_API, int32, GVoxelNumThreads, 2,
	"voxel.NumThreads",
	"The number of threads to use to process voxel tasks");

VOXEL_CONSOLE_VARIABLE(
	VOXELMETAGRAPH_API, bool, GVoxelHideTaskCount, false,
	"voxel.HideTaskCount",
	"");

VOXEL_CONSOLE_VARIABLE(
	VOXELMETAGRAPH_API, int32, GVoxelThreadingThreadPriority, 2,
	"voxel.threading.ThreadPriority",
	"0: Normal"
	"1: AboveNormal"
	"2: BelowNormal"
	"3: Highest"
	"4: Lowest"
	"5: SlightlyBelowNormal"
	"6: TimeCritical");

VOXEL_CONSOLE_VARIABLE(
	VOXELMETAGRAPH_API, float, GVoxelThreadingPriorityDuration, 0.5f,
	"voxel.threading.PriorityDuration",
	"Task priorities will be recomputed with the new camera position every PriorityDuration seconds");

VOXEL_CONSOLE_VARIABLE(
	VOXELMETAGRAPH_API, int32, GVoxelThreadingMaxConcurrentChunkTasks, 64,
	"voxel.threading.MaxConcurrentChunkTasks",
	"");

VOXEL_CONSOLE_VARIABLE(
	VOXELMETAGRAPH_API, int32, GVoxelThreadingMaxConcurrentRenderTasks, 64,
	"voxel.threading.MaxConcurrentRenderTasks",
	"");

FVoxelTaskProcessor* GVoxelTaskProcessor = nullptr;

VOXEL_RUN_ON_STARTUP_GAME(CreateGVoxelTaskProcessor)
{
	GVoxelTaskProcessor = new FVoxelTaskProcessor();

	FVoxelRenderUtilities::OnPreRender().AddLambda([](FRDGBuilder& GraphBuilder)
	{
		GVoxelTaskProcessor->Tick_RenderThread(GraphBuilder);
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelChunkTask::Execute()
{
	if (*IsCancelled)
	{
		GVoxelTaskProcessor->OnChunkTaskDone(AsShared());
		return;
	}
	FVoxelTaskStatScope Scope(Stat);

	TArray<TVoxelFutureValue<FVoxelChunkExecObject>> ObjectFutures;
	for (const FVoxelNode* LinkedTo : Node.GetNodeRuntime().GetPinData(FVoxelPinRef(Pin)).Exec_LinkedTo)
	{
		const FVoxelChunkExecNode* NextNode = Cast<FVoxelChunkExecNode>(*LinkedTo);
		if (!ensure(NextNode))
		{
			continue;
		}

		NextNode->Execute(Query, ObjectFutures);
	}

	FVoxelTask::New(
		MakeShared<FVoxelTaskStat>(Node),
		TEXT("ChunkTask"),
		EVoxelTaskThread::AnyThread,
		ReinterpretCastArray<FVoxelFutureValue>(ObjectFutures),
		[This = AsShared(), ObjectFutures]
		{
			GVoxelTaskProcessor->OnChunkTaskDone(This);

			TArray<TSharedPtr<const FVoxelChunkExecObject>> Objects;
			for (const TVoxelFutureValue<FVoxelChunkExecObject>& ObjectFuture : ObjectFutures)
			{
				const TSharedRef<const FVoxelChunkExecObject> Object = ObjectFuture.GetShared_CheckCompleted();
				if (Object->GetStruct() == FVoxelChunkExecObject::StaticStruct())
				{
					continue;
				}

				Objects.Add(Object);
			}

			This->OnComplete(Objects);
		});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint32 GVoxelTaskStatTLS = FPlatformTLS::AllocTlsSlot();

bool FVoxelTaskStat::bStaticRecordStats = false;
bool FVoxelTaskStat::bStaticDetailedStats = false;
bool FVoxelTaskStat::bStaticInclusiveStats = false;

FVoxelCriticalSection FVoxelTaskStat::FData::CriticalSection;

FVoxelTaskStat::FVoxelTaskStat()
{
	if (!Data)
	{
		return;
	}

	if (const TSharedPtr<FVoxelTaskStat> Stat = GetScopeStat())
	{
		Data->Node = Stat->Data->Node;
		Data->NodeOuter = Stat->Data->NodeOuter;

		VOXEL_SCOPE_LOCK(FData::CriticalSection);
		Data->Parents_RequiresLock.Add(Stat->Data);
	}
}

FVoxelTaskStat::FVoxelTaskStat(const FVoxelNode& Node)
{
	if (!Data)
	{
		return;
	}

	Data->Node = &Node;
	Data->NodeOuter = Node.GetOuter();

	if (const TSharedPtr<FVoxelTaskStat> Stat = GetScopeStat())
	{
		VOXEL_SCOPE_LOCK(FData::CriticalSection);
		Data->Parents_RequiresLock.Add(Stat->Data);
	}
}

FVoxelTaskStat::~FVoxelTaskStat()
{
	if (!Data)
	{
		return;
	}

	AsyncTask(ENamedThreads::GameThread, [Data = Data]
	{
		WriteStats(Data);
	});
}

TSharedPtr<FVoxelTaskStat> FVoxelTaskStat::GetScopeStat()
{
	FVoxelTaskStat* Stat = static_cast<FVoxelTaskStat*>(FPlatformTLS::GetTlsValue(GVoxelTaskStatTLS));
	if (!Stat)
	{
		return nullptr;
	}
	return Stat->AsShared();
}

void FVoxelTaskStat::ClearStats()
{
#if WITH_EDITOR
	VOXEL_FUNCTION_COUNTER();

	for (UEdGraphNode* Node : TObjectRange<UEdGraphNode>())
	{
		if (IVoxelMetaGraphNodeInterface* Interface = Cast<IVoxelMetaGraphNodeInterface>(Node))
		{
			Interface->Times = IVoxelMetaGraphNodeInterface::FTime();
		}
	}
#endif
}

void FVoxelTaskStat::WriteStats(const TSharedPtr<FData>& Data)
{
	VOXEL_FUNCTION_COUNTER();
	ensure(IsInGameThread());

	TVoxelStaticArray<double, Count> SelfTime{ ForceInit };
	for (int32 Index = 0; Index < Count; Index++)
	{
		SelfTime[Index] = Data->SelfTimeNanoseconds[Index].GetValue() / 1.e9;
	}

	SelfTime[CpuRenderThread] -= SelfTime[CopyCpuToGpu];
	ensure(SelfTime[CpuRenderThread] >= 0);

	INLINE_LAMBDA
	{
		if (!Data->Node)
		{
			return;
		}

		const Voxel::MetaGraph::FNode* SourceNode = Data->Node->GetNodeRuntime().GetSourceNode();
		if (!ensure(SourceNode))
		{
			return;
		}

		SourceNode->Source.ForeachGraphNode([&](const TWeakObjectPtr<UEdGraphNode> GraphNode, const float Strength)
		{
			if (!GraphNode.IsValid())
			{
				ensure(GExitPurge);
				return;
			}

			IVoxelMetaGraphNodeInterface* Interface = Cast<IVoxelMetaGraphNodeInterface>(GraphNode.Get());
			if (!Interface)
			{
				return;
			}

			for (int32 Index = 0; Index < Count; Index++)
			{
				Interface->Times[Index].Exclusive += SelfTime[Index] * Strength;
			}
		});
	};

	TSet<TSharedPtr<FData>> Parents;
	{
		VOXEL_SCOPE_LOCK(FData::CriticalSection);

		const TFunction<void(const TSharedPtr<FData>&)> AddParents = [&](const TSharedPtr<FData>& InData)
		{
			if (Parents.Contains(InData))
			{
				return;
			}
			Parents.Add(InData);

			for (const TSharedPtr<FData>& Parent : InData->Parents_RequiresLock)
			{
				AddParents(Parent);
			}
		};

		AddParents(Data);
	}

	for (const TSharedPtr<FData>& Parent : Parents)
	{
		if (!Parent->Node)
		{
			continue;
		}

		const Voxel::MetaGraph::FNode* SourceNode = Parent->Node->GetNodeRuntime().GetSourceNode();
		if (!ensure(SourceNode))
		{
			continue;
		}

		SourceNode->Source.ForeachGraphNode([&](const TWeakObjectPtr<UEdGraphNode> GraphNode, const float Strength)
		{
			if (!GraphNode.IsValid())
			{
				ensure(GExitPurge);
				return;
			}

			IVoxelMetaGraphNodeInterface* Interface = Cast<IVoxelMetaGraphNodeInterface>(GraphNode.Get());
			if (!Interface)
			{
				return;
			}

			for (int32 Index = 0; Index < Count; Index++)
			{
				Interface->Times[Index].Inclusive += SelfTime[Index] * Strength;
			}
		});
	}
}

FVoxelTaskStatScope::FVoxelTaskStatScope(const TSharedPtr<FVoxelTaskStat>& Stat)
	: Stat(Stat)
{
	if (!Stat ||
		!Stat->IsRecording())
	{
		return;
	}

	OldStat = FVoxelTaskStat::GetScopeStat();

	if (OldStat)
	{
		VOXEL_SCOPE_LOCK(FVoxelTaskStat::FData::CriticalSection);
		Stat->Data->Parents_RequiresLock.Add(OldStat->Data);
	}

	FPlatformTLS::SetTlsValue(GVoxelTaskStatTLS, Stat.Get());
}

FVoxelTaskStatScope::~FVoxelTaskStatScope()
{
	if (!Stat ||
		!Stat->IsRecording())
	{
		return;
	}

	FPlatformTLS::SetTlsValue(GVoxelTaskStatTLS, OldStat.Get());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelTask::~FVoxelTask()
{
	check(LambdaCalled || GVoxelTaskProcessor->IsExiting());
}

void FVoxelTask::Execute() const
{
	const EVoxelTaskThread CurrentThread =
		IsInGameThread()
		? EVoxelTaskThread::GameThread
		: IsInRenderingThread()
		? EVoxelTaskThread::RenderThread
		: EVoxelTaskThread::AsyncThread;

	ensure(Thread == EVoxelTaskThread::AnyThread || Thread == CurrentThread);
	ensure(NumDependencies.GetValue() == 0);

	static uint32 TlsSlot = FPlatformTLS::AllocTlsSlot();

	double* OldChildTime = nullptr;
	double ChildTime = 0;
	if (Stat->IsRecording())
	{
		OldChildTime = static_cast<double*>(FPlatformTLS::GetTlsValue(TlsSlot));
		FPlatformTLS::SetTlsValue(TlsSlot, &ChildTime);
	}

	const double StartTime = FPlatformTime::Seconds();
	{
		FVoxelTaskStatScope Scope(Stat);
		VOXEL_SCOPE_COUNTER_STRING(StatName.ToString());
		check(!LambdaCalled);
		Lambda();
		check(!LambdaCalled);
		LambdaCalled = true;
	}
	const double EndTime = FPlatformTime::Seconds();

	if (!Stat->IsRecording())
	{
		return;
	}

	FPlatformTLS::SetTlsValue(TlsSlot, OldChildTime);

	double Time = EndTime - StartTime;
	if (OldChildTime)
	{
		*OldChildTime += Time;
	}
	Time -= ChildTime;
	ensure(Time >= 0);

	switch (CurrentThread)
	{
	default: VOXEL_ASSUME(false);
	case EVoxelTaskThread::AsyncThread: Stat->AddTime(FVoxelTaskStat::CpuAsyncThread, Time); break;
	case EVoxelTaskThread::GameThread: Stat->AddTime(FVoxelTaskStat::CpuGameThread, Time); break;
	case EVoxelTaskThread::RenderThread: Stat->AddTime(FVoxelTaskStat::CpuRenderThread, Time); break;
	}
}

void FVoxelTask::OnDependencyComplete()
{
	const int32 NumDependenciesLeft = NumDependencies.Decrement();
	check(NumDependenciesLeft >= 0);
	if (NumDependenciesLeft == 0)
	{
		GVoxelTaskProcessor->ProcessTask(AsShared());
	}
}

void FVoxelTask::New(
	const TSharedRef<FVoxelTaskStat>& Stat, 
	const FName StatName, 
	const EVoxelTaskThread Thread, 
	const TConstArrayView<FVoxelFutureValue> Dependencies, 
	TVoxelFunction<void()>&& Lambda)
{
	const TSharedRef<FVoxelTask> Task = MakeShareable(new FVoxelTask(
		Stat,
		StatName,
		Thread,
		MoveTemp(Lambda)));

	if (Dependencies.Num() == 0)
	{
		GVoxelTaskProcessor->ProcessTask(Task);
	}
	else
	{
		Task->NumDependencies.Add(Dependencies.Num());

		for (const FVoxelFutureValue& Dependency : Dependencies)
		{
			Dependency.State->AddDependentTask(Task);
		}
	}
}

FVoxelFutureValue FVoxelTask::New(
	const TSharedRef<FVoxelTaskStat>& Stat,
	const FVoxelPinType& Type,
	const FName StatName,
	const EVoxelTaskThread Thread,
	const TConstArrayView<FVoxelFutureValue> Dependencies,
	TVoxelFunction<FVoxelFutureValue()>&& Lambda)
{
	check(Type.IsValid());
	ensure(!Type.HasTag());

	const TSharedRef<FVoxelFuturePinValueState> State = MakeShared<FVoxelFuturePinValueState>(Type);

	New(Stat, StatName, Thread, Dependencies, [State, Type, StatName, Lambda = MoveTemp(Lambda)]
	{
		(void)StatName;

		const FVoxelFutureValue Value = Lambda();

		if (!Value.IsValid())
		{
			State->SetValue(FVoxelSharedPinValue(Type));
			return;
		}

		if (Value.IsComplete())
		{
			FVoxelSharedPinValue FinalValue = Value.Get_CheckCompleted();
			if (!ensure(FinalValue.IsDerivedFrom(Type)))
			{
				FinalValue = FVoxelSharedPinValue(Type);
			}
			if (FinalValue.IsDerivedFrom<FVoxelBuffer>() &&
				!ensureMsgf(FinalValue.Get<FVoxelBuffer>().IsValid(), TEXT("Invalid buffer produced by %s"), *StatName.ToString()))
			{
				FinalValue = FVoxelSharedPinValue(Type);
			}

			State->SetValue(FinalValue);
			return;
		}

		Value.State->AddLinkedState(State);
	});

	return FVoxelFutureValue(State);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelTaskProcessor::FVoxelTaskProcessor()
{
	TFunction<void()> Callback = [=]
	{
		bIsExiting = true;

		{
			VOXEL_SCOPE_LOCK(CriticalSection);
			ChunkTasks_Heapified.Reset();
		}

		Threads.Reset();
	};

	FCoreDelegates::OnPreExit.AddLambda(Callback);
	FCoreDelegates::OnExit.AddLambda(Callback);
	GOnVoxelModuleUnloaded.AddLambda(Callback);
	FTaskGraphInterface::Get().AddShutdownCallback(Callback);
}

int32 FVoxelTaskProcessor::NumTasks()
{
	VOXEL_SCOPE_LOCK(CriticalSection);
	return ChunkTasks_Heapified.Num() + ChunkTasksInProgress.Num();
}

void FVoxelTaskProcessor::Tick()
{
	if (IsExiting())
	{
		return;
	}

	VOXEL_FUNCTION_COUNTER();

	{
		VOXEL_SCOPE_LOCK(CriticalSection);

		const int32 NumTasks = ChunkTasks_Heapified.Num() + ChunkTasksInProgress.Num();
		if (!GVoxelHideTaskCount && NumTasks > 0)
		{
			const FString Message = FString::Printf(TEXT("%d voxel tasks left using %d threads"), NumTasks, GVoxelNumThreads);
			GEngine->AddOnScreenDebugMessage(uint64(0x557D0C945D26), FApp::GetDeltaTime() * 1.5f, FColor::White, Message);
		}

		GVoxelNumThreads = FMath::Max(GVoxelNumThreads, 1);

		while (Threads.Num() < GVoxelNumThreads)
		{
			Threads.Add(MakeUnique<FThread>());
			Event.Trigger();
		}
		while (Threads.Num() > GVoxelNumThreads)
		{
			TUniquePtr<FThread> Thread = Threads.Pop(false);
			AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [Thread = MakeSharedCopy(MoveTemp(Thread))]
			{
				Thread->Reset();
			});
		}
	}

	VOXEL_SCOPE_COUNTER("ProcessTasks");

	TSharedPtr<FVoxelTask> Task;
	while (GameTasks.Dequeue(Task))
	{
		Task->Execute();
	}
}

DECLARE_GPU_STAT(FVoxelTaskProcessor);

void FVoxelTaskProcessor::Tick_RenderThread(FRDGBuilder& GraphBuilder)
{
	if (IsExiting())
	{
		return;
	}

	VOXEL_FUNCTION_COUNTER();
	RDG_GPU_STAT_SCOPE(GraphBuilder, FVoxelTaskProcessor);

	ON_SCOPE_EXIT
	{
		for (const TFunction<void(FRDGBuilder&)>& OnComplete : OnRenderThreadCompleteQueue)
		{
			OnComplete(GraphBuilder);
		}
		OnRenderThreadCompleteQueue.Reset();
	};
	
	int32 NumTasksExecuted = 0;

	const auto ProcessTask = [&](const TSharedPtr<FVoxelTask>& Task)
	{
		RDG_EVENT_SCOPE(GraphBuilder, "%s", *Task->StatName.ToString());

		static FDrawCallCategoryName DrawCallCategoryName;
		FRDGGPUStatScopeGuard Scope(GraphBuilder, *Task->StatName.ToString(), {}, nullptr, &DrawCallCategoryName.Counters);

		if (Task->Stat->IsRecording())
		{
			FVoxelShaderStatsScope::SetCallback([Stat = Task->Stat](int64 TimeInMicroSeconds, FName Name)
			{
				Stat->AddTime(
					Name == STATIC_FNAME("Readback")
					? FVoxelTaskStat::CopyGpuToCpu
					: FVoxelTaskStat::GpuCompute,
					TimeInMicroSeconds * 1000);
			});
		}

		Task->Execute();

		if (Task->Stat->IsRecording())
		{
			FVoxelShaderStatsScope::SetCallback(nullptr);
		}

		NumTasksExecuted++;
	};

	TVoxelArray<TSharedPtr<FVoxelTask>> QueuedTasks;
	{
		TSharedPtr<FVoxelTask> Task;
		while (RenderTasks.Dequeue(Task))
		{
			QueuedTasks.Add(Task);
		}
	}

	while (QueuedTasks.Num() > 0)
	{
		ProcessTask(QueuedTasks.Pop(false));

		TSharedPtr<FVoxelTask> DependentTask;
		while (RenderTasks.Dequeue(DependentTask))
		{
			ProcessTask(DependentTask);
		}

		if (NumTasksExecuted > GVoxelThreadingMaxConcurrentRenderTasks)
		{
			for (const TSharedPtr<FVoxelTask>& Task : QueuedTasks)
			{
				RenderTasks.Enqueue(Task);
			}
			break;
		}
	}
}

void FVoxelTaskProcessor::ProcessTask(const TSharedRef<FVoxelTask>& Task)
{
	if (IsExiting())
	{
		return;
	}

	check(Task->NumDependencies.GetValue() == 0);

	switch (Task->Thread)
	{
	default: VOXEL_ASSUME(false);
	case EVoxelTaskThread::AnyThread:
	{
		Task->Execute();
	}
	break;
	case EVoxelTaskThread::GameThread:
	{
		GameTasks.Enqueue(Task);
	}
	break;
	case EVoxelTaskThread::RenderThread:
	{
		// Enqueue to ensure commands enqueued before this will be run before it
		ENQUEUE_RENDER_COMMAND(FVoxelTaskProcessor_ProcessTask)([=](FRHICommandList& RHICmdList)
		{
			RenderTasks.Enqueue(Task);
		});
	}
	break;
	case EVoxelTaskThread::AsyncThread:
	{
		AsyncTasks.Enqueue(Task);
		GVoxelTaskProcessor->Event.Trigger();
	}
	break;
	}
}

void FVoxelTaskProcessor::EnqueueChunkTasks(const TVoxelArray<TSharedPtr<FVoxelChunkTask>>& Tasks)
{
	if (IsExiting())
	{
		return;
	}

	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(CriticalSection);

	for (const TSharedPtr<FVoxelChunkTask>& Task : Tasks)
	{
		ChunkTasks_Heapified.HeapPush(FChunkTask(Task));
	}

	Event.Trigger();
}

void FVoxelTaskProcessor::OnChunkTaskDone(const TSharedRef<FVoxelChunkTask>& Task)
{
	if (IsExiting())
	{
		return;
	}

	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(CriticalSection);

	ensure(ChunkTasksInProgress.Remove(Task));
	Event.Trigger();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelTaskProcessor::FThread::FThread()
{
	UE::Trace::ThreadGroupBegin(TEXT("VoxelThreadPool"));

	static int32 ThreadIndex = 0;
	const FString Name = FString::Printf(TEXT("Voxel Thread %d"), ThreadIndex++);

	Thread = FRunnableThread::Create(
		this,
		*Name,
		1024 * 1024,
		EThreadPriority(FMath::Clamp(GVoxelThreadingThreadPriority, 0, 6)),
		FPlatformAffinity::GetPoolThreadMask());

	UE::Trace::ThreadGroupEnd();
}

FVoxelTaskProcessor::FThread::~FThread()
{
	VOXEL_FUNCTION_COUNTER();

	// Tell the thread it needs to die
	bTimeToDie = true;
	// Trigger the thread so that it will come out of the wait state if
	// it isn't actively doing work
	GVoxelTaskProcessor->Event.Trigger();
	// Kill (but wait for thread to finish)
	Thread->Kill(true);
	// Delete
	delete Thread;
}

uint32 FVoxelTaskProcessor::FThread::Run()
{
	VOXEL_LLM_SCOPE();
	
Wait:
	if (bTimeToDie)
	{
		return 0;
	}

	if (!GVoxelTaskProcessor->Event.Wait(10))
	{
		goto Wait;
	}

GetNextTask:
	if (bTimeToDie)
	{
		return 0;
	}

	if (const TSharedPtr<FVoxelChunkTask> Task = GVoxelTaskProcessor->GetNextChunkTask())
	{
		Task->Execute();
		goto GetNextTask;
	}

	if (const TSharedPtr<FVoxelTask> Task = GVoxelTaskProcessor->GetNextAsyncTask())
	{
		Task->Execute();
		goto GetNextTask;
	}

	goto Wait;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<FVoxelTask> FVoxelTaskProcessor::GetNextAsyncTask()
{
	// Lock required because queue is being dequeued on multiple threads
	VOXEL_SCOPE_LOCK(CriticalSection);

	TSharedPtr<FVoxelTask> Task;
	AsyncTasks.Dequeue(Task);
	return Task;
}

TSharedPtr<FVoxelChunkTask> FVoxelTaskProcessor::GetNextChunkTask()
{
	VOXEL_FUNCTION_COUNTER();

	VOXEL_SCOPE_LOCK(CriticalSection);

	if (ChunkTasksInProgress.Num() >= GVoxelThreadingMaxConcurrentChunkTasks)
	{
		return nullptr;
	}

	const double Time = FPlatformTime::Seconds();
	if (Time > LastDistanceComputeTime + GVoxelThreadingPriorityDuration)
	{
		LastDistanceComputeTime = Time;
		RecomputeDistances_AssumeLocked();
	}

	if (ChunkTasks_Heapified.Num() == 0)
	{
		return nullptr;
	}

#if VOXEL_DEBUG
	ChunkTasks_Heapified.VerifyHeap(TLess<FChunkTask>());
#endif

	VOXEL_SCOPE_COUNTER("HeapPop");

	FChunkTask ChunkTask;
	ChunkTasks_Heapified.HeapPop(ChunkTask, false);
	ChunkTasksInProgress.Add(ChunkTask.Task);

	return ChunkTask.Task;
}

void FVoxelTaskProcessor::RecomputeDistances_AssumeLocked()
{
	VOXEL_FUNCTION_COUNTER();
	checkVoxelSlow(CriticalSection.IsLocked_Debug());

	{
		VOXEL_SCOPE_COUNTER("Remove cancelled");

		ChunkTasks_Heapified.RemoveAllSwap([&](const FChunkTask& ChunkTask)
		{
			if (!*ChunkTask.Task->IsCancelled)
			{
				return false;
			}

			return true;
		});
	}

	ParallelFor(ChunkTasks_Heapified, [&](FChunkTask& ChunkTask)
	{
		ChunkTask.ComputeDistance();
	});

	VOXEL_SCOPE_COUNTER("Heapify");
	ChunkTasks_Heapified.Heapify();
}