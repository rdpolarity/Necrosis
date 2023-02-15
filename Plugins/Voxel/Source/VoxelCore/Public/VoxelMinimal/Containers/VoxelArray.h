// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"

template<typename InElementType, typename InAllocator = FDefaultAllocator>
class TVoxelArray : public TArray<InElementType, InAllocator>
{
public:
	using Super = TArray<InElementType, InAllocator>;
	using typename Super::SizeType;
	using typename Super::ElementType;
	using Allocator = typename Super::AllocatorType;
	using Super::GetData;
	using Super::CheckInvariants;
	using Super::ArrayNum;
	using Super::ArrayMax;
	using Super::Emplace;
	using Super::Emplace_GetRef;
	using Super::Num;

	using Super::Super;

#if !defined(_MSC_VER) || _MSC_VER > 1929 || PLATFORM_COMPILER_CLANG // Copy constructors are inherited on MSVC, but implementing them manually deletes all the other inherited constructors
	template <typename OtherElementType, typename OtherAllocator>
	FORCEINLINE explicit TVoxelArray(const TArray<OtherElementType, OtherAllocator>& Other)
		: Super(Other)
	{
	}
#endif

public:
	FORCEINLINE void RangeCheck(SizeType Index) const
	{
		CheckInvariants();

		// Template property, branch will be optimized out
		if (Allocator::RequireRangeCheck)
		{
			checkfVoxelSlow((Index >= 0) & (Index < ArrayNum), TEXT("Array index out of bounds: %i from an array of size %i"), Index, ArrayNum); // & for one branch
		}
	}
	FORCEINLINE void CheckAddress(const ElementType* Addr) const
	{
		checkfVoxelSlow(Addr < GetData() || Addr >= (GetData() + ArrayMax), TEXT("Attempting to use a container element (%p) which already comes from the container being modified (%p, ArrayMax: %d, ArrayNum: %d, SizeofElement: %d)!"), Addr, GetData(), ArrayMax, ArrayNum, sizeof(ElementType));
	}

public:
	FORCEINLINE SizeType Add(ElementType&& Item)
	{
		CheckAddress(&Item);
		return Emplace(MoveTempIfPossible(Item));
	}
	FORCEINLINE SizeType Add(const ElementType& Item)
	{
		CheckAddress(&Item);
		return Emplace(Item);
	}
	FORCEINLINE ElementType& Add_GetRef(ElementType&& Item)
	{
		CheckAddress(&Item);
		return Emplace_GetRef(MoveTempIfPossible(Item));
	}
	FORCEINLINE ElementType& Add_GetRef(const ElementType& Item)
	{
		CheckAddress(&Item);
		return Emplace_GetRef(Item);
	}

public:
	FORCEINLINE void SwapMemory(SizeType FirstIndexToSwap, SizeType SecondIndexToSwap)
	{
		// FMemory::Memswap is not inlined
		
		ElementType& A = (*this)[FirstIndexToSwap];
		ElementType& B = (*this)[SecondIndexToSwap];

		TTypeCompatibleBytes<ElementType> Temp;
		FMemory::Memcpy(&Temp, &A, sizeof(ElementType));
		FMemory::Memcpy(&A, &B, sizeof(ElementType));
		FMemory::Memcpy(&B, &Temp, sizeof(ElementType));
	}
	FORCEINLINE void Swap(SizeType FirstIndexToSwap, SizeType SecondIndexToSwap)
	{
		checkVoxelSlow((FirstIndexToSwap >= 0) && (SecondIndexToSwap >= 0));
		checkVoxelSlow((ArrayNum > FirstIndexToSwap) && (ArrayNum > SecondIndexToSwap));
		if (FirstIndexToSwap != SecondIndexToSwap)
		{
			SwapMemory(FirstIndexToSwap, SecondIndexToSwap);
		}
	}
	
public:
	FORCEINLINE ElementType& operator[](SizeType Index)
	{
		RangeCheck(Index);
		return GetData()[Index];
	}

