// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelBufferUtilities.h"
#include "VoxelMetaGraphRuntimeUtilities.h"
#include "VoxelBufferUtilitiesImpl.ispc.generated.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

BEGIN_VOXEL_COMPUTE_SHADER(VoxelBufferUtilities_GetGradient)
	VOXEL_SHADER_PARAMETER_CST(uint32, Num)
	VOXEL_SHADER_PARAMETER_CST(float, Step)
	VOXEL_SHADER_PARAMETER_CST(int32, Stride)

	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, Values)
	VOXEL_SHADER_PARAMETER_UAV(Buffer<float>, OutValues)
END_VOXEL_SHADER()

BEGIN_VOXEL_COMPUTE_SHADER(VoxelBufferUtilities_GetGradientCollapse)
	VOXEL_SHADER_PARAMETER_CST(uint32, Num)
	VOXEL_SHADER_PARAMETER_CST(float, Step)

	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, Values)
	VOXEL_SHADER_PARAMETER_UAV(Buffer<float>, OutValues)
END_VOXEL_SHADER()

BEGIN_VOXEL_COMPUTE_SHADER(VoxelBufferUtilities_SplitGradientBuffer)
	VOXEL_SHADER_PARAMETER_CST(uint32, Num)
	VOXEL_SHADER_PARAMETER_CST(float, HalfStep)

	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, Values)
	VOXEL_SHADER_PARAMETER_UAV(Buffer<float>, OutValues)
END_VOXEL_SHADER()

END_VOXEL_NAMESPACE(MetaGraph)

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelFloatBuffer FVoxelBufferUtilities::GetGradient_Cpu(const FVoxelFloatBufferView& Buffer, float Step, int32 Stride)
{
	VOXEL_FUNCTION_COUNTER();
	ensure(!Buffer.IsConstant());

	TVoxelArray<float> Result = FVoxelFloatBuffer::Allocate(Buffer.Num());

	ispc::VoxelBufferUtilities_GetGradient(
		Buffer.GetData(),
		Buffer.Num(),
		Step,
		Stride,
		Result.GetData());

	return FVoxelFloatBuffer::MakeCpu(Result);
}

FVoxelFloatBuffer FVoxelBufferUtilities::GetGradient_Gpu(FRDGBuilder& GraphBuilder, const FVoxelFloatBuffer& Buffer, float Step, int32 Stride)
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	const FVoxelFloatBuffer Result = FVoxelFloatBuffer::MakeGpu(Buffer.Num());

	BEGIN_VOXEL_SHADER_CALL(VoxelBufferUtilities_GetGradient)
	{
		Parameters.Num = Result.Num();
		Parameters.Step = Step;
		Parameters.Stride = Stride;

		Parameters.Values = Buffer.GetGpuBuffer();
		Parameters.OutValues = Result.GetGpuBuffer();

		Execute(FComputeShaderUtils::GetGroupCount(Parameters.Num, 64));
	}
	END_VOXEL_SHADER_CALL()

	return Result;
}

FVoxelFloatBuffer FVoxelBufferUtilities::GetGradientCollapse_Cpu(const FVoxelFloatBufferView& Buffer, float Step)
{
	VOXEL_FUNCTION_COUNTER();
	ensure(!Buffer.IsConstant());

	TVoxelArray<float> Result = FVoxelFloatBuffer::Allocate(Buffer.Num() / 2);
	
	ispc::VoxelBufferUtilities_GetGradientCollapse(
		Buffer.GetData(),
		FVoxelBuffer::AlignNum(Buffer.Num()),
		Step,
		Result.GetData());

	return FVoxelFloatBuffer::MakeCpu(Result);
}

FVoxelFloatBuffer FVoxelBufferUtilities::GetGradientCollapse_Gpu(FRDGBuilder& GraphBuilder, const FVoxelFloatBuffer& Buffer, float Step)
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	const FVoxelFloatBuffer Result = FVoxelFloatBuffer::MakeGpu(Buffer.Num() / 2);

	BEGIN_VOXEL_SHADER_CALL(VoxelBufferUtilities_GetGradientCollapse)
	{
		Parameters.Num = Result.Num();
		Parameters.Step = Step;

		Parameters.Values = Buffer.GetGpuBuffer();
		Parameters.OutValues = Result.GetGpuBuffer();

		Execute(FComputeShaderUtils::GetGroupCount(Parameters.Num, 64));
	}
	END_VOXEL_SHADER_CALL()

	return Result;
}

