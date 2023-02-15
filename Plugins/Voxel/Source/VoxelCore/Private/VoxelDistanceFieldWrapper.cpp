﻿// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelDistanceFieldWrapper.h"

void FVoxelDistanceFieldWrapper::FMip::Initialize(const FVoxelDistanceFieldWrapper& Wrapper)
{
	LocalToVolumeScale = Wrapper.LocalToVolumeScale;
	
	// Expand to guarantee one voxel border for gradient reconstruction using bilinear filtering
	const FVector TexelObjectSpaceSize = Wrapper.LocalSpaceMeshBounds.GetSize() / FVector(IndirectionSize * DistanceField::UniqueDataBrickSize - FIntVector(2 * DistanceField::MeshDistanceFieldObjectBorder));
	const FBox DistanceFieldVolumeBounds = Wrapper.LocalSpaceMeshBounds.ExpandBy(TexelObjectSpaceSize);

	const FVector IndirectionVoxelSize = DistanceFieldVolumeBounds.GetSize() / FVector(IndirectionSize);

	const FVector VolumeSpaceDistanceFieldVoxelSize = IndirectionVoxelSize * LocalToVolumeScale / FVector(DistanceField::UniqueDataBrickSize);
	const float MaxDistanceForEncoding = VolumeSpaceDistanceFieldVoxelSize.Size() * DistanceField::BandSizeInVoxels;
	DistanceFieldToVolumeScaleBias = FVector2D(2.0f * MaxDistanceForEncoding, -MaxDistanceForEncoding);
}

FVoxelDistanceFieldWrapper::FBrick* FVoxelDistanceFieldWrapper::FMip::FindBrick(const FIntVector& Position)
{
	return Bricks[FVoxelUtilities::Get3DIndex(IndirectionSize, Position)].Get();
}

