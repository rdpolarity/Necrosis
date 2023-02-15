// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "DistanceFieldAtlas.h"

class VOXELCORE_API FVoxelDistanceFieldWrapper
{
public:
	using FBrick = TVoxelStaticArray<uint8, DistanceField::BrickSize * DistanceField::BrickSize * DistanceField::BrickSize>;

	class FMip
	{
	public:
		void Initialize(const FVoxelDistanceFieldWrapper& Wrapper);

		FBrick* FindBrick(const FIntVector& Position);
		FBrick& FindOrAddBrick(const FIntVector& Position);

		FORCEINLINE uint8 QuantizeDistance(float Distance) const
		{
			// Transform to the tracing shader's Volume space
			const float VolumeSpaceDistance = Distance * LocalToVolumeScale;
			// Transform to the Distance Field texture's space
			const float RescaledDistance = (VolumeSpaceDistance - DistanceFieldToVolumeScaleBias.Y) / DistanceFieldToVolumeScaleBias.X;
			checkVoxelSlow(DistanceField::DistanceFieldFormat == PF_G8);

			return FMath::Clamp<int32>(FMath::FloorToInt(RescaledDistance * 255.0f + .5f), 0, 255);
		}

	private:
		float LocalToVolumeScale;
		FVector2D DistanceFieldToVolumeScaleBias;
		FIntVector IndirectionSize = FIntVector::ZeroValue;
		TVoxelArray<TSharedPtr<FBrick>> Bricks;

		friend class FVoxelDistanceFieldWrapper;
	};

	const FBox LocalSpaceMeshBounds;
	const float LocalToVolumeScale;
	
	TVoxelStaticArray<FMip, DistanceField::NumMips> Mips{ ForceInit };

	explicit FVoxelDistanceFieldWrapper(FBox LocalSpaceMeshBounds)
		: LocalSpaceMeshBounds(LocalSpaceMeshBounds)
		, LocalToVolumeScale(1.0f / LocalSpaceMeshBounds.GetExtent().GetMax())
	{
	}

	void SetSize(const FIntVector& Mip0IndirectionSize);
	TSharedRef<FDistanceFieldVolumeData> Build() const;
};