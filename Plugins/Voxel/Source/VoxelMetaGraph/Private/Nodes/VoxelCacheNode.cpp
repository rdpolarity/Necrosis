// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelCacheNode.h"

VOXEL_CONSOLE_VARIABLE(
	VOXELMETAGRAPH_API, int32, GVoxelMetaGraphMaxCacheNodeEntries, 1000,
	"voxel.metagraph.MaxCacheNodeEntries",
	"");

DEFINE_UNIQUE_VOXEL_ID(FVoxelCachedValueId);
DEFINE_VOXEL_SUBSYSTEM(FVoxelCacheNodeSubsystem);

void FVoxelCacheNodeSubsystem::Cleanup(const TSharedRef<FNodeCache>& NodeCache)
{
	VOXEL_FUNCTION_COUNTER();	

	TVoxelArray<TSharedPtr<FVoxelCachedValueRef>> ValuesToDelete;
	{
		ON_SCOPE_EXIT
		{
			VOXEL_SCOPE_COUNTER("Delete values");
			ValuesToDelete.Reset();
		};
	}

	FVoxelScopeLock Lock(CriticalSection);

	if (NodeCache->QueryCache.Num() > FMath::Max(GVoxelMetaGraphMaxCacheNodeEntries, 0))
	{
		using FPair = TPair<const FQueryCache::FHashedQuery*, TSharedPtr<FVoxelCachedValueRef>>;

		TVoxelArray<FPair> Pairs;
		Pairs.Reserve(NodeCache->QueryCache.Num());
		for (const auto& It : NodeCache->QueryCache)
		{
			if (!It.Value->Value)
			{
				continue;
			}

			Pairs.Add({ It.Key, It.Value });
		}

		{
			VOXEL_SCOPE_COUNTER("Sort");
			Pairs.Sort([](const FPair& A, const FPair& B)
			{
				return A.Value->LastAccessTime > B.Value->LastAccessTime;
			});
		}

		ValuesToDelete.Reserve(Pairs.Num());

		while (NodeCache->QueryCache.Num() > FMath::Max(GVoxelMetaGraphMaxCacheNodeEntries / 2, 0))
		{
			if (Pairs.Num() == 0)
			{
				break;
			}

			const FPair Pair = Pairs.Pop(false);
			ValuesToDelete.Add(Pair.Value);
			NodeCache->QueryCache.Remove(*Pair.Key);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_Cache, OutData)
{
	using FQueryCache = FVoxelCacheNodeSubsystem::FQueryCache;
	using FNodeCache = FVoxelCacheNodeSubsystem::FNodeCache;

	FVoxelCacheNodeSubsystem& Subsystem = GetSubsystem<FVoxelCacheNodeSubsystem>();

	const FQueryCache::FHashedQuery HashedQuery(Query);

	TSharedPtr<FNodeCache> NodeCache;
	TSharedPtr<FVoxelCachedValueRef> CachedValueRef;
	{
		FVoxelScopeLock Lock(Subsystem.CriticalSection);

		NodeCache = Subsystem.NodeCaches.FindOrAdd(this);
		if (!NodeCache)
		{
			NodeCache = Subsystem.NodeCaches.Add(this, MakeShared<FNodeCache>(GetOuter()));
		}

		CachedValueRef = NodeCache->QueryCache.FindRef(HashedQuery);
		if (!CachedValueRef)
		{
			CachedValueRef = NodeCache->QueryCache.Add(HashedQuery, MakeShared<FVoxelCachedValueRef>());
		}
	}

	TUniquePtr<TValue<FVoxelCachedValue>>& CachedValuePtr = CachedValueRef->Value;
	{
		TVoxelKeyedScopeLock<FVoxelCachedValueId> ValueLock(Subsystem.ValueCriticalSection, CachedValueRef->Id);

		bool bNeedRecompute = !CachedValuePtr;

		if (CachedValuePtr && 
			CachedValuePtr->IsComplete())
		{
			for (const TSharedPtr<FVoxelDependency>& Dependency : CachedValuePtr->Get_CheckCompleted().Dependencies)
			{
				if (Dependency->IsInvalidated())
				{
					bNeedRecompute = true;
				}
			}
		}

		if (bNeedRecompute)
		{
			const TSharedRef<FVoxelQuery::FDependenciesQueue> DependenciesQueue = MakeShared<FVoxelQuery::FDependenciesQueue>();

			FVoxelQuery ChildQuery = Query;
			ChildQuery.SetDependenciesQueue(DependenciesQueue);

			const FVoxelFutureValue PinValue = Get(DataPin, ChildQuery);

			CachedValuePtr = MakeUniqueCopy(VOXEL_ON_COMPLETE_CUSTOM(FVoxelCachedValue, "Cache", AnyThread, NodeCache, DependenciesQueue, PinValue)
			{
				FVoxelCachedValue Value;
				Value.Value = PinValue;

				TSharedPtr<FVoxelDependency> Dependency;
				while (DependenciesQueue->Dequeue(Dependency))
				{
					Value.Dependencies.Add(Dependency);
				}

				GetSubsystem<FVoxelCacheNodeSubsystem>().Cleanup(NodeCache.ToSharedRef());

				return Value;
			});
		}
	}
	const TValue<FVoxelCachedValue> CachedValue = *CachedValuePtr;

	CachedValueRef->LastAccessTime = FPlatformTime::Seconds();

	return VOXEL_ON_COMPLETE(AnyThread, CachedValue)
	{
		for (const TSharedPtr<FVoxelDependency>& Dependency : CachedValue->Dependencies)
		{
			Query.AddDependency(Dependency.ToSharedRef());
		}

		return CachedValue->Value;
	};
}

FVoxelPinTypeSet FVoxelNode_Cache::GetPromotionTypes(const FVoxelPin& Pin) const
{
	return FVoxelPinTypeSet::All();
}

void FVoxelNode_Cache::PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	GetPin(DataPin).SetType(NewType);
	GetPin(OutDataPin).SetType(NewType);
}