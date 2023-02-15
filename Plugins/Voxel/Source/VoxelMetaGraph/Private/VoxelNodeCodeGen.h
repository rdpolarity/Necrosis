// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"

struct FVoxelNodeCodeGen
{
	static int32 GetNodeId(const UScriptStruct* Struct);
	static int32 GetRegisterWidth(const FVoxelPinType& Type);
	static FVoxelPinType GetRegisterType(const FVoxelPinType& Type);

	static void FormatCode(FString& Code);
	static FString GenerateFunction(const FVoxelNode& Node);
	static FString GenerateHLSLFunction(const FVoxelNode& Node);
	
	struct FCpuBuffer
	{
		void* Data;
		int32 Num;
	};
	static void ExecuteCpu(
		int32 Id,
		const TVoxelArray<FCpuBuffer>& Buffers,
		int32 Num);

	struct FGpuBuffer
	{
		int32 Num = 0;
		FVoxelRDGBuffer Buffer;
	};
	static void ExecuteGpu(
		FRDGBuilder& GraphBuilder,
		int32 Id,
		const TVoxelArray<FGpuBuffer>& BuffersIn,
		const TVoxelArray<FGpuBuffer>& BuffersOut,
		int32 Num);
};