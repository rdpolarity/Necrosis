// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

struct VOXELMETAGRAPH_API FRuntimeUtilities
{
public:
	static void WritePositions(
		TVoxelArrayView<float> OutPositionX,
		TVoxelArrayView<float> OutPositionY,
		TVoxelArrayView<float> OutPositionZ,
		const FVector3f& Start,
		float Step,
		const FIntVector& Size);
	
	static void WritePositions2D(
		TVoxelArrayView<float> OutPositionX,
		TVoxelArrayView<float> OutPositionY,
		const FVector2f& Start,
		float Step,
		const FIntPoint& Size);

	static void WritePositions_Unpacked(
		TVoxelArrayView<float> OutPositionX,
		TVoxelArrayView<float> OutPositionY,
		TVoxelArrayView<float> OutPositionZ,
		const FVector3f& Start,
		float Step,
		const FIntVector& Size);
	
	static void WriteIntPositions_Unpacked(
		TVoxelArrayView<int32> OutPositionX,
		TVoxelArrayView<int32> OutPositionY,
		TVoxelArrayView<int32> OutPositionZ,
		const FIntVector& Start,
		int32 Step,
		const FIntVector& Size);

	static void ReplicatePacked(
		TConstVoxelArrayView<float> Data,
		TVoxelArrayView<float> OutData);

	template<typename Type, typename OutDataType>
	static void UnpackData(
		TConstVoxelArrayView<Type> InData,
		OutDataType& OutData,
		const FIntVector& Size)
	{
		VOXEL_FUNCTION_COUNTER();
		check(InData.Num() == OutData.Num());
		check(InData.Num() == Size.X * Size.Y * Size.Z);

		check(Size % 2 == 0);
		const FIntVector BlockSize = Size / 2;

		int32 ReadIndex = 0;
		for (int32 Z = 0; Z < BlockSize.Z; Z++)
		{
			for (int32 Y = 0; Y < BlockSize.Y; Y++)
			{
				int32 BaseWriteIndex =
					Size.X * 2 * Y +
					Size.X * Size.Y * 2 * Z;

				for (int32 X = 0; X < BlockSize.X; X++)
				{
					checkVoxelSlow(ReadIndex == 8 * FVoxelUtilities::Get3DIndex<int32>(Size / 2, X, Y, Z));
					checkVoxelSlow(BaseWriteIndex == FVoxelUtilities::Get3DIndex<int32>(Size, 2 * X, 2 * Y, 2 * Z));

#define LOOP(Block) \
					{ \
						const int32 WriteIndex = BaseWriteIndex + bool(Block & 0x1) + bool(Block & 0x2) * Size.X + bool(Block & 0x4) * Size.X * Size.Y; \
						checkVoxelSlow(WriteIndex == FVoxelUtilities::Get3DIndex<int32>(Size, \
							2 * X + bool(Block & 0x1), \
							2 * Y + bool(Block & 0x2), \
							2 * Z + bool(Block & 0x4))); \
						\
						OutData[WriteIndex] = InData[ReadIndex + Block]; \
					}

					LOOP(0);
					LOOP(1);
					LOOP(2);
					LOOP(3);
					LOOP(4);
					LOOP(5);
					LOOP(6);
					LOOP(7);

#undef LOOP

					ReadIndex += 8;
					BaseWriteIndex += 2;
				}
			}
		}
	}
};

END_VOXEL_NAMESPACE(MetaGraph)