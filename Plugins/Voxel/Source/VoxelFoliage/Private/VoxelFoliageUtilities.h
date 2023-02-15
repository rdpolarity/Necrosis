// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelBuffer.h"

class FVoxelFoliageUtilities
{
public:
	static bool GenerateHaltonPositions(const FVoxelNode* Node, const FVoxelBox& Bounds, const double DistanceBetweenPositions, uint32 Seed, FVoxelFloatBuffer &OutPositionsX, FVoxelFloatBuffer& OutPositionsY);
	
	static TVoxelFutureValue<TVoxelBufferView<float>> SplitGradientsBuffer(const FVoxelNodeRuntime& Runtime, const FVoxelQuery& Query, const TVoxelPinRef<FVoxelFloatBuffer>& HeightPin, TVoxelBufferView<FVector> PositionsView, float Step);
	static FVoxelVectorBufferView CollapseGradient(FVoxelFloatBufferView GradientHeight, FVoxelVectorBufferView PositionsView, float Step);
};