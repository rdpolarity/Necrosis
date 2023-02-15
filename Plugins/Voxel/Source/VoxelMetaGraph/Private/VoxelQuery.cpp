// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelQuery.h"

#if DO_CHECK
VOXEL_RUN_ON_STARTUP_GAME(CheckVoxelQueryData)
{
	for (UScriptStruct* Struct : GetDerivedStructs<FVoxelQueryData>())
	{
		if (Struct == FVoxelQueryData::StaticStruct())
		{
			continue;
		}

		FVoxelInstancedStruct Instance(Struct);

		FVoxelQueryData& QueryData = Instance.Get<FVoxelQueryData>();
		(void)QueryData.GetQueryTypeHash();
		ensureAlways(QueryData.IsQueryIdentical(QueryData));
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelCriticalSection FVoxelDependency::CriticalSection;
FVoxelDependency::FOnDependenciesInvalidated FVoxelDependency::OnDependenciesInvalidated_RequiresLock;

void FVoxelDependency::InvalidateDependencies(const TSet<TSharedPtr<FVoxelDependency>>& Dependencies)
{
	VOXEL_FUNCTION_COUNTER();

	for (const TSharedPtr<FVoxelDependency>& Dependency : Dependencies)
	{
		ensure(Dependency->Invalidated.Increment() == 1);
	}

	FOnDependenciesInvalidated OnDependenciesInvalidatedCopy;
	{
		FVoxelScopeLock Lock(CriticalSection);
		OnDependenciesInvalidatedCopy = OnDependenciesInvalidated_RequiresLock;
	}

	OnDependenciesInvalidatedCopy.Broadcast(Dependencies);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelQuery::Add(const TSharedRef<const FVoxelQueryData>& QueryData)
{
	for (
		const UScriptStruct* Struct = QueryData->GetStruct();
		Struct != FVoxelQueryData::StaticStruct();
		Struct = CastChecked<UScriptStruct>(Struct->GetSuperStruct()))
	{
		for (auto It = QueryDatas.CreateIterator(); It; ++It)
		{
			if (It.Key()->IsChildOf(Struct) ||
				Struct->IsChildOf(It.Key()))
			{
				It.RemoveCurrent();
			}
		}
	}

	for (
		UScriptStruct* Struct = QueryData->GetStruct();
		Struct != FVoxelQueryData::StaticStruct();
		Struct = CastChecked<UScriptStruct>(Struct->GetSuperStruct()))
	{
		checkVoxelSlow(!QueryDatas.Contains(Struct));
		QueryDatas.Add(Struct, QueryData);
	}
}

TSharedRef<FVoxelDependency> FVoxelQuery::AllocateDependency() const
{
	check(DependenciesQueue);

	const TSharedRef<FVoxelDependency> Dependency = MakeShared<FVoxelDependency>();
	DependenciesQueue->Enqueue(Dependency);
	return Dependency;
}