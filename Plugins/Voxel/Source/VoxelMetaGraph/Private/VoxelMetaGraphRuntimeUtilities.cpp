// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphRuntimeUtilities.h"
#include "VoxelMetaGraphRuntimeUtilitiesHelpers.ispc.generated.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

void FRuntimeUtilities::WritePositions(
	TVoxelArrayView<float> OutPositionX,
	TVoxelArrayView<float> OutPositionY,
	TVoxelArrayView<float> OutPositionZ,
	const FVector3f& Start,
	float Step,
	const FIntVector& Size)
{
	VOXEL_FUNCTION_COUNTER();

	ensure(Size % 2 == 0);
	ensure(OutPositionX.Num() == Size.X * Size.Y * Size.Z);
	ensure(OutPositionY.Num() == Size.X * Size.Y * Size.Z);
	ensure(OutPositionZ.Num() == Size.X * Size.Y * Size.Z);

	const FIntVector BlockSize = Size / 2;

	ispc::MetaGraph_WritePositions(
		OutPositionX.GetData(),
		OutPositionY.GetData(),
		OutPositionZ.GetData(),
		BlockSize.X,
		BlockSize.Y,
		BlockSize.Z,
		Start.X,
		Start.Y,
		Start.Z,
		Step);
}

void FRuntimeUtilities::WritePositions2D(
	TVoxelArrayView<float> OutPositionX,
	TVoxelArrayView<float> OutPositionY,
	const FVector2f& Start,
	float Step,
	const FIntPoint& Size)
{
	VOXEL_FUNCTION_COUNTER();

	ensure(Size % 2 == 0);
	ensure(OutPositionX.Num() == Size.X * Size.Y);
	ensure(OutPositionY.Num() == Size.X * Size.Y);

	const FIntPoint BlockSize = Size / 2;

	ispc::MetaGraph_WritePositions2D(
		OutPositionX.GetData(),
		OutPositionY.GetData(),
		BlockSize.X,
		BlockSize.Y,
		Start.X,
		Start.Y,
		Step);
}

void FRuntimeUtilities::WritePositions_Unpacked(
	TVoxelArrayView<float> OutPositionX,
	TVoxelArrayView<float> OutPositionY,
	TVoxelArrayView<float> OutPositionZ,
	const FVector3f& Start,
	float Step,
	const FIntVector& Size)
{
	VOXEL_FUNCTION_COUNTER();

	ensure(Size % 2 == 0);
	ensure(OutPositionX.Num() == Size.X * Size.Y * Size.Z);
	ensure(OutPositionY.Num() == Size.X * Size.Y * Size.Z);
	ensure(OutPositionZ.Num() == Size.X * Size.Y * Size.Z);

	int32 Index = 0;
	for (int32 Z = 0; Z < Size.Z; Z++)
	{
		for (int32 Y = 0; Y < Size.Y; Y++)
		{
			for (int32 X = 0; X < Size.X; X++)
			{
				checkVoxelSlow(Index == FVoxelUtilities::Get3DIndex<int32>(Size, X, Y, Z));

				OutPositionX[Index] = Start.X + X * Step;
				OutPositionY[Index] = Start.Y + Y * Step;
				OutPositionZ[Index] = Start.Z + Z * Step;

				Index++;
			}
		}
	}
}

void FRuntimeUtilities::WriteIntPositions_Unpacked(
	TVoxelArrayView<int32> OutPositionX, 
	TVoxelArrayView<int32> OutPositionY, 
	TVoxelArrayView<int32> OutPositionZ, 
	const FIntVector& Start, 
	int32 Step, 
	const FIntVector& Size)
{
	VOXEL_FUNCTION_COUNTER();

	ensure(Size % 2 == 0);
	ensure(OutPositionX.Num() == Size.X * Size.Y * Size.Z);
	ensure(OutPositionY.Num() == Size.X * Size.Y * Size.Z);
	ensure(OutPositionZ.Num() == Size.X * Size.Y * Size.Z);

	int32 Index = 0;
	for (int32 Z = 0; Z < Size.Z; Z++)
	{
		for (int32 Y = 0; Y < Size.Y; Y++)
		{
			for (int32 X = 0; X < Size.X; X++)
			{
				checkVoxelSlow(Index == FVoxelUtilities::Get3DIndex<int32>(Size, X, Y, Z));

				OutPositionX[Index] = Start.X + X * Step;
				OutPositionY[Index] = Start.Y + Y * Step;
				OutPositionZ[Index] = Start.Z + Z * Step;

				Index++;
			}
		}
	}
}

void FRuntimeUtilities::ReplicatePacked(
	TConstVoxelArrayView<float> Data,
	TVoxelArrayView<float> OutData)
{
	VOXEL_FUNCTION_COUNTER();

	ensure(OutData.Num() == 2 * Data.Num());
	ensure(Data.Num() % 4 == 0);

	ispc::MetaGraph_ReplicatePacked(
		Data.GetData(),
		OutData.GetData(),
		Data.Num());
}

END_VOXEL_NAMESPACE(MetaGraph)