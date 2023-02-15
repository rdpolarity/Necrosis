// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphImpl.isph"

// ReSharper disable CppCStyleCast

struct FVoxelBuffer
{
	void* Data;
	bool bIsConstant;
};

FORCEINLINE bool LoadBool(const uniform FVoxelBuffer Buffer, const int32 Index)
{
	const bool* uniform BufferPtr = (bool*)Buffer.Data;
	return Buffer.bIsConstant ? BufferPtr[0] : BufferPtr[Index];
}
FORCEINLINE uint8 LoadByte(const uniform FVoxelBuffer Buffer, const int32 Index)
{
	const uint8* uniform BufferPtr = (uint8*)Buffer.Data;
	return Buffer.bIsConstant ? BufferPtr[0] : BufferPtr[Index];
}
FORCEINLINE float LoadFloat(const uniform FVoxelBuffer Buffer, const int32 Index)
{
	const float* uniform BufferPtr = (float*)Buffer.Data;
	return Buffer.bIsConstant ? BufferPtr[0] : BufferPtr[Index];
}
FORCEINLINE int32 LoadInt32(const uniform FVoxelBuffer Buffer, const int32 Index)
{
	const int32* uniform BufferPtr = (int32*)Buffer.Data;
	return Buffer.bIsConstant ? BufferPtr[0] : BufferPtr[Index];
}

FORCEINLINE bool StoreBool(const uniform FVoxelBuffer Buffer, const int32 Index, const bool bValue)
{
	bool* uniform BufferPtr = (bool*)Buffer.Data;
	return BufferPtr[Index] = bValue;
}
FORCEINLINE uint8 StoreByte(const uniform FVoxelBuffer Buffer, const int32 Index, const uint8 bValue)
{
	uint8* uniform BufferPtr = (uint8*)Buffer.Data;
	return BufferPtr[Index] = bValue;
}
FORCEINLINE float StoreFloat(const uniform FVoxelBuffer Buffer, const int32 Index, const float Value)
{
	float* uniform BufferPtr = (float*)Buffer.Data;
	return BufferPtr[Index] = Value;
}
FORCEINLINE int32 StoreInt32(const uniform FVoxelBuffer Buffer, const int32 Index, const int32 Value)
{
	int32* uniform BufferPtr = (int32*)Buffer.Data;
	return BufferPtr[Index] = Value;
}

#include "VoxelNodeCodeGenImpl_Functions.isph"

export void VoxelCodeGen_Execute(
	const uniform int32 Id,
	const FVoxelBuffer* uniform Buffers,
	const uniform int32 Num)
{
	switch (Id)
	{
		default: check(false);
#include "VoxelNodeCodeGenImpl_Cases.isph"
	}
}