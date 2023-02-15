// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "Containers/SparseArray.h"

// Default one does not do check on element access
template<typename InElementType, typename InAllocator = FDefaultSparseArrayAllocator>
class TVoxelSparseArray : public TSparseArray<InElementType, InAllocator>
{
public:
	using Super = TSparseArray<InElementType, InAllocator>;
	
	// Accessors.
	FORCEINLINE InElementType& operator[](int32 Index)
	{
		checkVoxelSlow(Index >= 0 && Index < this->GetMaxIndex() && this->IsValidIndex(Index));
		return Super::operator[](Index);
	}
	FORCEINLINE const InElementType& operator[](int32 Index) const
	{
		checkVoxelSlow(Index >= 0 && Index < this->GetMaxIndex() && this->IsValidIndex(Index));
		return Super::operator[](Index);
	}
};

// UniqueClass: to forbid copying ids from different classes
template<typename UniqueClass>
class TVoxelTypedSparseArrayId
{
public:
	TVoxelTypedSparseArrayId() = default;

	bool operator==(TVoxelTypedSparseArrayId Other) const { return Other.Index == Index; }
	bool operator!=(TVoxelTypedSparseArrayId Other) const { return Other.Index != Index; }

public:
	bool IsValid() const
	{
		return Index != 0;
	}
	void Reset()
	{
		Index = 0;
	}
	uint32 GetDebugValue() const
	{
		return Index;
	}

public:
	friend uint32 GetTypeHash(TVoxelTypedSparseArrayId Value)
	{
		return GetTypeHash(Value.Index);
	}
	
private:
	TVoxelTypedSparseArrayId(uint32 Index) : Index(Index) {}

	uint32 Index = 0;

	template<typename InKeyType, typename InElementType, typename InAllocator>
	friend class TVoxelTypedSparseArray;
};

#define DECLARE_TYPED_VOXEL_SPARSE_ARRAY_ID(Name) using Name = TVoxelTypedSparseArrayId<class Name##_Unique>;

template<typename InKeyType, typename InElementType, typename InAllocator = FDefaultSparseArrayAllocator>
class TVoxelTypedSparseArray
{
public:
	FORCEINLINE InElementType& operator[](InKeyType Index)
	{
		checkVoxelSlow(Index.IsValid());
		return Storage[Index.Index - 1];
	}
	FORCEINLINE const InElementType& operator[](InKeyType Index) const
	{
		checkVoxelSlow(Index.IsValid());
		return Storage[Index.Index - 1];
	}
	FORCEINLINE bool IsValidIndex(InKeyType Index) const
	{
		return Index.IsValid() && Storage.IsValidIndex(Index.Index - 1);
	}
	FORCEINLINE InKeyType Add(const InElementType& Element)
	{
		return { uint32(Storage.Add(Element)) + 1 };
	}
	FORCEINLINE InKeyType Add(InElementType&& Element)
	{
		return { uint32(Storage.Add(MoveTemp(Element))) + 1 };
	}
	FORCEINLINE void RemoveAt(InKeyType Index)
	{
		check(Index.IsValid());
		Storage.RemoveAt(Index.Index - 1);
	}
	FORCEINLINE int32 Num() const
	{
		return Storage.Num();
	}

	void Reset()
	{
		Storage.Reset();
	}

public:
	inline auto begin()       { return Storage.begin(); }
	inline auto begin() const { return Storage.begin(); }
	inline auto end  ()       { return Storage.end(); }
	inline auto end  () const { return Storage.end(); }

private:
	TVoxelSparseArray<InElementType, InAllocator> Storage;
};