	FORCEINLINE const ElementType& operator[](SizeType Index) const
	{
		RangeCheck(Index);
		return GetData()[Index];
	}

public:
#if TARRAY_RANGED_FOR_CHECKS && VOXEL_DEBUG
	typedef TCheckedPointerIterator<      ElementType, SizeType> RangedForIteratorType;
	typedef TCheckedPointerIterator<const ElementType, SizeType> RangedForConstIteratorType;
#else
	typedef       ElementType* RangedForIteratorType;
	typedef const ElementType* RangedForConstIteratorType;
#endif

#if TARRAY_RANGED_FOR_CHECKS && VOXEL_DEBUG
	FORCEINLINE RangedForIteratorType      begin() { return RangedForIteratorType(ArrayNum, GetData()); }
	FORCEINLINE RangedForConstIteratorType begin() const { return RangedForConstIteratorType(ArrayNum, GetData()); }
	FORCEINLINE RangedForIteratorType      end() { return RangedForIteratorType(ArrayNum, GetData() + Num()); }
	FORCEINLINE RangedForConstIteratorType end() const { return RangedForConstIteratorType(ArrayNum, GetData() + Num()); }
#else
	FORCEINLINE RangedForIteratorType      begin() { return GetData(); }
	FORCEINLINE RangedForConstIteratorType begin() const { return GetData(); }
	FORCEINLINE RangedForIteratorType      end() { return GetData() + Num(); }
	FORCEINLINE RangedForConstIteratorType end() const { return GetData() + Num(); }
#endif
};

template<typename ElementType, typename Allocator>
FORCEINLINE TVoxelArray<ElementType, Allocator>& CastToVoxelArray(TArray<ElementType, Allocator>& Array)
{
	return static_cast<TVoxelArray<ElementType, Allocator>&>(Array);
}

template<typename ElementType, typename Allocator>
FORCEINLINE const TVoxelArray<ElementType, Allocator>& CastToVoxelArray(const TArray<ElementType, Allocator>& Array)
{
	return static_cast<const TVoxelArray<ElementType, Allocator>&>(Array);
}

template<typename ElementType, typename Allocator>
FORCEINLINE TVoxelArray<ElementType, Allocator>&& CastToVoxelArray(TArray<ElementType, Allocator>&& Array)
{
	return static_cast<TVoxelArray<ElementType, Allocator>&&>(Array);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename To, typename ElementType, typename Allocator>
FORCEINLINE TVoxelArray<To, Allocator>& ReinterpretCastVoxelArray(TVoxelArray<ElementType, Allocator>& Array)
{
	static_assert(sizeof(To) == sizeof(ElementType), "");
	return reinterpret_cast<TVoxelArray<To, Allocator>&>(Array);
}

template<typename To, typename ElementType, typename Allocator>
FORCEINLINE const TVoxelArray<To, Allocator>& ReinterpretCastVoxelArray(const TVoxelArray<ElementType, Allocator>& Array)
{
	static_assert(sizeof(To) == sizeof(ElementType), "");
	return reinterpret_cast<const TVoxelArray<To, Allocator>&>(Array);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename ToType, typename ToAllocator, typename FromType, typename Allocator, typename = typename TEnableIf<sizeof(FromType) != sizeof(ToType)>::Type>
TVoxelArray<ToType, ToAllocator> ReinterpretCastVoxelArray_Copy(const TVoxelArray<FromType, Allocator>& Array)
{
	const int64 NumBytes = Array.Num() * sizeof(FromType);
	check(NumBytes % sizeof(ToType) == 0);
	return TVoxelArray<ToType, Allocator>(reinterpret_cast<const ToType*>(Array.GetData()), NumBytes / sizeof(ToType));
}
template<typename ToType, typename FromType, typename Allocator, typename = typename TEnableIf<sizeof(FromType) != sizeof(ToType)>::Type>
TVoxelArray<ToType, Allocator> ReinterpretCastVoxelArray_Copy(const TVoxelArray<FromType, Allocator>& Array)
{
	return ::ReinterpretCastVoxelArray_Copy<ToType, Allocator>(Array);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template <typename InElementType, typename Allocator>
struct TIsZeroConstructType<TVoxelArray<InElementType, Allocator>> : TIsZeroConstructType<TArray<InElementType, Allocator>>
{
};

template <typename InElementType, typename Allocator>
struct TContainerTraits<TVoxelArray<InElementType, Allocator>> : TContainerTraits<TArray<InElementType, Allocator>>
{
};

template <typename T, typename Allocator>
struct TIsContiguousContainer<TVoxelArray<T, Allocator>> : TIsContiguousContainer<TArray<T, Allocator>>
{
};

template <typename InElementType, typename Allocator> struct TIsTArray<               TVoxelArray<InElementType, Allocator>> { enum { Value = true }; };
template <typename InElementType, typename Allocator> struct TIsTArray<const          TVoxelArray<InElementType, Allocator>> { enum { Value = true }; };
template <typename InElementType, typename Allocator> struct TIsTArray<      volatile TVoxelArray<InElementType, Allocator>> { enum { Value = true }; };
template <typename InElementType, typename Allocator> struct TIsTArray<const volatile TVoxelArray<InElementType, Allocator>> { enum { Value = true }; };

template<typename T> using TVoxelArray64 = TVoxelArray<T, FDefaultAllocator64>;