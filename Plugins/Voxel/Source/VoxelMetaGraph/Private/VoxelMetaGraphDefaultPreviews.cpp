// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphDefaultPreviews.h"
#include "Widgets/Images/SImage.h"

void FVoxelMetaGraphPreviewFactory_FVoxelFloatBuffer::Create(const FParameters& Parameters) const
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelFutureValue FutureValue = Parameters.ExecuteQuery(MakeDefaultQuery(Parameters));

	FutureValue.OnComplete(EVoxelTaskThread::AsyncThread, [=]
	{
		const FVoxelFloatBufferView Buffer = FutureValue.Get_CheckCompleted<FVoxelFloatBufferView>();

		const TSharedRef<FVoxelMetaGraphPreview_Texture> Preview = MakeShared<FVoxelMetaGraphPreview_Texture>();
		Preview->GenerateColors = [=](const bool bNormalize, const int32 PreviewSize, const TVoxelArrayView<FLinearColor> Colors)
		{
			if (!FVoxelBufferAccessor(Buffer, Colors).IsValid())
			{
				FVoxelUtilities::SetAll(Colors, FLinearColor::Red);
				return;
			}

			const FFloatInterval MinMax = FVoxelUtilities::GetMinMaxSafe(Buffer.GetRawView());

			for (int32 Y = 0; Y < PreviewSize; Y++)
			{
				for (int32 X = 0; X < PreviewSize; X++)
				{
					const int32 Index = FVoxelUtilities::Get2DIndex<int32>(PreviewSize, X, Y);
					const float Scalar = Buffer[Index];

					if (!FMath::IsFinite(Scalar))
					{
						Colors[Index] = FColor::Magenta;
						continue;
					}

					const float ScaledValue = bNormalize ? (Scalar - MinMax.Min) / (MinMax.Max - MinMax.Min) : Scalar;
					Colors[Index] = FLinearColor(ScaledValue, ScaledValue, ScaledValue);
				}
			}

			Preview->MinValue = LexToSanitizedString(MinMax.Min);
			Preview->MaxValue = LexToSanitizedString(MinMax.Max);
		};
		Preview->GetTooltip = [=](int32 Index) -> FString
		{
			if (!Buffer.IsValidIndex(Index))
			{
				return "Invalid";
			}

			return LexToSanitizedString(Buffer[Index]);
		};

		RenderTexture(Parameters, Preview);
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelMetaGraphPreviewFactory_DensityBuffer::CanCreate(const FVoxelPinType& Type) const
{
	return
		Type.Is<FVoxelFloatBuffer>() &&
		Type.GetTag() == "Density";
}

void FVoxelMetaGraphPreviewFactory_DensityBuffer::Create(const FParameters& Parameters) const
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelFutureValue FutureValue = Parameters.ExecuteQuery(MakeDefaultQuery(Parameters));

	FutureValue.OnComplete(EVoxelTaskThread::AsyncThread, [=]
	{
		const FVoxelFloatBufferView Buffer = FutureValue.Get_CheckCompleted<FVoxelFloatBufferView>();

		const TSharedRef<FVoxelMetaGraphPreview_Texture> Preview = MakeShared<FVoxelMetaGraphPreview_Texture>();
		Preview->GenerateColors = [=](const bool bNormalize, const int32 PreviewSize, const TVoxelArrayView<FLinearColor> Colors)
		{
			if (!FVoxelBufferAccessor(Buffer, Colors).IsValid())
			{
				FVoxelUtilities::SetAll(Colors, FLinearColor::Red);
				return;
			}

			const FFloatInterval MinMax = FVoxelUtilities::GetMinMaxSafe(Buffer.GetRawView());
			const float Divisor = FMath::Max(FMath::Abs(MinMax.Min), FMath::Abs(MinMax.Max));

			for (int32 Y = 0; Y < PreviewSize; Y++)
			{
				for (int32 X = 0; X < PreviewSize; X++)
				{
					const int32 Index = FVoxelUtilities::Get2DIndex<int32>(PreviewSize, X, Y);
					const float Scalar = Buffer[Index];

					if (!FMath::IsFinite(Scalar))
					{
						Colors[Index] = FColor::Magenta;
						continue;
					}

					const float ScaledValue = Scalar / Divisor;
					Colors[Index] = FVoxelUtilities::GetDistanceFieldColor(ScaledValue);
				}
			}

			Preview->MinValue = LexToSanitizedString(MinMax.Min);
			Preview->MaxValue = LexToSanitizedString(MinMax.Max);
		};
		Preview->GetTooltip = [=](int32 Index) -> FString
		{
			if (!Buffer.IsValidIndex(Index))
			{
				return "Invalid";
			}

			return LexToSanitizedString(Buffer[Index]);
		};

		RenderTexture(Parameters, Preview);
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMetaGraphPreviewFactory_FVoxelVector2DBuffer::Create(const FParameters& Parameters) const
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelFutureValue FutureValue = Parameters.ExecuteQuery(MakeDefaultQuery(Parameters));

	FutureValue.OnComplete(EVoxelTaskThread::AsyncThread, [=]
	{
		const FVoxelVector2DBufferView Buffer = FutureValue.Get_CheckCompleted<FVoxelVector2DBufferView>();

		const TSharedRef<FVoxelMetaGraphPreview_Texture> Preview = MakeShared<FVoxelMetaGraphPreview_Texture>();
		Preview->GenerateColors = [=](const bool bNormalize, const int32 PreviewSize, const TVoxelArrayView<FLinearColor> Colors)
		{
			if (!FVoxelBufferAccessor(Buffer, Colors).IsValid())
			{
				FVoxelUtilities::SetAll(Colors, FLinearColor::Red);
				return;
			}

			const FFloatInterval MinMaxX = FVoxelUtilities::GetMinMaxSafe(Buffer.X.GetRawView());
			const FFloatInterval MinMaxY = FVoxelUtilities::GetMinMaxSafe(Buffer.Y.GetRawView());

			for (int32 Y = 0; Y < PreviewSize; Y++)
			{
				for (int32 X = 0; X < PreviewSize; X++)
				{
					const int32 Index = FVoxelUtilities::Get2DIndex<int32>(PreviewSize, X, Y);
					const FVector2f Vector = Buffer[Index];

					if (!FMath::IsFinite(Vector.X) ||
						!FMath::IsFinite(Vector.Y))
					{
						Colors[Index] = FColor::Magenta;
						continue;
					}

					const float ScaledValueX = bNormalize ? (Vector.X - MinMaxX.Min) / (MinMaxX.Max - MinMaxX.Min) : Vector.X;
					const float ScaledValueY = bNormalize ? (Vector.Y - MinMaxY.Min) / (MinMaxY.Max - MinMaxY.Min) : Vector.Y;

					Colors[Index] = FLinearColor(ScaledValueX, ScaledValueY, 0.f);
				}
			}

			Preview->MinValue = FString::Printf(TEXT("X=%3.3f Y=%3.3f"), MinMaxX.Min, MinMaxY.Min);
			Preview->MaxValue = FString::Printf(TEXT("X=%3.3f Y=%3.3f"), MinMaxX.Max, MinMaxY.Max);
		};
		Preview->GetTooltip = [=](int32 Index) -> FString
		{
			if (!Buffer.IsValidIndex(Index))
			{
				return "Invalid";
			}

			return Buffer[Index].ToString();
		};

		RenderTexture(Parameters, Preview);
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMetaGraphPreviewFactory_FVoxelVectorBuffer::Create(const FParameters& Parameters) const
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelFutureValue FutureValue = Parameters.ExecuteQuery(MakeDefaultQuery(Parameters));

	FutureValue.OnComplete(EVoxelTaskThread::AsyncThread, [=]
	{
		const FVoxelVectorBufferView Buffer = FutureValue.Get_CheckCompleted<FVoxelVectorBufferView>();

		const TSharedRef<FVoxelMetaGraphPreview_Texture> Preview = MakeShared<FVoxelMetaGraphPreview_Texture>();
		Preview->GenerateColors = [=](const bool bNormalize, const int32 PreviewSize, const TVoxelArrayView<FLinearColor> Colors)
		{
			if (!FVoxelBufferAccessor(Buffer, Colors).IsValid())
			{
				FVoxelUtilities::SetAll(Colors, FLinearColor::Red);
				return;
			}

			const FFloatInterval MinMaxX = FVoxelUtilities::GetMinMaxSafe(Buffer.X.GetRawView());
			const FFloatInterval MinMaxY = FVoxelUtilities::GetMinMaxSafe(Buffer.Y.GetRawView());
			const FFloatInterval MinMaxZ = FVoxelUtilities::GetMinMaxSafe(Buffer.Z.GetRawView());

			for (int32 Y = 0; Y < PreviewSize; Y++)
			{
				for (int32 X = 0; X < PreviewSize; X++)
				{
						const int32 Index = FVoxelUtilities::Get2DIndex<int32>(PreviewSize, X, Y);
						const FVector3f Vector = Buffer[Index];

						if (!FMath::IsFinite(Vector.X) ||
							!FMath::IsFinite(Vector.Y) ||
							!FMath::IsFinite(Vector.Z))
						{
							Colors[Index] = FColor::Magenta;
							continue;
						}

						const float ScaledValueX = bNormalize ? (Vector.X - MinMaxX.Min) / (MinMaxX.Max - MinMaxX.Min) : Vector.X;
						const float ScaledValueY = bNormalize ? (Vector.Y - MinMaxY.Min) / (MinMaxY.Max - MinMaxY.Min) : Vector.Y;
						const float ScaledValueZ = bNormalize ? (Vector.Z - MinMaxZ.Min) / (MinMaxZ.Max - MinMaxZ.Min) : Vector.Z;

						Colors[Index] = FLinearColor(ScaledValueX, ScaledValueY, ScaledValueZ);
				}
			}

			Preview->MinValue = FString::Printf(TEXT("X=%3.3f Y=%3.3f Z=%3.3f"), MinMaxX.Min, MinMaxY.Min, MinMaxZ.Min);
			Preview->MaxValue = FString::Printf(TEXT("X=%3.3f Y=%3.3f Z=%3.3f"), MinMaxX.Max, MinMaxY.Max, MinMaxZ.Max);
		};
		Preview->GetTooltip = [=](int32 Index) -> FString
		{
			if (!Buffer.IsValidIndex(Index))
			{
				return "Invalid";
			}

			return Buffer[Index].ToString();
		};

		RenderTexture(Parameters, Preview);
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMetaGraphPreviewFactory_FVoxelLinearColorBuffer::Create(const FParameters& Parameters) const
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelFutureValue FutureValue = Parameters.ExecuteQuery(MakeDefaultQuery(Parameters));

	FutureValue.OnComplete(EVoxelTaskThread::AsyncThread, [=]
	{
		const FVoxelLinearColorBufferView Buffer = FutureValue.Get_CheckCompleted<FVoxelLinearColorBufferView>();

		const TSharedRef<FVoxelMetaGraphPreview_Texture> Preview = MakeShared<FVoxelMetaGraphPreview_Texture>();
		Preview->GenerateColors = [=](const bool bNormalize, const int32 PreviewSize, const TVoxelArrayView<FLinearColor> Colors)
		{
			if (!FVoxelBufferAccessor(Buffer, Colors).IsValid())
			{
				FVoxelUtilities::SetAll(Colors, FLinearColor::Red);
				return;
			}

			const FFloatInterval MinMaxR = FVoxelUtilities::GetMinMaxSafe(Buffer.R.GetRawView());
			const FFloatInterval MinMaxG = FVoxelUtilities::GetMinMaxSafe(Buffer.G.GetRawView());
			const FFloatInterval MinMaxB = FVoxelUtilities::GetMinMaxSafe(Buffer.B.GetRawView());
			const FFloatInterval MinMaxA = FVoxelUtilities::GetMinMaxSafe(Buffer.A.GetRawView());

			for (int32 Y = 0; Y < PreviewSize; Y++)
			{
				for (int32 X = 0; X < PreviewSize; X++)
				{
						const int32 Index = FVoxelUtilities::Get2DIndex<int32>(PreviewSize, X, Y);
						const FLinearColor Color = Buffer[Index];

						if (!FMath::IsFinite(Color.R) ||
							!FMath::IsFinite(Color.G) ||
							!FMath::IsFinite(Color.B) ||
							!FMath::IsFinite(Color.A))
						{
							Colors[Index] = FColor::Magenta;
							continue;
						}

						const float ScaledValueR = bNormalize ? (Color.R - MinMaxR.Min) / (MinMaxR.Max - MinMaxR.Min) : Color.R;
						const float ScaledValueG = bNormalize ? (Color.G - MinMaxG.Min) / (MinMaxG.Max - MinMaxG.Min) : Color.G;
						const float ScaledValueB = bNormalize ? (Color.B - MinMaxB.Min) / (MinMaxB.Max - MinMaxB.Min) : Color.B;
						const float ScaledValueA = bNormalize ? (Color.A - MinMaxA.Min) / (MinMaxA.Max - MinMaxA.Min) : Color.A;

						Colors[Index] = FLinearColor(ScaledValueR, ScaledValueG, ScaledValueB, ScaledValueA);
				}
			}

			Preview->MinValue = FString::Printf(TEXT("(R=%3.3f G=%3.3f B=%3.3f A=%3.3f)"), MinMaxR.Min, MinMaxG.Min, MinMaxB.Min, MinMaxA.Min);
			Preview->MaxValue = FString::Printf(TEXT("(R=%3.3f G=%3.3f B=%3.3f A=%3.3f)"), MinMaxR.Max, MinMaxG.Max, MinMaxB.Max, MinMaxA.Max);
		};
		Preview->GetTooltip = [=](int32 Index) -> FString
		{
			if (!Buffer.IsValidIndex(Index))
			{
				return "Invalid";
			}

			return FString::Printf(TEXT("(R=%3.3f G=%3.3f B=%3.3f A=%3.3f)"), Buffer[Index].R, Buffer[Index].G, Buffer[Index].B, Buffer[Index].A);
		};

		RenderTexture(Parameters, Preview);
	});
}