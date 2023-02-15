// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoxelMacros.h"

template<typename T, typename = decltype(DeclVal<T>().AsShared())>
FORCEINLINE TWeakPtr<T> MakeWeakPtr(T* Ptr)
{
	return TWeakPtr<T>(StaticCastSharedRef<T>(Ptr->AsShared()));
}
template<typename T, typename = decltype(DeclVal<T>().AsShared())>
FORCEINLINE TWeakPtr<T> MakeWeakPtr(T& Ptr)
{
	return TWeakPtr<T>(StaticCastSharedRef<T>(Ptr.AsShared()));
}
template<typename T>
FORCEINLINE TWeakPtr<T> MakeWeakPtr(const TSharedPtr<T>& Ptr)
{
	return TWeakPtr<T>(Ptr);
}
template<typename T>
FORCEINLINE TWeakPtr<T> MakeWeakPtr(const TSharedRef<T>& Ptr)
{
	return TWeakPtr<T>(Ptr);
}

// Need TEnableIf as &&& is equivalent to &, so T could get matched with Smthg&
template<typename T>
FORCEINLINE typename TEnableIf<!TIsReferenceType<T>::Value, TSharedRef<T>>::Type MakeSharedCopy(T&& Data)
{
	return MakeShared<T>(MoveTemp(Data));
}
template<typename T>
FORCEINLINE typename TEnableIf<!TIsReferenceType<T>::Value, TUniquePtr<T>>::Type MakeUniqueCopy(T&& Data)
{
	return MakeUnique<T>(MoveTemp(Data));
}
template<typename T>
FORCEINLINE typename TEnableIf<!TIsReferenceType<T>::Value, T>::Type MakeCopy(T&& Data)
{
	return MoveTemp(Data);
}

template<typename T>
FORCEINLINE TSharedRef<T> MakeSharedCopy(const T& Data)
{
	return MakeShared<T>(Data);
}
template<typename T>
FORCEINLINE TUniquePtr<T> MakeUniqueCopy(const T& Data)
{
	return MakeUnique<T>(Data);
}
template<typename T>
FORCEINLINE T MakeCopy(const T& Data)
{
	return Data;
}

template<typename T>
FORCEINLINE SharedPointerInternals::TRawPtrProxy<T> MakeShareable(TUniquePtr<T> UniquePtr)
{
	checkVoxelSlow(UniquePtr.IsValid());
	return ::MakeShareable(UniquePtr.Release());
}