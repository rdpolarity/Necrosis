// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelBasicNodes.h"
#include "VoxelBufferUtilities.h"

DEFINE_VOXEL_NODE(FVoxelNode_ToArray, OutValue)
{
	const FVoxelFutureValue Value = Get(ValuePin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Value)
	{
		FVoxelPinValue Result = FVoxelPinValue(ReturnPinType);
		Result.Get<FVoxelBuffer>().InitializeFromConstant(Value);
		return FVoxelSharedPinValue(Result);
	};
}

FVoxelPinTypeSet FVoxelNode_ToArray::GetPromotionTypes(const FVoxelPin& Pin) const
{
	FVoxelPinTypeSet OutTypes;
	for (const FVoxelPinType& Type : FVoxelPinType::GetAllBufferTypes())
	{
		if (Pin.bIsInput)
		{
			OutTypes.Add(Type.GetInnerType());
		}
		else
		{
			OutTypes.Add(Type.GetBufferType());
		}
	}
	return OutTypes;
}

void FVoxelNode_ToArray::PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	Pin.SetType(NewType);

	if (Pin.bIsInput)
	{
		GetPin(OutValuePin).SetType(NewType.GetBufferType());
	}
	else
	{
		GetPin(ValuePin).SetType(NewType.GetInnerType());
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_GetLOD, LOD)
{
	FindVoxelQueryData(FVoxelLODQueryData, LODQueryData);

	return LODQueryData->LOD;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_IsInBounds, Result)
{
	FindVoxelQueryData(FVoxelBoundsQueryData, BoundsQueryData);

	const TValue<TBufferView<FVector>> Positions = GetBufferView(PositionPin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, Positions)
	{
		TVoxelArray<bool> Result = FVoxelBoolBuffer::Allocate(Positions.Num());
		for (int32 Index = 0; Index < Positions.Num(); Index++)
		{
			Result[Index] = BoundsQueryData->Bounds.Contains(Positions[Index]);
		}
		return FVoxelBoolBuffer::MakeCpu(Result);
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_RandFloat, Result)
{
	const TValue<TBufferView<FVector>> Positions = GetBufferView(PositionPin, Query);
	const TValue<int32> Seed = Get(SeedPin, Query);
	const TValue<float> Min = Get(MinPin, Query);
	const TValue<float> Max = Get(MaxPin, Query);
	const TValue<int32> RoundingDecimals = Get(RoundingDecimalsPin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, Positions, Seed, Min, Max, RoundingDecimals)
	{
		const uint64 SeedHash = FVoxelUtilities::MurmurHash64(Seed);
		const float RoundValue = FMath::Pow(10, float(RoundingDecimals));

		TVoxelArray<float> Result = FVoxelFloatBuffer::Allocate(Positions.Num());
		for (int32 Index = 0; Index < Positions.Num(); Index++)
		{
			FVector3f Position;
			Position.X = int32(Positions[Index].X * RoundValue + 0.5f) / RoundValue;
			Position.Y = int32(Positions[Index].Y * RoundValue + 0.5f) / RoundValue;
			Position.Z = int32(Positions[Index].Z * RoundValue + 0.5f) / RoundValue;

			const uint64 PositionHash = SeedHash ^ FVoxelUtilities::MurmurHash(Position);
			const float Value = FVoxelUtilities::GetFraction(PositionHash);

			Result[Index] = Min + Value * (Max - Min);
		}
		return FVoxelFloatBuffer::MakeCpu(Result);
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_RandUnitVector, Result)
{
	const TValue<TBufferView<FVector>> Positions = GetBufferView(PositionPin, Query);
	const TValue<int32> Seed = Get(SeedPin, Query);
	const TValue<int32> RoundingDecimals = Get(RoundingDecimalsPin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, Positions, Seed, RoundingDecimals)
	{
		const uint64 SeedHash = FVoxelUtilities::MurmurHash64(Seed);
		const float RoundValue = FMath::Pow(10, float(RoundingDecimals));

		TVoxelArray<float> ResultX = FVoxelFloatBuffer::Allocate(Positions.Num());
		TVoxelArray<float> ResultY = FVoxelFloatBuffer::Allocate(Positions.Num());
		TVoxelArray<float> ResultZ = FVoxelFloatBuffer::Allocate(Positions.Num());

		for (int32 Index = 0; Index < Positions.Num(); Index++)
		{
			FVector3f Position;
			Position.X = int32(Positions[Index].X * RoundValue + 0.5f) / RoundValue;
			Position.Y = int32(Positions[Index].Y * RoundValue + 0.5f) / RoundValue;
			Position.Z = int32(Positions[Index].Z * RoundValue + 0.5f) / RoundValue;

			const uint64 PositionHash = SeedHash ^ FVoxelUtilities::MurmurHash(Position);
			const FRandomStream RandomStream(PositionHash);
			const FVector UnitVector = RandomStream.GetUnitVector();

			ResultX[Index] = UnitVector.X;
			ResultY[Index] = UnitVector.Y;
			ResultZ[Index] = UnitVector.Z;
		}

		return FVoxelVectorBuffer::MakeCpu(ResultX, ResultY, ResultZ);
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelPinTypeSet FVoxelNode_RunOnCPU::GetPromotionTypes(const FVoxelPin& Pin) const
{
	return FVoxelPinTypeSet::All();
}

void FVoxelNode_RunOnCPU::PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	GetPin(DataPin).SetType(NewType);
	GetPin(OutDataPin).SetType(NewType);
}

DEFINE_VOXEL_NODE(FVoxelNode_RunOnCPU, OutData)
{
	return Get(DataPin, Query.MakeCpuQuery());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelPinTypeSet FVoxelNode_RunOnGPU::GetPromotionTypes(const FVoxelPin& Pin) const
{
	return FVoxelPinTypeSet::All();
}

void FVoxelNode_RunOnGPU::PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	GetPin(DataPin).SetType(NewType);
	GetPin(OutDataPin).SetType(NewType);
}

DEFINE_VOXEL_NODE(FVoxelNode_RunOnGPU, OutData)
{
	return Get(DataPin, Query.MakeGpuQuery());
}