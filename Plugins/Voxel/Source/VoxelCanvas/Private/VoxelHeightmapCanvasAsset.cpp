// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelHeightmapCanvasAsset.h"
#include "VoxelHeightmapCanvasData.h"
#include "VoxelDependencyManager.h"

DEFINE_VOXEL_FACTORY(UVoxelHeightmapCanvasAsset);

void UVoxelHeightmapCanvasAsset::Serialize(FArchive& Ar)
{
	VOXEL_FUNCTION_COUNTER_LLM();

	Super::Serialize(Ar);

	if (!Data)
	{
		Data = MakeShared<FVoxelHeightmapCanvasData>();
	}

	if (bCompress)
	{
		BulkData.SetBulkDataFlags(BULKDATA_SerializeCompressed);
	}
	else
	{
		BulkData.ClearBulkDataFlags(BULKDATA_SerializeCompressed);
	}

	FVoxelObjectUtilities::SerializeBulkData(this, BulkData, Ar, *Data);
}

#if WITH_EDITOR
void UVoxelHeightmapCanvasAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FixupData();
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelHeightmapCanvasAsset::PaintCircle(
	const FVector2D Center, 
	const float Radius, 
	const float Falloff, 
	const EVoxelFalloff FalloffType, 
	const float Strength, 
	const float Value,
	EVoxelHeightmapCanvasPaintBehavior Behavior)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(Data) ||
		!ensure(Data->GetSize() == Size))
	{
		return;
	}

	const TVoxelArrayView<float> DataView = Data->GetData();

	const FIntPoint Min = FVoxelUtilities::Clamp(FVoxelUtilities::FloorToInt(Center - Radius), FIntPoint::ZeroValue, Size - 1);
	const FIntPoint Max = FVoxelUtilities::Clamp(FVoxelUtilities::CeilToInt(Center + Radius), FIntPoint::ZeroValue, Size - 1);

	FVoxelOptionalBox2D Bounds;
	for (int32 X = Min.X; X <= Max.X; X++)
	{
		for (int32 Y = Min.Y; Y <= Max.Y; Y++)
		{
			const float Distance = FVector2f::Distance(FVector2f(X, Y), FVector2f(Center));
			const float FinalStrength =
				FMath::Clamp(Strength, 0.f, 1.f) *
				FVoxelUtilities::GetFalloff(FalloffType, Distance, Radius, Falloff);

			if (FinalStrength == 0.f)
			{
				continue;
			}

			Bounds += FVector2D(X, Y);

			const int32 Index = FVoxelUtilities::Get2DIndex<int32>(Size, X, Y);
			const float NewValue = FMath::Lerp(DataView[Index], Value, FinalStrength);
			ApplyBehavior(DataView[Index], NewValue, Behavior);
		}
	}

	if (Bounds.IsValid())
	{
		Update(Bounds.GetBox());
	}
}

void UVoxelHeightmapCanvasAsset::PaintSquare(
	const FVector2D Center,
	const float Extent, 
	const float Value,
	EVoxelHeightmapCanvasPaintBehavior Behavior)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(Data) ||
		!ensure(Data->GetSize() == Size))
	{
		return;
	}

	const TVoxelArrayView<float> DataView = Data->GetData();
	
	const FIntPoint Min = FVoxelUtilities::Clamp(FVoxelUtilities::FloorToInt(Center - Extent), FIntPoint::ZeroValue, Size - 1);
	const FIntPoint Max = FVoxelUtilities::Clamp(FVoxelUtilities::CeilToInt(Center + Extent), FIntPoint::ZeroValue, Size - 1);
	
	for (int32 X = Min.X; X <= Max.X; X++)
	{
		for (int32 Y = Min.Y; Y <= Max.Y; Y++)
		{
			const int32 Index = FVoxelUtilities::Get2DIndex<int32>(Size, X, Y);
			ApplyBehavior(DataView[Index], Value, Behavior);
		}
	}

	Update(FVoxelBox2D(FVector2D(Min), FVector2D(Max)));
}

void UVoxelHeightmapCanvasAsset::PaintRamp(
	const FVector2D Start,
	const FVector2D End,
	const float Width, 
	const float StartValue,
	const float EndValue,
	EVoxelHeightmapCanvasPaintBehavior Behavior)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(Data) ||
		!ensure(Data->GetSize() == Size))
	{
		return;
	}

	const TVoxelArrayView<float> DataView = Data->GetData();
	
	const FIntPoint Min = FVoxelUtilities::Clamp(FVoxelUtilities::FloorToInt(FVoxelUtilities::ComponentMin(Start, End) - Width), FIntPoint::ZeroValue, Size - 1);
	const FIntPoint Max = FVoxelUtilities::Clamp(FVoxelUtilities::CeilToInt(FVoxelUtilities::ComponentMax(Start, End) + Width), FIntPoint::ZeroValue, Size - 1);

	const FVector2f Direction = FVector2f((End - Start).GetSafeNormal());
	const float Length = (End - Start).Size();

	FVoxelOptionalBox2D Bounds;
	for (int32 X = Min.X; X <= Max.X; X++)
	{
		for (int32 Y = Min.Y; Y <= Max.Y; Y++)
		{
			const FVector2f Position(X, Y);
			const float Time = FVector2f::DotProduct(Position - FVector2f(Start), Direction);
			if (Time < 0 || Time > Length)
			{
				continue;
			}

			const FVector2f ProjectedPoint = FVector2f(Start) + Time * Direction;
			const float Distance = FVector2f::Distance(ProjectedPoint, Position);
			if (Distance > Width)
			{
				continue;
			}

			Bounds += FVector2D(X, Y);

			const int32 Index = FVoxelUtilities::Get2DIndex<int32>(Size, X, Y);
			const float NewValue = FMath::Lerp(StartValue, EndValue, Time / Length);
			ApplyBehavior(DataView[Index], NewValue, Behavior);
		}
	}

	if (Bounds.IsValid())
	{
		Update(Bounds.GetBox());
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelHeightmapCanvasAsset::Update(const FVoxelBox2D& Bounds) const
{
	VOXEL_FUNCTION_COUNTER();

	ensure(false);
	// GVoxelBrushRegistry->GetBrushes<xxx>()
	// Find used layer names
	// Iterate brush subsystems, trigger updates
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelHeightmapCanvasAsset::FixupData()
{
	if (!Data)
	{
		Data = MakeShared<FVoxelHeightmapCanvasData>();
	}

	Size = FVoxelUtilities::Clamp(Size, 1, 4096);

	if (Data->GetSize() != Size)
	{
		Data = MakeShared<FVoxelHeightmapCanvasData>();
		Data->Initialize(Size);
	}
}