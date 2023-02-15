// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"

template<typename ElementType, typename SizeType = int32>
class TVoxelArrayView;

// Switched range check to use checkVoxel
template<typename InElementType, typename InSizeType>
class TVoxelArrayView : public TArrayView<InElementType, InSizeType>
{
public:
	using Super = TArrayView<InElementType, InSizeType>;
	using typename Super::ElementType;
	using typename Super::SizeType;
	using Super::IsValidIndex;
	using Super::CheckInvariants;
	using Super::GetData;
	using Super::Num;

	template<typename T>
	using TIsCompatibleElementType = ArrayViewPrivate::TIsCompatibleElementType<T, ElementType>;
	template<typename T>
	using TIsCompatibleRangeType = ArrayViewPrivate::TIsCompatibleRangeType<T, ElementType>;
	template<typename T>
	using TIsReinterpretableRangeType = ArrayViewPrivate::TIsReinterpretableRangeType<T, ElementType>;

	TVoxelArrayView() = default;

	template<
		typename OtherRangeType,
		typename = typename TEnableIf<
			TAnd<
				TIsContiguousContainer<typename TRemoveCV<typename TRemoveReference<OtherRangeType>::Type>::Type>,
				TOr<
					TIsCompatibleRangeType<OtherRangeType>,
					TIsReinterpretableRangeType<OtherRangeType>
				>
			>::Value
		>::Type
	>
	FORCEINLINE TVoxelArrayView(OtherRangeType&& Other)
	{
		struct FDummy
		{
			ElementType* DataPtr;
			SizeType ArrayNum;
		};

		const auto InCount = GetNum(Forward<OtherRangeType>(Other));
		checkVoxelSlow((InCount >= 0) && ((sizeof(InCount) < sizeof(SizeType)) || (InCount <= static_cast<decltype(InCount)>(TNumericLimits<SizeType>::Max()))));

		reinterpret_cast<FDummy&>(*this).DataPtr = TChooseClass<
			TIsCompatibleRangeType<OtherRangeType>::Value,
			TIsCompatibleRangeType<OtherRangeType>,
			TIsReinterpretableRangeType<OtherRangeType>
		>::Result::GetData(Forward<OtherRangeType>(Other));

		reinterpret_cast<FDummy&>(*this).ArrayNum = SizeType(InCount);
	}

	template<typename OtherElementType, typename = typename TEnableIf<TIsCompatibleElementType<OtherElementType>::Value>::Type>
	FORCEINLINE TVoxelArrayView(OtherElementType* InData, SizeType InCount)
	{
		struct FDummy
		{
			ElementType* DataPtr;
			SizeType ArrayNum;
		};
		reinterpret_cast<FDummy&>(*this).DataPtr = InData;
		reinterpret_cast<FDummy&>(*this).ArrayNum = InCount;

		checkVoxelSlow(InCount >= 0);
	}

	FORCEINLINE void RangeCheck(InSizeType Index) const
	{
		CheckInvariants();

		checkfVoxelSlow((Index >= 0) & (Index < Num()),TEXT("Array index out of bounds: %i from an array of size %i"),Index,Num()); // & for one branch
	}
	
	FORCEINLINE TVoxelArrayView Slice(InSizeType Index, InSizeType InNum) const
	{
		checkVoxelSlow(InNum > 0);
		checkVoxelSlow(IsValidIndex(Index));
		checkVoxelSlow(IsValidIndex(Index + InNum - 1));
		return TVoxelArrayView(GetData() + Index, InNum);
	}
	
	FORCEINLINE ElementType& operator[](InSizeType Index) const
	{
		RangeCheck(Index);
		return GetData()[Index];
	}
	
	FORCEINLINE ElementType& Last(InSizeType IndexFromTheEnd = 0) const
	{
		RangeCheck(Num() - IndexFromTheEnd - 1);
		return GetData()[Num() - IndexFromTheEnd - 1];
	}
};

template <typename InElementType>
struct TIsZeroConstructType<TVoxelArrayView<InElementType>> : TIsZeroConstructType<TArrayView<InElementType>>
{
};
template <typename T>
struct TIsContiguousContainer<TVoxelArrayView<T>> : TIsContiguousContainer<TArrayView<T>>
{
};

template <
	typename OtherRangeType,
	typename CVUnqualifiedOtherRangeType = typename TRemoveCV<typename TRemoveReference<OtherRangeType>::Type>::Type,
	typename = typename TEnableIf<TIsContiguousContainer<CVUnqualifiedOtherRangeType>::Value>::Type
>
auto MakeVoxelArrayView(OtherRangeType&& Other)
{
	return TVoxelArrayView<typename TRemovePointer<decltype(GetData(DeclVal<OtherRangeType&>()))>::Type>(Forward<OtherRangeType>(Other));
}

template<typename ElementType>
auto MakeVoxelArrayView(ElementType* Pointer, int32 Size)
{
	return TVoxelArrayView<ElementType>(Pointer, Size);
}

template<typename ToType, typename FromType, typename SizeType>
TVoxelArrayView<ToType, SizeType> ReinterpretCastVoxelArrayView(TVoxelArrayView<FromType, SizeType> ArrayView)
{
	const int64 NumBytes = ArrayView.Num() * sizeof(FromType);
	checkVoxelSlow(NumBytes % sizeof(ToType) == 0);
	return TVoxelArrayView<ToType, SizeType>(reinterpret_cast<ToType*>(ArrayView.GetData()), NumBytes / sizeof(ToType));
}
template<typename ToType, typename FromType, typename SizeType, typename = typename TEnableIf<!TIsConst<ToType>::Value>::Type>
TVoxelArrayView<const ToType, SizeType> ReinterpretCastVoxelArrayView(TVoxelArrayView<const FromType, SizeType> ArrayView)
{
	return ::ReinterpretCastVoxelArrayView<const ToType>(ArrayView);
}

template<typename ToType, typename FromType, typename AllocatorType>
TVoxelArrayView<ToType, typename AllocatorType::SizeType> ReinterpretCastVoxelArrayView(TArray<FromType, AllocatorType>& Array)
{
	return ::ReinterpretCastVoxelArrayView<ToType>(::MakeVoxelArrayView(Array));
}
template<typename ToType, typename FromType, typename AllocatorType>
TVoxelArrayView<const ToType, typename AllocatorType::SizeType> ReinterpretCastVoxelArrayView(const TArray<FromType, AllocatorType>& Array)
{
	return ::ReinterpretCastVoxelArrayView<const ToType>(::MakeVoxelArrayView(Array));
}

template<typename T>
using TVoxelArrayView64 = TVoxelArrayView<T, int64>;

template<typename T, typename SizeType = int32>
using TConstVoxelArrayView = TVoxelArrayView<const T, SizeType>;

template<typename T>
using TConstVoxelArrayView64 = TConstVoxelArrayView<T, int64>;