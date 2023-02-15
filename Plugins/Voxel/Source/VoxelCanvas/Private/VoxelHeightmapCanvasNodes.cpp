// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelHeightmapCanvasNodes.h"
#include "Nodes/VoxelPositionNodes.h"

FVoxelVectorBuffer FVoxelHeightmapCanvas::GeneratePositionBuffer() const
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelArray<float> PositionX = FVoxelBuffer::Allocate<float>(Size.X * Size.Y);
	TVoxelArray<float> PositionY = FVoxelBuffer::Allocate<float>(Size.X * Size.Y);

	int32 Index = 0;
	for (int32 Y = 0; Y < Size.Y; Y++)
	{
		for (int32 X = 0; X < Size.X; X++)
		{
			checkVoxelSlow(Index == FVoxelUtilities::Get2DIndex(Size, X, Y));
			PositionX[Index] = X;
			PositionY[Index] = Y;
			Index++;
		}
	}

	FVoxelVectorBuffer Positions;
	Positions.X = FVoxelFloatBuffer::MakeCpu(PositionX);
	Positions.Y = FVoxelFloatBuffer::MakeCpu(PositionY);
	Positions.Z = FVoxelFloatBuffer::Constant(0.f);
	return Positions;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_GetBaseHeightmapCanvas, Canvas)
{
	ensure(false);
	return {};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_DisplaceHeightmapCanvas, OutCanvas)
{
	const TValue<FVoxelHeightmapCanvas> PreviousCanvas = Get(CanvasPin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, PreviousCanvas)
	{
		const TSharedRef<FVoxelHeightmapCanvas> Canvas = MakeShared<FVoxelHeightmapCanvas>();
		Canvas->Initialize(PreviousCanvas->Size);

		FVoxelQuery OffsetQuery = Query;
		OffsetQuery.Add<FVoxelSparsePositionQueryData>().Initialize(Canvas->GeneratePositionBuffer());

		const TValue<TBufferView<FVector2D>> Offsets = GetNode().GetNodeRuntime().GetBufferView(OffsetPin, OffsetQuery);

		return VOXEL_ON_COMPLETE(AsyncThread, PreviousCanvas, Canvas, Offsets)
		{
			const FIntPoint Size = Canvas->Size;
			for (int32 X = 0; X < Size.X; X++)
			{
				for (int32 Y = 0; Y < Size.Y; Y++)
				{
					const int32 Index = FVoxelUtilities::Get2DIndex<int32>(Size, X, Y);
					const FVector2f SamplePosition = FVector2f(X, Y) + Offsets[Index];

					const int32 MinX = FMath::Clamp(FMath::FloorToInt(SamplePosition.X), 0, Size.X - 1);
					const int32 MaxX = FMath::Clamp(FMath::CeilToInt(SamplePosition.X), 0, Size.X - 1);
					const int32 MinY = FMath::Clamp(FMath::FloorToInt(SamplePosition.Y), 0, Size.Y - 1);
					const int32 MaxY = FMath::Clamp(FMath::CeilToInt(SamplePosition.Y), 0, Size.Y - 1);

					Canvas->Heightmap[Index] = FVoxelUtilities::BilinearInterpolation(
						PreviousCanvas->Heightmap[FVoxelUtilities::Get2DIndex<int32>(Size, MinX, MinY)],
						PreviousCanvas->Heightmap[FVoxelUtilities::Get2DIndex<int32>(Size, MaxX, MinY)],
						PreviousCanvas->Heightmap[FVoxelUtilities::Get2DIndex<int32>(Size, MinX, MaxY)],
						PreviousCanvas->Heightmap[FVoxelUtilities::Get2DIndex<int32>(Size, MaxX, MaxY)],
						FMath::Clamp(SamplePosition.X - MinX, 0.f, 1.f),
						FMath::Clamp(SamplePosition.Y - MinY, 0.f, 1.f));
				}
			}
			return Canvas;
		};
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_BlurHeightmapCanvas, OutCanvas)
{
	const TValue<FVoxelHeightmapCanvas> PreviousCanvas = Get(CanvasPin, Query);
	const TValue<float> Strength = Get(StrengthPin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, PreviousCanvas, Strength)
	{
		const TSharedRef<FVoxelHeightmapCanvas> Canvas = MakeShared<FVoxelHeightmapCanvas>();
		*Canvas = *PreviousCanvas;

		float CenterStrength = 1.f;
		float NeighborStrength = Strength * 8;
		{
			const float Sum = CenterStrength + NeighborStrength;
			CenterStrength /= Sum;
			NeighborStrength /= Sum;
		}

		const FIntPoint Size = Canvas->Size;
		for (int32 X = 1; X < Size.X - 1; X++)
		{
			for (int32 Y = 1; Y < Size.Y - 1; Y++)
			{
				const int32 Index = FVoxelUtilities::Get2DIndex<int32>(Size, X, Y);

				float Sum = 0.f;
				for (int32 I = -1; I <= 1; I++)
				{
					for (int32 J = -1; J <= 1; J++)
					{
						if (I == 0 && J == 0)
						{
							continue;
						}

						const int32 OtherIndex = FVoxelUtilities::Get2DIndex<int32>(Size, X + I, Y + J);

						Sum += PreviousCanvas->Heightmap[OtherIndex];
					}
				}

				Canvas->Heightmap[Index] = PreviousCanvas->Heightmap[Index] * CenterStrength + Sum / 8.f * NeighborStrength;
			}
		}
		return Canvas;
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_UpscaleHeightmapCanvas2x, OutCanvas)
{
	const TValue<FVoxelHeightmapCanvas> PreviousCanvas = Get(CanvasPin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, PreviousCanvas)
	{
		const TSharedRef<FVoxelHeightmapCanvas> Canvas = MakeShared<FVoxelHeightmapCanvas>();
		Canvas->Initialize(2 * PreviousCanvas->Size);
		
		const FIntPoint Size = Canvas->Size;
		for (int32 X = 0; X < Size.X; X++)
		{
			for (int32 Y = 0; Y < Size.Y; Y++)
			{
				const int32 Index = FVoxelUtilities::Get2DIndex<int32>(Size, X, Y);
				const int32 PreviousIndex = FVoxelUtilities::Get2DIndex<int32>(PreviousCanvas->Size, X / 2, Y / 2);

				Canvas->Heightmap[Index] = PreviousCanvas->Heightmap[PreviousIndex];
			}
		}

		return Canvas;
	};
}