FVoxelDistanceFieldWrapper::FBrick& FVoxelDistanceFieldWrapper::FMip::FindOrAddBrick(const FIntVector& Position)
{
	TSharedPtr<FBrick>& Ptr = Bricks[FVoxelUtilities::Get3DIndex(IndirectionSize, Position)];
	if (!Ptr)
	{
		Ptr = MakeShared<FBrick>(NoInit);
	}
	return *Ptr;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelDistanceFieldWrapper::SetSize(const FIntVector& Mip0IndirectionSize)
{
	for (int32 MipIndex = 0; MipIndex < DistanceField::NumMips; MipIndex++)
	{
		Mips[MipIndex].IndirectionSize = FIntVector(
			FVoxelUtilities::DivideCeil_Positive(Mip0IndirectionSize.X, 1 << MipIndex),
			FVoxelUtilities::DivideCeil_Positive(Mip0IndirectionSize.Y, 1 << MipIndex),
			FVoxelUtilities::DivideCeil_Positive(Mip0IndirectionSize.Z, 1 << MipIndex));

		Mips[MipIndex].Bricks.Reset();
		Mips[MipIndex].Bricks.SetNum(Mips[MipIndex].IndirectionSize.X * Mips[MipIndex].IndirectionSize.Y * Mips[MipIndex].IndirectionSize.Z);

		Mips[MipIndex].Initialize(*this);
	}
}

TSharedRef<FDistanceFieldVolumeData> FVoxelDistanceFieldWrapper::Build() const
{
	VOXEL_FUNCTION_COUNTER();
	
	const TSharedRef<FDistanceFieldVolumeData> OutData = MakeShared<FDistanceFieldVolumeData>();

	TArray<uint8> StreamableMipData;

	for (int32 MipIndex = 0; MipIndex < DistanceField::NumMips; MipIndex++)
	{
		VOXEL_SCOPE_COUNTER("Mip");
		
		const FMip& Mip = Mips[MipIndex];

		FSparseDistanceFieldMip& OutMip = OutData->Mips[MipIndex];
		TVoxelArray<uint32> IndirectionTable;
		IndirectionTable.Empty(Mip.IndirectionSize.X * Mip.IndirectionSize.Y * Mip.IndirectionSize.Z);
		IndirectionTable.AddUninitialized(Mip.IndirectionSize.X * Mip.IndirectionSize.Y * Mip.IndirectionSize.Z);

		for (int32 i = 0; i < IndirectionTable.Num(); i++)
		{
			IndirectionTable[i] = DistanceField::InvalidBrickIndex;
		}

		uint32 NumBricks = 0;
		for (const TSharedPtr<FBrick>& Brick : Mip.Bricks)
		{
			if (Brick)
			{
				NumBricks++;
			}
		}

		const uint32 BrickSizeBytes = DistanceField::BrickSize * DistanceField::BrickSize * DistanceField::BrickSize * GPixelFormats[DistanceField::DistanceFieldFormat].BlockBytes;

		TVoxelArray<uint8> DistanceFieldBrickData;
		DistanceFieldBrickData.Empty(BrickSizeBytes * NumBricks);
		DistanceFieldBrickData.AddUninitialized(BrickSizeBytes * NumBricks);

		int32 BrickIndex = 0;
		for (int32 X = 0; X < Mip.IndirectionSize.X; X++)
		{
			for (int32 Y = 0; Y < Mip.IndirectionSize.X; Y++)
			{
				for (int32 Z = 0; Z < Mip.IndirectionSize.X; Z++)
				{
					const int32 IndirectionIndex = FVoxelUtilities::Get3DIndex(Mip.IndirectionSize, X, Y, Z);
					const TSharedPtr<FBrick>& Brick = Mip.Bricks[IndirectionIndex];

					if (!Brick)
					{
						IndirectionTable[IndirectionIndex] = DistanceField::InvalidBrickIndex;
						continue;
					}
					
					IndirectionTable[IndirectionIndex] = BrickIndex;
					FMemory::Memcpy(&DistanceFieldBrickData[BrickIndex * BrickSizeBytes], Brick->GetData(), Brick->Num() * Brick->GetTypeSize());

					BrickIndex++;
				}
			}
		}
		check(BrickIndex == NumBricks);

		const int32 IndirectionTableBytes = IndirectionTable.Num() * IndirectionTable.GetTypeSize();
		const int32 MipDataBytes = IndirectionTableBytes + DistanceFieldBrickData.Num();

		if (MipIndex == DistanceField::NumMips - 1)
		{
			VOXEL_SCOPE_COUNTER("Copy");
			
			OutData->AlwaysLoadedMip.Empty(MipDataBytes);
			OutData->AlwaysLoadedMip.AddUninitialized(MipDataBytes);

			FPlatformMemory::Memcpy(&OutData->AlwaysLoadedMip[0], IndirectionTable.GetData(), IndirectionTableBytes);

			if (DistanceFieldBrickData.Num() > 0)
			{
				FPlatformMemory::Memcpy(&OutData->AlwaysLoadedMip[IndirectionTableBytes], DistanceFieldBrickData.GetData(), DistanceFieldBrickData.Num());
			}
		}
		else
		{
			VOXEL_SCOPE_COUNTER("Copy");
			
			OutMip.BulkOffset = StreamableMipData.Num();
			StreamableMipData.AddUninitialized(MipDataBytes);
			OutMip.BulkSize = StreamableMipData.Num() - OutMip.BulkOffset;
			check(OutMip.BulkSize > 0);

			FPlatformMemory::Memcpy(&StreamableMipData[OutMip.BulkOffset], IndirectionTable.GetData(), IndirectionTableBytes);

			if (DistanceFieldBrickData.Num() > 0)
			{
				FPlatformMemory::Memcpy(&StreamableMipData[OutMip.BulkOffset + IndirectionTableBytes], DistanceFieldBrickData.GetData(), DistanceFieldBrickData.Num());
			}
		}

		OutMip.IndirectionDimensions = Mip.IndirectionSize;
		OutMip.DistanceFieldToVolumeScaleBias = Mip.DistanceFieldToVolumeScaleBias;
		OutMip.NumDistanceFieldBricks = NumBricks;

		// Account for the border voxels we added
		const FVector VirtualUVMin = FVector(DistanceField::MeshDistanceFieldObjectBorder) / FVector(Mip.IndirectionSize * DistanceField::UniqueDataBrickSize);
		const FVector VirtualUVSize = FVector(Mip.IndirectionSize * DistanceField::UniqueDataBrickSize - FIntVector(2 * DistanceField::MeshDistanceFieldObjectBorder)) / FVector(Mip.IndirectionSize * DistanceField::UniqueDataBrickSize);

		const FVector VolumePositionExtent = LocalSpaceMeshBounds.GetExtent() * LocalToVolumeScale;

		// [-VolumePositionExtent, VolumePositionExtent] -> [VirtualUVMin, VirtualUVMin + VirtualUVSize]
		OutMip.VolumeToVirtualUVScale = VirtualUVSize / (2.f * VolumePositionExtent);
		OutMip.VolumeToVirtualUVAdd = VolumePositionExtent * OutMip.VolumeToVirtualUVScale + VirtualUVMin;
	}

	OutData->LocalSpaceMeshBounds = LocalSpaceMeshBounds.ShiftBy(-FVector(0.5f));
	OutData->bMostlyTwoSided = true;
	
	VOXEL_SCOPE_COUNTER("Final copy");
	OutData->StreamableMips.Lock(LOCK_READ_WRITE);
	uint8* Ptr = static_cast<uint8*>(OutData->StreamableMips.Realloc(StreamableMipData.Num()));
	FMemory::Memcpy(Ptr, StreamableMipData.GetData(), StreamableMipData.Num());
	OutData->StreamableMips.Unlock();
	OutData->StreamableMips.SetBulkDataFlags(BULKDATA_Force_NOT_InlinePayload);

	return OutData;
}