FVoxelFloatBuffer FVoxelBufferUtilities::SplitGradientBuffer_Cpu(const FVoxelFloatBuffer& Buffer, const FVoxelFloatBufferView& BufferView, const float HalfStep)
{
	VOXEL_FUNCTION_COUNTER();
	ensure(!Buffer.IsConstant());
	checkVoxelSlow(Buffer.MakeView().Get_CheckCompleted().GetByteArray() == BufferView.GetByteArray());

	TVoxelArray<float> Result = FVoxelFloatBuffer::Allocate(Buffer.Num() * 2);

	ispc::VoxelBufferUtilities_SplitGradientBuffer(
		BufferView.GetData(),
		Buffer.Num(),
		HalfStep,
		Result.GetData());

	FVoxelFloatBuffer ResultBuffer = FVoxelFloatBuffer::MakeCpu(Result);
	if (const TOptional<FFloatInterval> Interval = Buffer.GetInterval())
	{
		ResultBuffer.SetInterval({ Interval->Min - HalfStep, Interval->Max + HalfStep });
	}
	return ResultBuffer;
}

FVoxelFloatBuffer FVoxelBufferUtilities::SplitGradientBuffer_Gpu(FRDGBuilder& GraphBuilder, const FVoxelFloatBuffer& Buffer, float HalfStep)
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	FVoxelFloatBuffer Result = FVoxelFloatBuffer::MakeGpu(Buffer.Num() * 2);

	BEGIN_VOXEL_SHADER_CALL(VoxelBufferUtilities_SplitGradientBuffer)
	{
		Parameters.Num = Buffer.Num();
		Parameters.HalfStep = HalfStep;

		Parameters.Values = Buffer.GetGpuBuffer();
		Parameters.OutValues = Result.GetGpuBuffer();

		Execute(FComputeShaderUtils::GetGroupCount(Parameters.Num, 64));
	}
	END_VOXEL_SHADER_CALL()

	if (const TOptional<FFloatInterval> Interval = Buffer.GetInterval())
	{
		Result.SetInterval({ Interval->Min - HalfStep, Interval->Max + HalfStep });
	}

	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<FVoxelBufferData> FVoxelBufferUtilities::Filter_Cpu(const FVoxelTerminalBufferView& Buffer, const FVoxelBoolBufferView& Condition)
{
	VOXEL_FUNCTION_COUNTER();
	ensure(!Condition.IsConstant());

	const int32 TypeSize = Buffer.GetInnerType().GetTypeSize();
	const TConstVoxelArrayView<uint8> BufferByteView = Buffer.MakeByteView();
	
	if (Buffer.IsConstant())
	{
		int32 Num = 0;
		for (const bool bValue : Condition)
		{
			if (bValue)
			{
				Num++;
			}
		}
			
		TVoxelArray<uint8> OutData;
		FVoxelUtilities::SetNumFast(OutData, Num * TypeSize);

		VOXEL_SWITCH_TERMINAL_TYPE_SIZE(TypeSize)
		{
			for (int32 Index = 0; Index < Num; Index++)
			{
				FMemory::Memcpy(&OutData[Index * StaticTypeSize], BufferByteView.GetData(), StaticTypeSize);
			}
		};

		return FVoxelBufferData::MakeCpu(Buffer.GetInnerType(), MakeSharedCopy(MoveTemp(OutData)));
	}
	check(Buffer.Num() == Condition.Num());

	TVoxelArray<uint8> OutData = FVoxelBuffer::AllocateRaw(Buffer.Num(), TypeSize);

	int32 WriteIndex = 0;
	VOXEL_SWITCH_TERMINAL_TYPE_SIZE(TypeSize)
	{
		for (int32 Index = 0; Index < Buffer.Num(); Index++)
		{
			if (!Condition[Index])
			{
				continue;
			}

			FMemory::Memcpy(&OutData[WriteIndex * StaticTypeSize], &BufferByteView[Index * StaticTypeSize], StaticTypeSize);
			WriteIndex++;
		}
	};

	OutData.SetNum(WriteIndex * TypeSize, false);
	return FVoxelBufferData::MakeCpu(Buffer.GetInnerType(), MakeSharedCopy(MoveTemp(OutData)));
}

