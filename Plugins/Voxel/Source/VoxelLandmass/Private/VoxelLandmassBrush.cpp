// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelLandmassBrush.h"
#include "VoxelLandmassBrushData.h"
#include "VoxelLandmassMeshBrush.h"
#include "VoxelLandmassUtilities.h"
#include "Nodes/VoxelPositionNodes.h"
#include "VoxelLandmassBrushImpl.ispc.generated.h"

DEFINE_VOXEL_BRUSH(FVoxelLandmassBrush);

void FVoxelLandmassBrush::CacheData_GameThread()
{
	if (!Mesh)
	{
		return;
	}

	UVoxelLandmassMeshBrush* MeshBrush = UVoxelLandmassMeshBrushAssetUserData::GetBrush(*Mesh);
	if (!MeshBrush)
	{
		return;
	}

	Data = MeshBrush->GetBrushData();
	Mesh = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelLandmassBrushImpl::FVoxelLandmassBrushImpl(const FVoxelLandmassBrush& Brush, const FMatrix& WorldToLocal)
	: TVoxelBrushImpl(Brush, WorldToLocal)
	, Data(Brush.Data.ToSharedRef())
{
	const FMatrix44f DataToLocal =
		FMatrix44f(
			FTranslationMatrix(FVector(Data->Origin)) *
			FScaleMatrix(Data->VoxelSize) *
			Brush.BrushToWorld *
			WorldToLocal);

	LocalToData = DataToLocal.Inverse();

	DataToLocalScale = DataToLocal.GetScaleVector().GetAbsMax();

	const FBox3f MeshBounds = Data->MeshBounds;

	SetBounds(FVoxelBox(
		MeshBounds
		.ExpandBy(1)
		.ShiftBy(-Data->Origin)
		.TransformBy(DataToLocal)));
}

float FVoxelLandmassBrushImpl::GetDistance(const FVector& LocalPosition) const
{
	return FMath::Abs(FVoxelLandmassUtilities::SampleBrush(*this, FVector3f(LocalPosition)));
}

TSharedPtr<FVoxelDistanceField> FVoxelLandmassBrushImpl::GetDistanceField() const
{
	const TSharedRef<FVoxelMeshDistanceField> DistanceField = MakeShared<FVoxelMeshDistanceField>();
	DistanceField->Brush = SharedThis(this);
	DistanceField->Bounds = GetBounds();
	DistanceField->Smoothness = Brush.Smoothness;
	DistanceField->bIsSubtractive = Brush.bInvert;
	DistanceField->Priority = Brush.Priority;
	return DistanceField;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE_CPU(FVoxelNode_FVoxelMeshDistanceField_GetDistances, Distance)
{
	FindVoxelQueryData(FVoxelPositionQueryData, PositionQueryData);
	const TValue<TBufferView<FVector>> Positions = PositionQueryData->GetPositions().MakeView();

	return VOXEL_ON_COMPLETE(AsyncThread, Positions)
	{
		TVoxelArray<float> Buffer = FVoxelFloatBuffer::Allocate(Positions.Num());

		const FVoxelLandmassBrushData& BrushData = *Brush->Data;
		check(BrushData.DistanceField.Num() == BrushData.Size.X * BrushData.Size.Y * BrushData.Size.Z);
		check(BrushData.Normals.Num() == BrushData.Size.X * BrushData.Size.Y * BrushData.Size.Z);

		ispc::VoxelNode_FVoxelMeshDistanceField_GetDistances(
			Positions.X.GetData(),
			Positions.Y.GetData(),
			Positions.Z.GetData(),
			Positions.X.IsConstant(),
			Positions.Y.IsConstant(),
			Positions.Z.IsConstant(),
			Positions.Num(),
			GetISPCValue(BrushData.Size),
			BrushData.DistanceField.GetData(),
			BrushData.Normals.GetData(),
			GetISPCValue(Brush->LocalToData),
			Brush->DataToLocalScale,
			Brush->Brush.DistanceOffset,
			Brush->Brush.bHermiteInterpolation,
			Buffer.GetData());

		return FVoxelFloatBuffer::MakeCpu(Buffer);
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelFutureValue<FVoxelFloatBuffer> FVoxelMeshDistanceField::GetDistances(const FVoxelQuery& Query) const
{
	return VOXEL_CALL_NODE(FVoxelNode_FVoxelMeshDistanceField_GetDistances, DistancePin)
	{
		Node.Brush = Brush;
	};
}