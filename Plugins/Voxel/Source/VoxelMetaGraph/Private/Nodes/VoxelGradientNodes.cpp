// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelGradientNodes.h"
#include "Nodes/VoxelPositionNodes.h"
#include "VoxelBufferUtilities.h"

DEFINE_VOXEL_NODE_GPU(FVoxelNode_GetGradientBase, Gradient)
{
	FindVoxelQueryData(FVoxelPositionQueryData, PositionQueryData);
	FindVoxelQueryData(FVoxelGradientStepQueryData, GradientStepQueryData);

	const EVoxelAxis Axis = GetAxis();
	const int32 Stride = PositionQueryData->GetGradientStride(Axis);

	if (Stride != -1)
	{
		const TValue<FVoxelFloatBuffer> Value = Get(ValuePin, Query);

		return VOXEL_ON_COMPLETE(RenderThread, PositionQueryData, GradientStepQueryData, Stride, Value)
		{
			if (Value.IsConstant())
			{
				return FVoxelFloatBuffer::Constant(0.f);
			}

			CheckVoxelBuffersNum(Value, PositionQueryData->GetPositions());

			return FVoxelBufferUtilities::GetGradient_Gpu(GraphBuilder, Value, GradientStepQueryData->Step, Stride);
		};
	}

	return VOXEL_ON_COMPLETE(RenderThread, PositionQueryData, GradientStepQueryData, Axis)
	{
		const float HalfStep = GradientStepQueryData->Step / 2.f;

		FVoxelVectorBuffer Positions;
		switch (Axis)
		{
		default: ensure(false);
		case EVoxelAxis::X:
		{
			Positions.X = FVoxelBufferUtilities::SplitGradientBuffer_Gpu(GraphBuilder, PositionQueryData->GetPositions().X, HalfStep);
			Positions.Y = FVoxelBufferUtilities::SplitGradientBuffer_Gpu(GraphBuilder, PositionQueryData->GetPositions().Y, 0.f);
			Positions.Z = FVoxelBufferUtilities::SplitGradientBuffer_Gpu(GraphBuilder, PositionQueryData->GetPositions().Z, 0.f);
		}
		break;
		case EVoxelAxis::Y:
		{
			Positions.X = FVoxelBufferUtilities::SplitGradientBuffer_Gpu(GraphBuilder, PositionQueryData->GetPositions().X, 0.f);
			Positions.Y = FVoxelBufferUtilities::SplitGradientBuffer_Gpu(GraphBuilder, PositionQueryData->GetPositions().Y, HalfStep);
			Positions.Z = FVoxelBufferUtilities::SplitGradientBuffer_Gpu(GraphBuilder, PositionQueryData->GetPositions().Z, 0.f);
		}
		break;
		case EVoxelAxis::Z:
		{
			Positions.X = FVoxelBufferUtilities::SplitGradientBuffer_Gpu(GraphBuilder, PositionQueryData->GetPositions().X, 0.f);
			Positions.Y = FVoxelBufferUtilities::SplitGradientBuffer_Gpu(GraphBuilder, PositionQueryData->GetPositions().Y, 0.f);
			Positions.Z = FVoxelBufferUtilities::SplitGradientBuffer_Gpu(GraphBuilder, PositionQueryData->GetPositions().Z, HalfStep);
		}
		break;
		}

		FVoxelQuery ChildQuery = Query;
		ChildQuery.Add<FVoxelGradientPositionQueryData>().Initialize(
			PositionQueryData->Is2D(),
			Axis,
			Positions,
			Cast<FVoxelGradientPositionQueryData>(PositionQueryData).Get());

		const TSharedPtr<const FVoxelGradientPositionQueryData> ChildPositionQueryData = ChildQuery.Find<FVoxelGradientPositionQueryData>();
		
		const TValue<FVoxelFloatBuffer> Value = Get(ValuePin, ChildQuery);

		return VOXEL_ON_COMPLETE(RenderThread, PositionQueryData, ChildPositionQueryData, GradientStepQueryData, Value)
		{
			if (Value.IsConstant())
			{
				return FVoxelFloatBuffer::Constant(0.f);
			}

			const int32 Num = ComputeVoxelBuffersNum(Value, ChildPositionQueryData->GetPositions());
			ensure(Num == 2 * PositionQueryData->GetPositions().Num());

			return FVoxelBufferUtilities::GetGradientCollapse_Gpu(GraphBuilder, Value, GradientStepQueryData->Step);
		};
	};
}

