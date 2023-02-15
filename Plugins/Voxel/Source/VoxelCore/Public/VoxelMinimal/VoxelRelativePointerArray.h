// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "VoxelMinimal/Containers/VoxelArray.h"

template<typename T>
struct TVoxelRelativePointerArray
{
	TVoxelArray<T*> Data;

	TVoxelRelativePointerArray() = default;
	TVoxelRelativePointerArray(TVoxelRelativePointerArray&&) = delete;
	TVoxelRelativePointerArray& operator=(TVoxelRelativePointerArray&&) = delete;

	FORCEINLINE TVoxelRelativePointerArray(const TVoxelRelativePointerArray& Other)
	{
		Data = Other.Data;

		const uint8* ThisPtr = reinterpret_cast<const uint8*>(this);
		const uint8* OtherPtr = reinterpret_cast<const uint8*>(&Other);

		for (T*& Pointer : Data)
		{
			const uint8* PointerPtr = reinterpret_cast<const uint8*>(Pointer);
			checkVoxelSlow(OtherPtr < PointerPtr);
			Pointer = reinterpret_cast<T*>(VOXEL_CONST_CAST(ThisPtr + (PointerPtr - OtherPtr)));
		}
	}
	FORCEINLINE TVoxelRelativePointerArray& operator=(const TVoxelRelativePointerArray& Other)
	{
		Data = Other.Data;

		const uint8* ThisPtr = reinterpret_cast<const uint8*>(this);
		const uint8* OtherPtr = reinterpret_cast<const uint8*>(&Other);

		for (T*& Pointer : Data)
		{
			const uint8* PointerPtr = reinterpret_cast<const uint8*>(Pointer);
			checkVoxelSlow(OtherPtr < PointerPtr);
			Pointer = reinterpret_cast<T*>(VOXEL_CONST_CAST(ThisPtr + (PointerPtr - OtherPtr)));
		}

		return *this;
	}

	FORCEINLINE T* operator[](int32 Index)
	{
		return Data[Index];
	}
	FORCEINLINE T* operator[](int32 Index) const
	{
		return Data[Index];
	}
};