// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelTask.h"
#include "VoxelMetaGraphNodeInterface.generated.h"

UINTERFACE()
class VOXELMETAGRAPH_API UVoxelMetaGraphNodeInterface : public UInterface
{
	GENERATED_BODY()
};

class VOXELMETAGRAPH_API IVoxelMetaGraphNodeInterface : public IInterface
{
	GENERATED_BODY()

public:
	struct FTime
	{
		double Exclusive = 0;
		double Inclusive = 0;

		double Get() const
		{
			return FVoxelTaskStat::bStaticInclusiveStats ? Inclusive : Exclusive;
		}

		FTime& operator+=(const FTime& Other)
		{
			Exclusive += Other.Exclusive;
			Inclusive += Other.Inclusive;
			return *this;
		}
		FTime operator+(const FTime& Other) const
		{
			return FTime(*this) += Other;
		}
	};
	TVoxelStaticArray<FTime, FVoxelTaskStat::Count> Times{ ForceInit };

	FTime GetCpuTime() const
	{
		return
			Times[FVoxelTaskStat::CpuGameThread] +
			Times[FVoxelTaskStat::CpuRenderThread] +
			Times[FVoxelTaskStat::CpuAsyncThread];
	}
	FTime GetGpuTime() const
	{
		return Times[FVoxelTaskStat::GpuCompute];
	}
	FTime GetCopyTime() const
	{
		return
			Times[FVoxelTaskStat::CopyCpuToGpu] +
			Times[FVoxelTaskStat::CopyGpuToCpu];
	}
};