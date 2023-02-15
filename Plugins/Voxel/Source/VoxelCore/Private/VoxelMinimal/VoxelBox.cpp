// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMinimal.h"

const FVoxelBox FVoxelBox::Infinite = FVoxelBox(FVector3d(-1e50), FVector3d(1e50));

FVoxelBox FVoxelBox::FromPositions(TConstVoxelArrayView<FIntVector> Positions)
{
    VOXEL_FUNCTION_COUNTER();

    if (Positions.Num() == 0)
    {
        return {};
    }

    FIntVector Min = Positions[0];
    FIntVector Max = Positions[0];
    
    for (int32 Index = 1; Index < Positions.Num(); Index++) 
    {
        Min = FVoxelUtilities::ComponentMin(Min, Positions[Index]);
        Max = FVoxelUtilities::ComponentMax(Max, Positions[Index]);
    } 

    return FVoxelBox(Min, Max);
}

FVoxelBox FVoxelBox::FromPositions(TConstVoxelArrayView<FVector3f> Positions)
{
    VOXEL_FUNCTION_COUNTER();

    if (Positions.Num() == 0)
    {
        return {};
    }

    FVector3f Min = Positions[0];
    FVector3f Max = Positions[0];
    
    for (int32 Index = 1; Index < Positions.Num(); Index++) 
    {
        Min = FVoxelUtilities::ComponentMin(Min, Positions[Index]);
        Max = FVoxelUtilities::ComponentMax(Max, Positions[Index]);
    } 

    return FVoxelBox(Min, Max);
}

FVoxelBox FVoxelBox::FromPositions(TConstVoxelArrayView<FVector3d> Positions)
{
    VOXEL_FUNCTION_COUNTER();

    if (Positions.Num() == 0)
    {
        return {};
    }

    FVector3d Min = Positions[0];
    FVector3d Max = Positions[0];
    
    for (int32 Index = 1; Index < Positions.Num(); Index++) 
    {
        Min = FVoxelUtilities::ComponentMin(Min, Positions[Index]);
        Max = FVoxelUtilities::ComponentMax(Max, Positions[Index]);
    } 

    return FVoxelBox(Min, Max);
}

FVoxelBox FVoxelBox::FromPositions(
    TConstVoxelArrayView<float> PositionX, 
    TConstVoxelArrayView<float> PositionY, 
    TConstVoxelArrayView<float> PositionZ)
{
    VOXEL_FUNCTION_COUNTER();

    const int32 Num = PositionX.Num();
    check(Num == PositionX.Num());
    check(Num == PositionY.Num());
    check(Num == PositionZ.Num());

    if (Num == 0)
    {
        return {};
    }

	FVector3f Min = { PositionX[0], PositionY[0], PositionZ[0] };
	FVector3f Max = { PositionX[0], PositionY[0], PositionZ[0] };

	for (int32 Index = 1; Index < Num; Index++)
	{
		const FVector3f Position{ PositionX[Index], PositionY[Index], PositionZ[Index] };
		Min = FVoxelUtilities::ComponentMin(Min, Position);
		Max = FVoxelUtilities::ComponentMax(Max, Position);
	}

	return FVoxelBox(Min, Max);
}