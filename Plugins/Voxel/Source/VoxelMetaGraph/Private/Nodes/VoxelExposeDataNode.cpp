// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelExposeDataNode.h"
#include "Nodes/VoxelPositionNodes.h"

DEFINE_VOXEL_SUBSYSTEM(FVoxelExposedDataSubsystem);

FVoxelPinType FVoxelExposedDataSubsystem::GetType(FName Name) const
{
	VOXEL_FUNCTION_COUNTER();

	CriticalSection.Lock();
	const TSharedPtr<FData> Data = Datas.FindRef(Name);
	CriticalSection.Unlock();

	if (!Data)
	{
		return {};
	}

	return Data->Node.GetPin(Data->Node.DataPin).GetType();
}

FVoxelFutureValue FVoxelExposedDataSubsystem::GetData(const FName Name, const FVoxelQuery& Query) const
{
	VOXEL_FUNCTION_COUNTER();

	CriticalSection.Lock();
	const TSharedPtr<FData> Data = Datas.FindRef(Name);
	CriticalSection.Unlock();

	if (!Data)
	{
		return {};
	}

	FVoxelQuery LocalQuery = Query;
	LocalQuery.SetDependenciesQueue(MakeShared<FVoxelQuery::FDependenciesQueue>());

	const FVoxelFutureValue Value = Data->Node.GetNodeRuntime().Get(Data->Node.DataPin, LocalQuery);
	if (!Value.IsValid())
	{
		return {};
	}

	if (Value.GetParentType().IsBuffer())
	{
		return FVoxelTask::New<FVoxelBufferView>(
			MakeShared<FVoxelTaskStat>(),
			"GetData",
			EVoxelTaskThread::AnyThread,
			{ Value },
			[=]
			{
				return Value.Get_CheckCompleted<FVoxelBuffer>().MakeGenericView();
			});
	}

	return Value;
}

FVoxelFutureValue FVoxelExposedDataSubsystem::GetData(const FName Name, const FVoxelQuery& Query, const FVector3f& Position) const
{
	FVoxelVectorBuffer Positions;
	Positions.X = FVoxelFloatBuffer::Constant(Position.X);
	Positions.Y = FVoxelFloatBuffer::Constant(Position.Y);
	Positions.Z = FVoxelFloatBuffer::Constant(Position.Z);

	FVoxelQuery LocalQuery = Query;
	LocalQuery.Add<FVoxelSparsePositionQueryData>().Initialize(Positions);
	return GetData(Name, LocalQuery);
}

void FVoxelExposedDataSubsystem::RegisterNode(const FName Name, const FVoxelNode_ExposeData& Node)
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelScopeLock Lock(CriticalSection);

	if (const TSharedPtr<FData> Data = Datas.FindRef(Name))
	{
		VOXEL_MESSAGE(Error, "Data {0} is exposed multiple times: {1}, {2}", Name, Data->Node, Node);
		return;
	}

	Datas.Add(Name, MakeShared<FData>(Node, Node.GetOuter()));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNode_ExposeData::Initialize()
{
	Super::Initialize();
	ensure(IsInGameThread());

	FVoxelQuery Query;
	Query.Callstack.Add(this);

	const TValue<FName> Name = GetNodeRuntime().Get(NamePin, Query);

	FVoxelTask::New(
		MakeShared<FVoxelTaskStat>(*this),
		"ExposeData",
		EVoxelTaskThread::AnyThread,
		{ Name },
		[=]
		{
			GetNodeRuntime().GetSubsystem<FVoxelExposedDataSubsystem>().RegisterNode(Name.Get_CheckCompleted(), *this);
		});
}

FVoxelPinTypeSet FVoxelNode_ExposeData::GetPromotionTypes(const FVoxelPin& Pin) const
{
	return FVoxelPinTypeSet::All();
}

void FVoxelNode_ExposeData::PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	Pin.SetType(NewType);
}