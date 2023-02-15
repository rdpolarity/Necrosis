// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelExecNode.h"
#include "VoxelChunkManager.h"

FVoxelExecObject::~FVoxelExecObject()
{
	ensure(!bIsCreated);
}

void FVoxelExecObject::Initialize(const FVoxelNode& Node)
{
	ensure(!bIsCreated);
	ensure(!PrivateNode);
	PrivateNode = &Node;
	
	for (const FVoxelPin& Pin : PrivateNode->GetPins())
	{
		if (Pin.GetType().Is<FVoxelChunkExec>())
		{
			ensure(ChunkExecPinName.IsNone());
			ChunkExecPinName = Pin.Name;
		}
		else if (Pin.GetType().Is<FVoxelExec>())
		{
			if (Pin.Name == "Then" ||
				Pin.Name == "Exec")
			{
				continue;
			}
			ensure(ChunkOnCompletePinName.IsNone());
			ChunkOnCompletePinName = Pin.Name;
		}
	}

	NodeOuter = PrivateNode->GetOuter();
	Stat = MakeShared<FVoxelTaskStat>(*PrivateNode);
}

void FVoxelExecObject::Create(FVoxelRuntime& Runtime)
{
	ensure(IsInGameThread());
	ensure(!bIsCreated);
	bIsCreated = true;

	ResultManager = MakeShared<FVoxelChunkManager>();
	ResultManager->Create(Runtime, AsShared());
}

void FVoxelExecObject::Destroy(FVoxelRuntime& Runtime)
{
	ensure(IsInGameThread());
	ensure(bIsCreated);
	bIsCreated = false;

	ResultManager->Destroy(Runtime);
}

void FVoxelExecObject::Tick(FVoxelRuntime& Runtime)
{
	ensure(IsInGameThread());
	ensure(bIsCreated);
	ResultManager->Tick(Runtime);
}

void FVoxelExecObject::OnChunksComplete(FVoxelRuntime& Runtime)
{
	if (ChunkOnCompletePinName.IsNone())
	{
		return;
	}

	TArray<TVoxelFutureValue<FVoxelExecObject>> OutObjects;
	for (const FVoxelNode* LinkedTo : PrivateNode->GetNodeRuntime().GetPinData(ChunkOnCompletePinName).Exec_LinkedTo)
	{
		const FVoxelExecNode* Node = Cast<FVoxelExecNode>(*LinkedTo);
		if (!ensure(Node))
		{
			continue;
		}

		Node->Execute(OutObjects);
	}

	if (OutObjects.Num() == 0)
	{
		return;
	}

	FVoxelTask::New(
		MakeShared<FVoxelTaskStat>(),
		"OnChunksComplete",
		EVoxelTaskThread::GameThread,
		ReinterpretCastArray<FVoxelFutureValue>(OutObjects),
		[this, WeakRuntime = MakeWeakPtr(Runtime.AsShared()), OutObjects]
		{
			ensure(IsInGameThread());

			const TSharedPtr<FVoxelRuntime> PinnedRuntime = WeakRuntime.Pin();
			if (!PinnedRuntime.IsValid())
			{
				return;
			}

			for (const TVoxelFutureValue<FVoxelExecObject>& ObjectFuture : OutObjects)
			{
				const TSharedRef<FVoxelExecObject> Object = ConstCastSharedRef<FVoxelExecObject>(ObjectFuture.GetShared_CheckCompleted());
				if (Object->GetStruct() == FVoxelExecObject::StaticStruct())
				{
					continue;
				}

				VOXEL_MESSAGE(Warning, "{0}: Cannot use Spawn Chunks nodes in an On Chunk Spawned event", PrivateNode);
			}
		});
}

TSharedRef<FVoxelChunkRef> FVoxelExecObject::CreateChunk(const FVoxelQuery& Query) const
{
	return ResultManager->CreateChunk(ChunkExecPinName, *PrivateNode, Query);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelChunkExecObject::~FVoxelChunkExecObject()
{
	ensure(!bIsCreated);
}

void FVoxelChunkExecObject::CallCreate(FVoxelRuntime& Runtime) const
{
	VOXEL_FUNCTION_COUNTER();
	ensure(!bIsCreated);
	ensure(IsInGameThread());

	Create(Runtime);

	bIsCreated = true;
}

void FVoxelChunkExecObject::CallDestroy(FVoxelRuntime& Runtime) const
{
	VOXEL_FUNCTION_COUNTER();
	ensure(bIsCreated);
	ensure(IsInGameThread());

	Destroy(Runtime);

	bIsCreated = false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelExecNode_Default::Execute(TArray<TValue<FVoxelExecObject>>& OutObjects) const
{
	FVoxelQuery Query;
	Query.Callstack.Add(this);
	OutObjects.Add(Execute(Query));

	for (const FVoxelNode* LinkedTo : GetNodeRuntime().GetPinData(ThenPin).Exec_LinkedTo)
	{
		const FVoxelExecNode* Node = Cast<FVoxelExecNode>(*LinkedTo);
		if (!ensure(Node))
		{
			continue;
		}

		Node->Execute(OutObjects);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelChunkExecNode_Default::Execute(const FVoxelQuery& Query, TArray<TValue<FVoxelChunkExecObject>>& OutObjects) const
{
	OutObjects.Add(Execute(Query));

	for (const FVoxelNode* LinkedTo : GetNodeRuntime().GetPinData(ThenPin).Exec_LinkedTo)
	{
		const FVoxelChunkExecNode* Node = Cast<FVoxelChunkExecNode>(*LinkedTo);
		if (!ensure(Node))
		{
			continue;
		}

		Node->Execute(Query, OutObjects);
	}
}