// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelFutureValue.h"
#include "VoxelTask.h"

FVoxelFuturePinValueState::~FVoxelFuturePinValueState()
{
	if (GVoxelTaskProcessor->IsExiting())
	{
		return;
	}

	ensure(bIsComplete);
	ensure(DependentTasks.Num() == 0);
	ensure(LinkedStates.Num() == 0);
}

void FVoxelFuturePinValueState::AddDependentTask(const TSharedRef<FVoxelTask>& Task)
{
	if (bIsComplete)
	{
		Task->OnDependencyComplete();
		return;
	}

	VOXEL_SCOPE_LOCK(CriticalSection);

	if (bIsComplete)
	{
		Task->OnDependencyComplete();
		return;
	}

	DependentTasks.Add(Task);
}

void FVoxelFuturePinValueState::AddLinkedState(const TSharedRef<FVoxelFuturePinValueState>& State)
{
	check(Type.IsDerivedFrom(State->Type));

	if (bIsComplete)
	{
		State->SetValue(Value);
		return;
	}

	VOXEL_SCOPE_LOCK(CriticalSection);

	if (bIsComplete)
	{
		State->SetValue(Value);
		return;
	}

	LinkedStates.Add(State);
}

void FVoxelFuturePinValueState::SetValue(const FVoxelSharedPinValue& NewValue)
{
	check(!bIsComplete);
	check(!Value.IsValid());

	check(NewValue.IsValid());
	check(NewValue.IsDerivedFrom(Type));

	Value = NewValue;

	FVoxelScopeLockNoStats Lock(CriticalSection);

	ensure(!bIsComplete);
	bIsComplete = true;

	for (const TSharedPtr<FVoxelTask>& Task : DependentTasks)
	{
		Task->OnDependencyComplete();
	}
	DependentTasks.Empty();

	for (const TSharedPtr<FVoxelFuturePinValueState>& State : LinkedStates)
	{
		State->SetValue(Value);
	}
	LinkedStates.Empty();
}

void FVoxelFutureValue::OnComplete(const EVoxelTaskThread Thread, TVoxelFunction<void()>&& Lambda) const
{
	check(IsValid());

	FVoxelTask::New(
		MakeShared<FVoxelTaskStat>(),
		STATIC_FNAME("FVoxelFutureValue::OnComplete"),
		Thread,
		{ *this },
		MoveTemp(Lambda));
}