DEFINE_VOXEL_NODE_CPU(FVoxelNode_GetGradientBase, Gradient)
{
	FindVoxelQueryData(FVoxelPositionQueryData, PositionQueryData);
	FindVoxelQueryData(FVoxelGradientStepQueryData, GradientStepQueryData);

	const EVoxelAxis Axis = GetAxis();
	const int32 Stride = PositionQueryData->GetGradientStride(Axis);

	if (Stride != -1)
	{
		const TValue<TBufferView<float>> Value = GetBufferView(ValuePin, Query);

		return VOXEL_ON_COMPLETE(AsyncThread, PositionQueryData, GradientStepQueryData, Stride, Value)
		{
			if (Value.IsConstant())
			{
				return FVoxelFloatBuffer::Constant(0.f);
			}
			
			CheckVoxelBuffersNum(Value, PositionQueryData->GetPositions());

			return FVoxelBufferUtilities::GetGradient_Cpu(Value, GradientStepQueryData->Step, Stride);
		};
	}

	const TValue<TBufferView<float>> PositionsX = PositionQueryData->GetPositions().X.MakeView();
	const TValue<TBufferView<float>> PositionsY = PositionQueryData->GetPositions().Y.MakeView();
	const TValue<TBufferView<float>> PositionsZ = PositionQueryData->GetPositions().Z.MakeView();

	return VOXEL_ON_COMPLETE(AsyncThread, PositionQueryData, GradientStepQueryData, Axis, PositionsX, PositionsY, PositionsZ)
	{
		const float HalfStep = GradientStepQueryData->Step / 2.f;

		FVoxelVectorBuffer Positions;
		switch (Axis)
		{
		default: ensure(false);
		case EVoxelAxis::X:
		{
			Positions.X = FVoxelBufferUtilities::SplitGradientBuffer_Cpu(PositionQueryData->GetPositions().X, PositionsX, HalfStep);
			Positions.Y = FVoxelBufferUtilities::SplitGradientBuffer_Cpu(PositionQueryData->GetPositions().Y, PositionsY, 0.f);
			Positions.Z = FVoxelBufferUtilities::SplitGradientBuffer_Cpu(PositionQueryData->GetPositions().Z, PositionsZ, 0.f);
		}
		break;
		case EVoxelAxis::Y:
		{
			Positions.X = FVoxelBufferUtilities::SplitGradientBuffer_Cpu(PositionQueryData->GetPositions().X, PositionsX, 0.f);
			Positions.Y = FVoxelBufferUtilities::SplitGradientBuffer_Cpu(PositionQueryData->GetPositions().Y, PositionsY, HalfStep);
			Positions.Z = FVoxelBufferUtilities::SplitGradientBuffer_Cpu(PositionQueryData->GetPositions().Z, PositionsZ, 0.f);
		}
		break;
		case EVoxelAxis::Z:
		{
			Positions.X = FVoxelBufferUtilities::SplitGradientBuffer_Cpu(PositionQueryData->GetPositions().X, PositionsX, 0.f);
			Positions.Y = FVoxelBufferUtilities::SplitGradientBuffer_Cpu(PositionQueryData->GetPositions().Y, PositionsY, 0.f);
			Positions.Z = FVoxelBufferUtilities::SplitGradientBuffer_Cpu(PositionQueryData->GetPositions().Z, PositionsZ, HalfStep);
		}
		break;
		}

		FVoxelQuery ChildQuery = Query;
		ChildQuery.Add<FVoxelGradientPositionQueryData>().Initialize(
			PositionQueryData->Is2D(),
			Axis,
			Positions,
			Cast<FVoxelGradientPositionQueryData>(PositionQueryData).Get());

		const TSharedPtr<const FVoxelGradientPositionQueryData> ChildPositionQueryData = ChildQuery.Find<FVoxelGradientPositionQueryData>();
		
		const TValue<TBufferView<float>> Value = GetBufferView(ValuePin, ChildQuery);

		return VOXEL_ON_COMPLETE(AsyncThread, PositionQueryData, ChildPositionQueryData, GradientStepQueryData, Value)
		{
			if (Value.IsConstant())
			{
				return FVoxelFloatBuffer::Constant(0.f);
			}
			
			const int32 Num = ComputeVoxelBuffersNum(Value, ChildPositionQueryData->GetPositions());
			ensure(Num == 2 * PositionQueryData->GetPositions().Num());

			return FVoxelBufferUtilities::GetGradientCollapse_Cpu(Value, GradientStepQueryData->Step);
		};
	};
}