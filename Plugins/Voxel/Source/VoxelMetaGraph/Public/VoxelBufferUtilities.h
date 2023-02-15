// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBuffer.h"

struct VOXELMETAGRAPH_API FVoxelBufferUtilities
{
public:
	static FVoxelFloatBuffer GetGradient_Cpu(const FVoxelFloatBufferView& Buffer, float Step, int32 Stride);
	static FVoxelFloatBuffer GetGradient_Gpu(FRDGBuilder& GraphBuilder, const FVoxelFloatBuffer& Buffer, float Step, int32 Stride);

	static FVoxelFloatBuffer GetGradientCollapse_Cpu(const FVoxelFloatBufferView& Buffer, float Step);
	static FVoxelFloatBuffer GetGradientCollapse_Gpu(FRDGBuilder& GraphBuilder, const FVoxelFloatBuffer& Buffer, float Step);

	static FVoxelFloatBuffer SplitGradientBuffer_Cpu(const FVoxelFloatBuffer& Buffer, const FVoxelFloatBufferView& BufferView, float HalfStep);
	static FVoxelFloatBuffer SplitGradientBuffer_Gpu(FRDGBuilder& GraphBuilder, const FVoxelFloatBuffer& Buffer, float HalfStep);
	
public:
	static TSharedRef<FVoxelBufferData> Filter_Cpu(const FVoxelTerminalBufferView& Buffer, const FVoxelBoolBufferView& Condition);
	static TSharedRef<FVoxelBufferData> Select_Cpu(TConstVoxelArrayView<int32> Indices, TConstVoxelArrayView<const FVoxelTerminalBufferView*> Buffers);
	static TSharedRef<FVoxelBufferData> ExpandQuery2D_Cpu(const FVoxelTerminalBufferView& Buffer, int32 Count);
};