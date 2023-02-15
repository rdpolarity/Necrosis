// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelPositionNodes.h"
#include "VoxelBufferUtilities.h"
#include "VoxelMetaGraphRuntimeUtilities.h"

void FVoxelGradientPositionQueryData::Initialize(
	bool bInIs2D,
	EVoxelAxis Axis,
	const FVoxelVectorBuffer& Positions,
	const FVoxelGradientPositionQueryData* ExistingQueryData)
{
	bIs2D = bInIs2D;

	switch (Axis)
	{
	default: ensure(false);
	case EVoxelAxis::X: StrideX = 1; break;
	case EVoxelAxis::Y: StrideY = 1; break;
	case EVoxelAxis::Z: StrideZ = 1; break;
	}
	
	PrivatePositions = Positions;

	if (ExistingQueryData)
	{
		if (ExistingQueryData->StrideX != -1)
		{
			ensure(StrideX == -1);
			StrideX = 2 * ExistingQueryData->StrideX;
			ensure(StrideX <= 4);
		}
		if (ExistingQueryData->StrideY != -1)
		{
			ensure(StrideY == -1);
			StrideY = 2 * ExistingQueryData->StrideY;
			ensure(StrideY <= 4);
		}
		if (ExistingQueryData->StrideZ != -1)
		{
			ensure(StrideZ == -1);
			StrideZ = 2 * ExistingQueryData->StrideZ;
			ensure(StrideZ <= 4);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<FVoxelPositionQueryData> FVoxelDensePositionQueryData::TryCull(const FVoxelBox& Bounds) const
{
	VOXEL_FUNCTION_COUNTER();

	if (Bounds.Contains(GetBounds()))
	{
		return nullptr;
	}

	const double Step = double(PrivateStep);

	FVoxelBox NewBounds = Bounds.Overlap(GetBounds());
	NewBounds.ShiftBy(-FVector(PrivateStart));
	NewBounds.Min = FVector(FVoxelUtilities::CeilToInt(NewBounds.Min / Step)) * Step;
	NewBounds.Max = FVector(FVoxelUtilities::FloorToInt(NewBounds.Max / Step)) * Step;
	NewBounds.ShiftBy(FVector(PrivateStart));

	const FIntVector Size = FVoxelUtilities::RoundToInt(NewBounds.Size() / Step);

	const TSharedRef<FVoxelDensePositionQueryData> NewPositionQueryData = MakeShared<FVoxelDensePositionQueryData>();
	NewPositionQueryData->Initialize(
		FVector3f(NewBounds.Min),
		PrivateStep,
		Size);
	return NewPositionQueryData;
}

BEGIN_VOXEL_NAMESPACE(MetaGraph)

BEGIN_VOXEL_COMPUTE_SHADER(WritePositions)
	VOXEL_SHADER_PARAMETER_CST(FVector3f, Start)
	VOXEL_SHADER_PARAMETER_CST(float, Step)
	VOXEL_SHADER_PARAMETER_CST(FIntVector, BlockSize)

	VOXEL_SHADER_PARAMETER_UAV(Buffer<float>, OutPositionX)
	VOXEL_SHADER_PARAMETER_UAV(Buffer<float>, OutPositionY)
	VOXEL_SHADER_PARAMETER_UAV(Buffer<float>, OutPositionZ)
END_VOXEL_SHADER()

END_VOXEL_NAMESPACE(MetaGraph)

void FVoxelDensePositionQueryData::Initialize(
	const FVector3f& Start,
	const float Step,
	const FIntVector& Size)
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_USE_NAMESPACE(MetaGraph);

	PrivateStart = Start;
	PrivateStep = Step;
	PrivateSize = Size;

	if (!ensure(PrivateSize % 2 == 0))
	{
		PrivateSize = PrivateSize + 1;
	}

	if (IsInRenderingThread())
	{
		CachedPositions.X = FVoxelFloatBuffer::MakeGpu(PrivateSize.X * PrivateSize.Y * PrivateSize.Z);
		CachedPositions.Y = FVoxelFloatBuffer::MakeGpu(PrivateSize.X * PrivateSize.Y * PrivateSize.Z);
		CachedPositions.Z = FVoxelFloatBuffer::MakeGpu(PrivateSize.X * PrivateSize.Y * PrivateSize.Z);

		FRDGBuilder& GraphBuilder = FVoxelRDGBuilderScope::Get();

		BEGIN_VOXEL_SHADER_CALL(WritePositions)
		{
			ensure(PrivateSize % 2 == 0);
			const FIntVector BlockSize = FIntVector(PrivateSize) / 2;

			Parameters.Start = Start;
			Parameters.Step = Step;
			Parameters.BlockSize = BlockSize;

			Parameters.OutPositionX = CachedPositions.X.GetGpuBuffer();
			Parameters.OutPositionY = CachedPositions.Y.GetGpuBuffer();
			Parameters.OutPositionZ = CachedPositions.Z.GetGpuBuffer();

			Execute(FComputeShaderUtils::GetGroupCount(BlockSize, 4));
		}
		END_VOXEL_SHADER_CALL()
	}
	else
	{
		TVoxelArray<float> WriteX = FVoxelFloatBuffer::Allocate(PrivateSize.X * PrivateSize.Y * PrivateSize.Z);
		TVoxelArray<float> WriteY = FVoxelFloatBuffer::Allocate(PrivateSize.X * PrivateSize.Y * PrivateSize.Z);
		TVoxelArray<float> WriteZ = FVoxelFloatBuffer::Allocate(PrivateSize.X * PrivateSize.Y * PrivateSize.Z);

		FRuntimeUtilities::WritePositions(
			WriteX,
			WriteY,
			WriteZ,
			PrivateStart,
			PrivateStep,
			PrivateSize);

		CachedPositions = FVoxelVectorBuffer::MakeCpu(WriteX, WriteY, WriteZ);
	}

	CachedPositions.SetBounds(GetBounds());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelDense2DPositionQueryData::Initialize(
	const FVector2f& Start,
	const float Step,
	const FIntPoint& Size)
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_USE_NAMESPACE(MetaGraph);

	PrivateStart = Start;
	PrivateStep = Step;
	PrivateSize = Size;

	if (!ensure(PrivateSize % 2 == 0))
	{
		PrivateSize = PrivateSize + 1;
	}

	TVoxelArray<float> WriteX = FVoxelFloatBuffer::Allocate(PrivateSize.X * PrivateSize.Y);
	TVoxelArray<float> WriteY = FVoxelFloatBuffer::Allocate(PrivateSize.X * PrivateSize.Y);

	FRuntimeUtilities::WritePositions2D(
		WriteX,
		WriteY,
		PrivateStart,
		PrivateStep,
		PrivateSize);

	CachedPositions.X = FVoxelFloatBuffer::MakeCpu(WriteX);
	CachedPositions.Y = FVoxelFloatBuffer::MakeCpu(WriteY);
	CachedPositions.Z = FVoxelFloatBuffer::Constant(0.f);

	CachedPositions.SetBounds(GetBounds().ToBox3D(0.f, 0.f));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelSparsePositionQueryData::Initialize(const FVoxelVectorBuffer& Positions)
{
	ensure(Positions.IsValid());
	PrivatePositions = Positions;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_GetPosition3D, Position)
{
	FindVoxelQueryData(FVoxelPositionQueryData, PositionQueryData);

	if (PositionQueryData->Is2D())
	{
		VOXEL_MESSAGE(Error, "{0}: Using GetPosition on 2D positions, please use GetPosition2D instead", this);
	}

	return PositionQueryData->GetPositions();
}

DEFINE_VOXEL_NODE(FVoxelNode_GetPosition2D, Position)
{
	FindVoxelQueryData(FVoxelPositionQueryData, PositionQueryData);

	const FVoxelVectorBuffer Positions = PositionQueryData->GetPositions();

	FVoxelVector2DBuffer Positions2D;
	Positions2D.X = Positions.X;
	Positions2D.Y = Positions.Y;
	return Positions2D;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_QueryWithGradientStep, OutData)
{
	const TValue<float> Step = Get(StepPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Step)
	{
		FVoxelQuery ChildQuery = Query;
		ChildQuery.Add<FVoxelGradientStepQueryData>().Step = Step;
		return Get(DataPin, ChildQuery);
	};
}

FVoxelPinTypeSet FVoxelNode_QueryWithGradientStep::GetPromotionTypes(const FVoxelPin& Pin) const
{
	return FVoxelPinTypeSet::All();
}

void FVoxelNode_QueryWithGradientStep::PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	GetPin(DataPin).SetType(NewType);
	GetPin(OutDataPin).SetType(NewType);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_QueryWithPosition, OutData)
{
	const TValue<FVoxelVectorBuffer> Positions = Get(PositionPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Positions)
	{
		FVoxelQuery ChildQuery = Query;
		ChildQuery.Add<FVoxelSparsePositionQueryData>().Initialize(Positions);
		return Get(DataPin, ChildQuery);
	};
}

FVoxelPinTypeSet FVoxelNode_QueryWithPosition::GetPromotionTypes(const FVoxelPin& Pin) const
{
	return FVoxelPinTypeSet::All();
}

void FVoxelNode_QueryWithPosition::PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	GetPin(DataPin).SetType(NewType);
	GetPin(OutDataPin).SetType(NewType);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_Query2D, OutData)
{
	const TSharedPtr<const FVoxelDensePositionQueryData> PositionQueryData = Query.Find<FVoxelDensePositionQueryData>();
	if (!PositionQueryData ||
		PositionQueryData->GetSize().Z <= 1)
	{
		return Get(DataPin, Query);
	}

	FVoxelQuery ChildQuery = Query;
	ChildQuery.Add<FVoxelDense2DPositionQueryData>().Initialize(
		FVector2f(PositionQueryData->GetStart()),
		PositionQueryData->GetStep(),
		FIntPoint(PositionQueryData->GetSize().X, PositionQueryData->GetSize().Y));
	
	const TValue<FVoxelBuffer> DataBuffer = Get<FVoxelBuffer>(DataPin, ChildQuery);
	
	return VOXEL_ON_COMPLETE(AnyThread, PositionQueryData, DataBuffer)
	{
		const TValue<FVoxelBufferView> DataView = DataBuffer->MakeGenericView();

		return VOXEL_ON_COMPLETE(AsyncThread, PositionQueryData, DataBuffer, DataView)
		{
			if (DataView->IsConstant())
			{
				return FVoxelSharedPinValue::Make(DataBuffer);
			}

			FVoxelPinValue Result = FVoxelPinValue(FVoxelPinType::MakeStruct(DataBuffer->GetStruct()));
			Result.Get<FVoxelBuffer>().ForeachBufferPair(*DataBuffer, [&](FVoxelTerminalBuffer& ResultBuffer, const FVoxelTerminalBuffer& DataTerminalBuffer)
			{
				ResultBuffer = FVoxelBufferUtilities::ExpandQuery2D_Cpu(DataTerminalBuffer.MakeView().Get_CheckCompleted(), PositionQueryData->GetSize().Z);
			});
			return FVoxelSharedPinValue(Result);
		};
	};
}

FVoxelPinTypeSet FVoxelNode_Query2D::GetPromotionTypes(const FVoxelPin& Pin) const
{
	FVoxelPinTypeSet OutTypes;
	OutTypes.Add(FVoxelPinType::GetAllBufferTypes());
	return OutTypes;
}

void FVoxelNode_Query2D::PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	GetPin(DataPin).SetType(NewType);
	GetPin(OutDataPin).SetType(NewType);
}