// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelHeightmap.h"

DEFINE_VOXEL_FACTORY(UVoxelHeightmap);
DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelHeightmapMemory);

void FVoxelHeightmap::Serialize(FArchive& Ar)
{
	VOXEL_FUNCTION_COUNTER();

	using FVersion = DECLARE_VOXEL_VERSION
	(
		FirstVersion
	);

	int32 Version = FVersion::LatestVersion;
	Ar << Version;

	Ar << SizeX;
	Ar << SizeY;

	Heights.BulkSerialize(Ar);

	ensure(SizeX * SizeY == Heights.Num());

	UpdateStats();
}

void FVoxelHeightmap::Initialize(
	int32 NewSizeX,
	int32 NewSizeY,
	TVoxelArray<uint16>&& NewHeights)
{
	check(NewSizeX * NewSizeY == NewHeights.Num());

	SizeX = NewSizeX;
	SizeY = NewSizeY;
	Heights = MoveTemp(NewHeights);
}

TSharedRef<FVoxelRDGExternalBuffer> FVoxelHeightmap::GetHeights_RenderThread() const
{
	check(IsInRenderingThread());

	if (!Heights_RenderThread)
	{
		Heights_RenderThread = FVoxelRDGExternalBuffer::CreateTyped<uint16>(Heights, PF_R16_UINT, TEXT("Heights"));
	}

	return Heights_RenderThread.ToSharedRef();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelHeightmap::Serialize(FArchive& Ar)
{
	VOXEL_FUNCTION_COUNTER_LLM();
	
	Super::Serialize(Ar);

	if (!Heightmap)
	{
		Heightmap = MakeShared<FVoxelHeightmap>();
	}

	if (!SourceHeightmap)
	{
		SourceHeightmap = MakeShared<FVoxelHeightmap>();
	}

	if (bCompress)
	{
		BulkData.SetBulkDataFlags(BULKDATA_SerializeCompressed);
		SourceBulkData.SetBulkDataFlags(BULKDATA_SerializeCompressed);
	}
	else
	{
		BulkData.ClearBulkDataFlags(BULKDATA_SerializeCompressed);
		SourceBulkData.ClearBulkDataFlags(BULKDATA_SerializeCompressed);
	}

	bool bIsCooked = Ar.IsCooking();
	Ar << bIsCooked;

	FVoxelObjectUtilities::SerializeBulkData(this, BulkData, Ar, *Heightmap);

	if (!bIsCooked)
	{
		FVoxelObjectUtilities::SerializeBulkData(this, SourceBulkData, Ar, *SourceHeightmap);
	}
}

#if WITH_EDITOR
void UVoxelHeightmap::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
	{
		UpdateHeightmap();
	}

	OnPropertyChanged.Broadcast();
}
#endif

void UVoxelHeightmap::UpdateHeightmap()
{
	VOXEL_FUNCTION_COUNTER();

	Config.InternalScaleZ = 1.f;
	Config.InternalOffsetZ = 0.f;

	if (!ensure(SourceHeightmap))
	{
		Heightmap.Reset();
		return;
	}

	if (!Config.bEnableCutoff)
	{
		Heightmap = MakeSharedCopy(*SourceHeightmap);
		return;
	}

	const int32 SizeX = SourceHeightmap->GetSizeX();
	const int32 SizeY = SourceHeightmap->GetSizeY();

	TVoxelArray<FIntPoint> Positions;
	FVoxelUtilities::SetNumFast(Positions, SizeX * SizeY);

	for (int32 Y = 0; Y < SizeY; Y++)
	{
		for (int32 X = 0; X < SizeX; X++)
		{
			const int32 Index = FVoxelUtilities::Get2DIndex<int32>(SizeX, SizeY, X, Y);
			const float Height = SourceHeightmap->GetHeight(Index);

			if (Height <= Config.Cutoff)
			{
				Positions[Index] = FIntPoint(MAX_int32);
			}
			else
			{
				Positions[Index] = FIntPoint(X, Y);
			}
		}
	}

	FVoxelUtilities::JumpFlood2D(FIntPoint(SizeX, SizeY), Positions);

	TVoxelArray<float> Heights;
	FVoxelUtilities::SetNumFast(Heights, SizeX * SizeY);

	for (int32 Y = 0; Y < SizeY; Y++)
	{
		for (int32 X = 0; X < SizeX; X++)
		{
			const int32 Index = FVoxelUtilities::Get2DIndex<int32>(SizeX, SizeY, X, Y);
			const FIntPoint Position = Positions[Index];

			if (Position == FIntPoint(X, Y) ||
				Position == MAX_int32)
			{
				Heights[Index] = SourceHeightmap->GetHeight(Index) - Config.Cutoff;
				continue;
			}

			const float Distance = FVoxelUtilities::Size(Position - FIntPoint(X, Y));

			float Height = Config.ScaleZ * (SourceHeightmap->GetHeight(Position.X, Position.Y) - Config.Cutoff);
			Height -= Config.CutoffSlope * Config.ScaleXY * Distance;
			Heights[Index] = Height / Config.ScaleZ;
		}
	}
	
	TVoxelArray<uint16> NormalizedHeights;
	FVoxelUtilities::SetNumFast(NormalizedHeights, SizeX * SizeY);
	{
		float Min = Heights[0];
		float Max = Heights[0];
		for (const float Height : Heights)
		{
			Min = FMath::Min(Min, Height);
			Max = FMath::Max(Max, Height);
		}

		for (int32 Index = 0; Index < SizeX * SizeY; Index++)
		{
			const float Value = (Heights[Index] - Min) / (Max - Min);
			NormalizedHeights[Index] = FVoxelUtilities::ClampToUINT16(FMath::RoundToInt(MAX_uint16 * Value));
		}

		Config.InternalScaleZ = Max - Min;
		Config.InternalOffsetZ = Min;
	}

	Heightmap = MakeShared<FVoxelHeightmap>();
	Heightmap->Initialize(SizeX, SizeY, MoveTemp(NormalizedHeights));

	MarkPackageDirty();

	OnHeightmapUpdated.Broadcast();
}