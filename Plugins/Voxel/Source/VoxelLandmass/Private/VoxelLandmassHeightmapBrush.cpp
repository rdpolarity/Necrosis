// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelLandmassHeightmapBrush.h"
#include "Nodes/VoxelPositionNodes.h"
#include "VoxelLandmassHeightmapBrushImpl.ispc.generated.h"

DEFINE_VOXEL_BRUSH(FVoxelLandmassHeightmapBrush);

void FVoxelLandmassHeightmapBrush::CacheData_GameThread()
{
	check(IsInGameThread());

	if (!Heightmap)
	{
		return;
	}

	HeightmapPtr = Heightmap->Heightmap;
	HeightmapConfig = Heightmap->Config;
	Heightmap = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelLandmassHeightmapBrushImpl::FVoxelLandmassHeightmapBrushImpl(const FVoxelLandmassHeightmapBrush& Brush, const FMatrix& WorldToLocal)
	: TVoxelBrushImpl(Brush, WorldToLocal)
	, Heightmap(Brush.HeightmapPtr.ToSharedRef())
{
	const FVoxelHeightmapConfig& Config = Brush.HeightmapConfig;

	const FTransform Transform = FTransform(Brush.BrushToWorld * WorldToLocal);
	const FQuat LocalRotation = Transform.GetRotation();

	FQuat Swing;
	FQuat Twist;
	LocalRotation.ToSwingTwist(FVector::UnitZ(), Swing, Twist);

	// Go through euler angles to avoid flipping at 240 degrees
	const float AngleZ = FMath::DegreesToRadians(Twist.Euler().Z);

	Scale = FVector2D(Transform.GetScale3D()) * Config.ScaleXY;
	Rotation = FQuat2D(AngleZ);
	Position = FVector2D(Transform.GetTranslation());
	Rotation3D = FVector2D(
		FMath::Tan(Swing.GetTwistAngle(FVector::UnitY())),
		FMath::Tan(Swing.GetTwistAngle(-FVector::UnitX())));

	InnerScaleZ = Config.ScaleZ * Config.InternalScaleZ;
	InnerOffsetZ = Config.ScaleZ * Config.InternalOffsetZ;

	ScaleZ = Transform.GetScale3D().Z;
	OffsetZ = Transform.GetTranslation().Z;
	
	const FVoxelBox2D Bounds(
		-FVector2d(Heightmap->GetSizeX(), Heightmap->GetSizeY()) / 2,
		FVector2d(Heightmap->GetSizeX(), Heightmap->GetSizeY()) / 2);

	// TODO Compute proper Z bounds
	SetBounds(Bounds.Scale(Scale).TransformBy(Rotation).ShiftBy(Position).ToBox3D(-1e50, 1e50));
}

float FVoxelLandmassHeightmapBrushImpl::GetDistance(const FVector& LocalPosition) const
{
	// TODO Make sure logic is same as ispc one

	FVector2D InPosition = FVector2D(LocalPosition);

	InPosition -= Position;
	InPosition = Rotation.Inverse().TransformPoint(InPosition);
	InPosition /= Scale;

	InPosition.X += Heightmap->GetSizeX() / 2.f;
	InPosition.Y += Heightmap->GetSizeY() / 2.f;

	const FIntPoint Min = FVoxelUtilities::Clamp(FVoxelUtilities::FloorToInt(InPosition), 0, FIntPoint(Heightmap->GetSizeX(), Heightmap->GetSizeY()) - 2);
	const FIntPoint Max = Min + 1;
	const FVector2D Alpha = FVoxelUtilities::Clamp(InPosition - FVector2D(Min), 0.f, 1.f);

	float Height = FVoxelUtilities::BilinearInterpolation(
		Heightmap->GetHeight(Min.X, Min.Y),
		Heightmap->GetHeight(Max.X, Min.Y),
		Heightmap->GetHeight(Min.X, Max.Y),
		Heightmap->GetHeight(Max.X, Max.Y),
		Alpha.X,
		Alpha.Y);

	Height *= InnerScaleZ;
	Height += InnerOffsetZ;

	if (Brush.bSubtractive)
	{
		Height *= -1;
	}

	Height *= ScaleZ;
	Height += OffsetZ;

	return FMath::Abs(LocalPosition.Z - Height);
}

TSharedPtr<FVoxelDistanceField> FVoxelLandmassHeightmapBrushImpl::GetDistanceField() const
{
	const TSharedRef<FVoxelHeightmapDistanceField> DistanceField = MakeShared<FVoxelHeightmapDistanceField>();
	DistanceField->Brush = SharedThis(this);
	DistanceField->bIsHeightmap = true;
	DistanceField->Bounds = GetBounds();
	DistanceField->Smoothness = Brush.Smoothness;
	DistanceField->bIsSubtractive = Brush.bSubtractive;
	DistanceField->Priority = Brush.Priority;
	return DistanceField;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE_CPU(FVoxelNode_FVoxelHeightmapDistanceField_GetDistances, Distance)
{
	FindVoxelQueryData(FVoxelPositionQueryData, PositionQueryData);
	const TValue<TBufferView<FVector>> Positions = PositionQueryData->GetPositions().MakeView();

	return VOXEL_ON_COMPLETE(AsyncThread, Positions)
	{
		TVoxelArray<float> Buffer = FVoxelFloatBuffer::Allocate(Positions.Num());
		
		const FVector2f Rotation = Brush->Rotation.Inverse().GetVector();

		// TODO Check if PositionQueryData is dense and optimize ISPC based on that

		ispc::VoxelNode_FVoxelHeightmapDistanceField_GetDistances(
			Positions.X.GetData(),
			Positions.X.IsConstant(),
			Positions.Y.GetData(),
			Positions.Y.IsConstant(),
			Positions.Z.GetData(),
			Positions.Z.IsConstant(),
			Brush->Position.X,
			Brush->Position.Y,
			Rotation.X,
			Rotation.Y,
			Brush->Rotation3D.X,
			Brush->Rotation3D.Y,
			Brush->Scale.X,
			Brush->Scale.Y,
			Brush->InnerScaleZ,
			Brush->InnerOffsetZ,
			Brush->ScaleZ,
			Brush->OffsetZ,
			Brush->Brush.bSubtractive,
			Brush->Heightmap->GetSizeX(),
			Brush->Heightmap->GetSizeY(),
			Brush->Heightmap->GetHeights().GetData(),
			Buffer.Num(),
			Buffer.GetData());

		return FVoxelFloatBuffer::MakeCpu(Buffer);
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelFutureValue<FVoxelFloatBuffer> FVoxelHeightmapDistanceField::GetDistances(const FVoxelQuery& Query) const
{
	return VOXEL_CALL_NODE(FVoxelNode_FVoxelHeightmapDistanceField_GetDistances, DistancePin)
	{
		Node.Brush = Brush;
	};
}