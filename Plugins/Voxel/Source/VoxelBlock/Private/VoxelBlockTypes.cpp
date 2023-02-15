// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelBlockTypes.h"

#if VOXEL_DEBUG
VOXEL_RUN_ON_STARTUP_GAME(TestVoxelBlockRotation)
{
	for (int32 Index = 0; Index < 24; Index++)
	{
		const EVoxelBlockRotation Rotation = EVoxelBlockRotation(Index);
		ensure(FVoxelBlockRotation::FromRotator(FVoxelBlockRotation::ToRotator(Rotation)) == Rotation);
	}

	for (int32 Index = 0; Index < 6; Index++)
	{
		const EVoxelBlockFace Face = EVoxelBlockFace(Index);
		ensure(FVoxelBlockRotation::VectorToCubicFace(FVoxelBlockRotation::CubicFaceToVector(Face)) == Face);
	}
}
#endif

EVoxelBlockRotation FVoxelBlockRotation::FromRotator(FRotator Rotation)
{
	Rotation.Pitch = FMath::RoundToInt(Rotation.Pitch / 90) * 90;
	Rotation.Yaw = FMath::RoundToInt(Rotation.Yaw / 90) * 90;
	Rotation.Roll = FMath::RoundToInt(Rotation.Roll / 90) * 90;
	
	const FVector Indices = Rotation.RotateVector(FVector(1, 2, 3));

	const int32 X = FMath::RoundToInt(Indices.X);
	const int32 Y = FMath::RoundToInt(Indices.Y);
	const int32 Z = FMath::RoundToInt(Indices.Z);

	const bool bNegX = X < 0;
	const bool bNegY = Y < 0;
	const bool bNegZ = Z < 0;
	
#define INDEX(NegX, NegY, NegZ, X, Y, Z) (NegX << 0) + (NegY << 1) + (NegZ << 2) + (X << 3) + (Y << 5) + (Z << 7)
	switch (INDEX(bNegX, bNegY, bNegZ, FMath::Abs(X), FMath::Abs(Y), FMath::Abs(Z)))
	{
	default: ensure(false);
	case INDEX(0, 0, 0, 1, 2, 3): return EVoxelBlockRotation::PosX_PosY_PosZ;
	case INDEX(0, 1, 1, 1, 2, 3): return EVoxelBlockRotation::PosX_NegY_NegZ;
	case INDEX(0, 0, 1, 1, 3, 2): return EVoxelBlockRotation::PosX_PosZ_NegY;
	case INDEX(0, 1, 0, 1, 3, 2): return EVoxelBlockRotation::PosX_NegZ_PosY;
		
	case INDEX(1, 0, 1, 1, 2, 3): return EVoxelBlockRotation::NegX_PosY_NegZ;
	case INDEX(1, 1, 0, 1, 2, 3): return EVoxelBlockRotation::NegX_NegY_PosZ;
	case INDEX(1, 0, 0, 1, 3, 2): return EVoxelBlockRotation::NegX_PosZ_PosY;
	case INDEX(1, 1, 1, 1, 3, 2): return EVoxelBlockRotation::NegX_NegZ_NegY;

	case INDEX(0, 0, 1, 2, 1, 3): return EVoxelBlockRotation::PosY_PosX_NegZ;
	case INDEX(0, 1, 0, 2, 1, 3): return EVoxelBlockRotation::PosY_NegX_PosZ;
	case INDEX(0, 0, 0, 2, 3, 1): return EVoxelBlockRotation::PosY_PosZ_PosX;
	case INDEX(0, 1, 1, 2, 3, 1): return EVoxelBlockRotation::PosY_NegZ_NegX;

	case INDEX(1, 0, 0, 2, 1, 3): return EVoxelBlockRotation::NegY_PosX_PosZ;
	case INDEX(1, 1, 1, 2, 1, 3): return EVoxelBlockRotation::NegY_NegX_NegZ;
	case INDEX(1, 0, 1, 2, 3, 1): return EVoxelBlockRotation::NegY_PosZ_NegX;
	case INDEX(1, 1, 0, 2, 3, 1): return EVoxelBlockRotation::NegY_NegZ_PosX;

	case INDEX(0, 0, 0, 3, 1, 2): return EVoxelBlockRotation::PosZ_PosX_PosY;
	case INDEX(0, 1, 1, 3, 1, 2): return EVoxelBlockRotation::PosZ_NegX_NegY;
	case INDEX(0, 0, 1, 3, 2, 1): return EVoxelBlockRotation::PosZ_PosY_NegX;
	case INDEX(0, 1, 0, 3, 2, 1): return EVoxelBlockRotation::PosZ_NegY_PosX;

	case INDEX(1, 0, 1, 3, 1, 2): return EVoxelBlockRotation::NegZ_PosX_NegY;
	case INDEX(1, 1, 0, 3, 1, 2): return EVoxelBlockRotation::NegZ_NegX_PosY;
	case INDEX(1, 0, 0, 3, 2, 1): return EVoxelBlockRotation::NegZ_PosY_PosX;
	case INDEX(1, 1, 1, 3, 2, 1): return EVoxelBlockRotation::NegZ_NegY_NegX;
	}
#undef INDEX
}

FVoxelIntBox FVoxelBlockRotation::RotateBounds(const FVoxelIntBox& Bounds, EVoxelBlockRotation Rotation)
{
	FIntVector Min = Bounds.Min;
	// Make sure that Max is _inside_ the bounds and not on the border, as they might end up as min now
	FIntVector Max = Bounds.Max - 1;

	Min = Rotate(Min, Rotation);
	Max = Rotate(Max, Rotation);

	const FIntVector NewMin = FVoxelUtilities::ComponentMin(Min, Max);
	const FIntVector NewMax = FVoxelUtilities::ComponentMax(Min, Max);

	return FVoxelIntBox(NewMin, NewMax + 1);
}