TSharedRef<FVoxelBufferData> FVoxelBufferUtilities::Select_Cpu(TConstVoxelArrayView<int32> Indices, TConstVoxelArrayView<const FVoxelTerminalBufferView*> Buffers)
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelPinType InnerType = Buffers[0]->GetInnerType();
	const int32 TypeSize = InnerType.GetTypeSize();

	TVoxelArray<TConstVoxelArrayView<uint8>> BufferViews;
	for (const FVoxelTerminalBufferView* Buffer : Buffers)
	{
		check(Buffer->GetInnerType() == InnerType);
		check(Buffer->Num() == 1 || Buffer->Num() == Indices.Num());

		const TConstVoxelArrayView<uint8> View = Buffer->MakeByteView();
		check(View.Num() == Buffer->Num() * TypeSize);
		BufferViews.Add(View);
	}

	TVoxelArray<uint8> OutData = FVoxelBuffer::AllocateRaw(Indices.Num(), TypeSize);

	VOXEL_SWITCH_TERMINAL_TYPE_SIZE(TypeSize)
	{
		for (int32 Index = 0; Index < Indices.Num(); Index++)
		{
			const int32 BufferIndex = Indices[Index];

			if (!Buffers.IsValidIndex(BufferIndex))
			{
				FMemory::Memzero(&OutData[Index * StaticTypeSize], StaticTypeSize);
				continue;
			}

			const TConstVoxelArrayView<uint8>& BufferView = BufferViews[BufferIndex];
			if (BufferView.Num() == StaticTypeSize)
			{
				FMemory::Memcpy(&OutData[Index * StaticTypeSize], BufferView.GetData(), StaticTypeSize);
				continue;
			}

			FMemory::Memcpy(&OutData[Index * StaticTypeSize], &BufferView[Index * StaticTypeSize], StaticTypeSize);
		}
	};
	
	return FVoxelBufferData::MakeCpu(Buffers[0]->GetInnerType(), MakeSharedCopy(MoveTemp(OutData)));
}

TSharedRef<FVoxelBufferData> FVoxelBufferUtilities::ExpandQuery2D_Cpu(const FVoxelTerminalBufferView& Buffer, int32 Count)
{
	VOXEL_FUNCTION_COUNTER();
	ensure(!Buffer.IsConstant());
	ensure(Count >= 2);
	ensure(Count % 2 == 0);

	const FVoxelPinType InnerType = Buffer.GetInnerType();
	const int32 TypeSize = InnerType.GetTypeSize();
	const TConstVoxelArrayView<uint8> BufferView = Buffer.MakeByteView();

	const int32 Num = Buffer.Num();
	ensure(Num % 4 == 0);

	TVoxelArray<uint8> OutData;
	FVoxelUtilities::SetNumFast(OutData, Num * Count * TypeSize);

	if (TypeSize == sizeof(float))
	{
		VOXEL_USE_NAMESPACE(MetaGraph);
		FRuntimeUtilities::ReplicatePacked(::ReinterpretCastVoxelArrayView<float>(BufferView), ::ReinterpretCastVoxelArrayView<float>(MakeVoxelArrayView(OutData).Slice(0, 2 * Num * TypeSize)));
	}
	else
	{
		VOXEL_SWITCH_TERMINAL_TYPE_SIZE(TypeSize)
		{
			for (int32 Index = 0; Index < Num; Index += 4)
			{
				FMemory::Memcpy(&OutData[(2 * Index + 0) * StaticTypeSize], &BufferView[Index * StaticTypeSize], 4 * StaticTypeSize);
				FMemory::Memcpy(&OutData[(2 * Index + 4) * StaticTypeSize], &BufferView[Index * StaticTypeSize], 4 * StaticTypeSize);
			}
		};
	}

	for (int32 Index = 2; Index < Count; Index += 2)
	{
		VOXEL_SCOPE_COUNTER("Memcpy");
		FVoxelUtilities::Memcpy(MakeVoxelArrayView(OutData).Slice(Index * Num * TypeSize, 2 * Num * TypeSize), MakeVoxelArrayView(OutData).Slice(0, 2 * Num * TypeSize));
	}

	return FVoxelBufferData::MakeCpu(Buffer.GetInnerType(), MakeSharedCopy(MoveTemp(OutData)));
}