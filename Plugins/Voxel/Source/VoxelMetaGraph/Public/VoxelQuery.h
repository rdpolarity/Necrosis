// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelQuery.generated.h"

struct FVoxelNode;
class FVoxelChunkManager;

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelQueryData : public FVoxelVirtualStruct, public TSharedFromThis<FVoxelQueryData>
{
	GENERATED_BODY()
	DECLARE_VIRTUAL_STRUCT_PARENT(FVoxelQueryData, GENERATED_VOXEL_QUERY_DATA_BODY)

	virtual uint64 GetQueryTypeHash() const VOXEL_PURE_VIRTUAL({});
	virtual bool IsQueryIdentical(const FVoxelQueryData& Other) const VOXEL_PURE_VIRTUAL({});
};

#define GENERATED_VOXEL_QUERY_DATA_BODY() \
	GENERATED_VIRTUAL_STRUCT_BODY_IMPL(FVoxelQueryData) \
	virtual uint64 GetQueryTypeHash() const override \
	{ \
		checkVoxelSlow(GetStruct() == StaticStruct()); \
		return TVoxelQueryDataHelper<VOXEL_THIS_TYPE>::GetTypeHash(*this); \
	} \
	virtual bool IsQueryIdentical(const FVoxelQueryData& Other) const override \
	{ \
		checkVoxelSlow(GetStruct() == StaticStruct()); \
		if (GetStruct() != Other.GetStruct()) \
		{ \
			return false; \
		} \
		return TVoxelQueryDataHelper<VOXEL_THIS_TYPE>::Identical(*this, static_cast<const VOXEL_THIS_TYPE&>(Other)); \
	} \
	\
	VOXEL_ON_CONSTRUCT() \
	{ \
		TVoxelQueryDataHelper<VOXEL_THIS_TYPE>::Check(); \
	}; \
	\
	template<typename> \
	friend struct TVoxelQueryDataHelper;

template<typename T>
struct TVoxelQueryDataHelper
{
	FORCEINLINE static uint64 GetTypeHash(const T& Value)
	{
		return Value.GetHash() ^ FVoxelUtilities::MurmurHash64(TVoxelQueryDataHelper<typename T::Super>::GetTypeHash(Value));
	}
	FORCEINLINE static bool Identical(const T& A, const T& B)
	{
		return A.Identical(B) && TVoxelQueryDataHelper<typename T::Super>::Identical(A, B);
	}
	static void Check()
	{
		if constexpr (!TIsSame<typename T::Super, FVoxelQueryData>::Value)
		{
			check(&T::GetHash != &T::Super::GetHash);
			(void)static_cast<bool(T::*)(const T&) const>(&T::Identical);
		}
	}
};

template<>
struct TVoxelQueryDataHelper<FVoxelQueryData>
{
	FORCEINLINE static uint64 GetTypeHash(const FVoxelQueryData& Value)
	{
		return 0;
	}
	FORCEINLINE static bool Identical(const FVoxelQueryData& A, const FVoxelQueryData& B)
	{
		return true;
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct FVoxelGpuQueryData : public FVoxelQueryData
{
	GENERATED_BODY()
	GENERATED_VOXEL_QUERY_DATA_BODY()
	
	uint64 GetHash() const
	{
		return 0;
	}
	bool Identical(const FVoxelGpuQueryData& Other) const
	{
		return true;
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DECLARE_UNIQUE_VOXEL_ID(FVoxelChunkId);

class VOXELMETAGRAPH_API FVoxelDependency
{
public:
	FVoxelDependency() = default;
	UE_NONCOPYABLE(FVoxelDependency);

	bool IsInvalidated() const
	{
		return Invalidated.GetValue() != 0;
	}

	static void InvalidateDependencies(const TSet<TSharedPtr<FVoxelDependency>>& Dependencies);

private:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnDependenciesInvalidated, const TSet<TSharedPtr<FVoxelDependency>>& Dependencies);

	static FVoxelCriticalSection CriticalSection;
	static FOnDependenciesInvalidated OnDependenciesInvalidated_RequiresLock;

	FThreadSafeCounter Invalidated = 0;
	TMap<TWeakPtr<FVoxelChunkManager>, TArray<FVoxelChunkId>> Chunks_RequiresLock;

	friend class FVoxelChunkManager;
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelQuery
{
	GENERATED_BODY()

public:
	FVoxelQuery() = default;

public:
	void Add(const TSharedRef<const FVoxelQueryData>& QueryData);

	template<typename T, typename = typename TEnableIf<TIsDerivedFrom<T, FVoxelQueryData>::Value>::Type>
	T& Add()
	{
		TSharedRef<T> QueryData = MakeShared<T>();
		this->Add(QueryData);
		return *QueryData;
	}
	
	template<typename T, typename = typename TEnableIf<TIsDerivedFrom<T, FVoxelQueryData>::Value>::Type>
	FORCEINLINE TSharedPtr<const T> Find() const
	{
		return ::StaticCastSharedPtr<const T>(QueryDatas.FindRef(T::StaticStruct()));
	}

public:
	bool IsGpu() const
	{
		return QueryDatas.Contains(FVoxelGpuQueryData::StaticStruct());
	}
	FVoxelQuery MakeCpuQuery() const
	{
		FVoxelQuery NewQuery = *this;
		NewQuery.QueryDatas.Remove(FVoxelGpuQueryData::StaticStruct());
		return NewQuery;
	}
	FVoxelQuery MakeGpuQuery() const
	{
		FVoxelQuery NewQuery = *this;
		NewQuery.Add<FVoxelGpuQueryData>();
		return NewQuery;
	}

public:
	using FDependenciesQueue = TQueue<TSharedPtr<FVoxelDependency>, EQueueMode::Mpsc>;

	TSharedRef<FVoxelDependency> AllocateDependency() const;
	void AddDependency(const TSharedRef<FVoxelDependency>& Dependency) const
	{
		DependenciesQueue->Enqueue(Dependency);
	}

	void SetDependenciesQueue(const TSharedPtr<FDependenciesQueue>& NewDependenciesQueue)
	{
		DependenciesQueue = NewDependenciesQueue;
	}

private:
	TMap<UScriptStruct*, TSharedPtr<const FVoxelQueryData>> QueryDatas;
	TSharedPtr<FDependenciesQueue> DependenciesQueue;

	template<typename ValueType>
	friend class TVoxelQueryMap;

public:
	struct FCallstack : TArray<const FVoxelNode*> {};
	FCallstack Callstack;
};

USTRUCT()
struct FVoxelBoundsQueryData : public FVoxelQueryData
{
	GENERATED_BODY()
	GENERATED_VOXEL_QUERY_DATA_BODY()

	FVoxelBox Bounds;
	
	uint64 GetHash() const
	{
		return FVoxelUtilities::MurmurHash(Bounds);
	}
	bool Identical(const FVoxelBoundsQueryData& Other) const
	{
		return Bounds == Other.Bounds;
	}
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelLODQueryData : public FVoxelQueryData
{
	GENERATED_BODY()
	GENERATED_VOXEL_QUERY_DATA_BODY()

	int32 LOD = 0;

	uint64 GetHash() const
	{
		return FVoxelUtilities::MurmurHash(LOD);
	}
	bool Identical(const FVoxelLODQueryData& Other) const
	{
		return LOD == Other.LOD;
	}
};