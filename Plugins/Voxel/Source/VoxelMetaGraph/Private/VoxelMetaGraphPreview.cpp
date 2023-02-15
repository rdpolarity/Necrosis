// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphPreview.h"
#include "Nodes/VoxelPositionNodes.h"
#include "Widgets/Images/SImage.h"
#include "Slate/DeferredCleanupSlateBrush.h"

const FVoxelMetaGraphPreviewFactory* FVoxelMetaGraphPreviewFactory::FindFactory(const FVoxelPinType& Type)
{
	static TArray<TVoxelInstancedStruct<FVoxelMetaGraphPreviewFactory>> Array;
	if (Array.Num() == 0)
	{
		for (UScriptStruct* Struct : GetDerivedStructs<FVoxelMetaGraphPreviewFactory>())
		{
			Array.Add(TVoxelInstancedStruct<FVoxelMetaGraphPreviewFactory>(Struct));
		}
	}

	const FVoxelMetaGraphPreviewFactory* FoundFactory = nullptr;
	for (const TVoxelInstancedStruct<FVoxelMetaGraphPreviewFactory>& Factory : Array)
	{
		if (!Factory->CanCreate(Type))
		{
			continue;
		}

		ensure(!FoundFactory);
		FoundFactory = &Factory.Get();
	}

	return FoundFactory;
}

bool FVoxelMetaGraphPreviewFactory::CanCreate(const FVoxelPinType& Type) const
{
	if (!Type.IsStruct() ||
		Type.HasTag())
	{
		return false;
	}

	FString TypeString;
	if (!GetStruct()->GetName().Split(
		TEXT("_"),
		nullptr,
		&TypeString,
		ESearchCase::CaseSensitive,
		ESearchDir::FromEnd))
	{
		return false;
	}

	return Type.GetStruct()->GetStructCPPName() == TypeString;
}

FVoxelQuery FVoxelMetaGraphPreviewFactory::MakeDefaultQuery(const FParameters& Parameters)
{
	FVoxelBox Bounds;
	const FVoxelVectorBuffer Positions = ComputePositions(Parameters, &Bounds);

	FVoxelQuery Query;
	Query.Add<FVoxelLODQueryData>().LOD = 0;
	Query.Add<FVoxelBoundsQueryData>().Bounds = Bounds;
	Query.Add<FVoxelGradientStepQueryData>().Step = Parameters.PixelToLocal.GetScaleVector().GetAbsMax();
	Query.Add<FVoxelSparsePositionQueryData>().Initialize(Positions);
	return Query;
}

FVoxelVectorBuffer FVoxelMetaGraphPreviewFactory::ComputePositions(const FParameters& Parameters, FVoxelBox* OutBounds)
{
	VOXEL_FUNCTION_COUNTER();

	const int32 PreviewSize = Parameters.PreviewSize;

	TVoxelArray<float> PositionsX = FVoxelFloatBuffer::Allocate(PreviewSize * PreviewSize);
	TVoxelArray<float> PositionsY = FVoxelFloatBuffer::Allocate(PreviewSize * PreviewSize);
	TVoxelArray<float> PositionsZ = FVoxelFloatBuffer::Allocate(PreviewSize * PreviewSize);

	FVoxelOptionalBox Bounds;
	for (int32 Y = 0; Y < PreviewSize; Y++)
	{
		for (int32 X = 0; X < PreviewSize; X++)
		{
			const int32 Index = FVoxelUtilities::Get2DIndex<int32>(PreviewSize, X, Y);
			const FVector Position = Parameters.PixelToLocal.TransformPosition(FVector(X, Y, 0));

			Bounds += Position;

			PositionsX[Index] = Position.X;
			PositionsY[Index] = Position.Y;
			PositionsZ[Index] = Position.Z;
		}
	}

	if (OutBounds)
	{
		*OutBounds = Bounds.GetBox();
	}

	return FVoxelVectorBuffer::MakeCpu(PositionsX, PositionsY, PositionsZ);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMetaGraphPreview_Texture::UpdateTexture(const FSimpleDelegate& OnComplete)
{
	VOXEL_FUNCTION_COUNTER();

	AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [=]
	{
		VOXEL_SCOPE_COUNTER("Generate colors");

		TVoxelArray<FLinearColor> Colors;
		FVoxelUtilities::SetNumZeroed(Colors, PreviewSize * PreviewSize);
		GenerateColors(bNormalize, PreviewSize, Colors);

		TVoxelArray<FColor> FinalColors;
		FVoxelUtilities::SetNumFast(FinalColors, PreviewSize * PreviewSize);

		for (int32 Y = 0; Y < PreviewSize; Y++)
		{
			for (int32 X = 0; X < PreviewSize; X++)
			{
				FLinearColor Color = Colors[FVoxelUtilities::Get2DIndex<int32>(PreviewSize, X, Y)];
				Color.A = 1.f;
				FinalColors[FVoxelUtilities::Get2DIndex<int32>(PreviewSize, X, PreviewSize - 1 - Y)] = Color.ToFColor(false);
			}
		}

		const TSharedRef<TArray<uint8>> Bytes = MakeSharedCopy(ReinterpretCastArray_Copy<uint8>(FinalColors));

		AsyncTask(ENamedThreads::GameThread, [=]
		{
			VOXEL_SCOPE_COUNTER("Update texture");

			if (!Texture.IsValid() ||
				Texture->GetSizeX() != PreviewSize ||
				Texture->GetSizeY() != PreviewSize)
			{
				Texture = FVoxelTextureUtilities::CreateTexture2D(
					"Preview",
					PreviewSize,
					PreviewSize,
					false,
					TF_Bilinear,
					PF_B8G8R8A8);

				Brush.Reset();
			}

			if (!Brush)
			{
				Brush = FDeferredCleanupSlateBrush::CreateBrush(Texture.Get(), FVector2D(PreviewSize, PreviewSize));
			}

			FVoxelRenderUtilities::AsyncCopyTexture(Texture, Bytes, OnComplete);
		});
	});
}

FString FVoxelMetaGraphPreview_Texture::GetValue(const FVector2D& MousePosition) const
{
	const FIntPoint Position = FVoxelUtilities::FloorToInt(MousePosition);
	return GetTooltip(
		FVoxelUtilities::Get2DIndex<int32>(
			PreviewSize,
			FMath::Clamp(Position.X, 0, PreviewSize - 1),
			FMath::Clamp(Position.Y, 0, PreviewSize - 1)));
}

FString FVoxelMetaGraphPreview_Texture::GetMinValue() const
{
	return MinValue;
}

FString FVoxelMetaGraphPreview_Texture::GetMaxValue() const
{
	return MaxValue;
}

TSharedPtr<SWidget> FVoxelMetaGraphPreview_Texture::MakeWidget()
{
	return
		SNew(SImage)
		.Image_Lambda([=]
		{
			return Brush ? Brush->GetSlateBrush() : nullptr;
		});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMetaGraphPreviewFactory_Texture::RenderTexture(
	const FParameters& Parameters,
	const TSharedRef<FVoxelMetaGraphPreview_Texture>& Preview)
{
	Preview->PreviewSize = Parameters.PreviewSize;
	Preview->PixelToLocal = Parameters.PixelToLocal;
	Preview->bNormalize = Parameters.bNormalize;

	if (const TSharedPtr<FVoxelMetaGraphPreview_Texture> PreviewTexture = Cast<FVoxelMetaGraphPreview_Texture>(Parameters.PreviousPreview))
	{
		Preview->Texture = PreviewTexture->Texture;
		Preview->Brush = PreviewTexture->Brush;
	}

	Preview->UpdateTexture(MakeLambdaDelegate([=]
	{
		Parameters.Finalize(Preview);
	}));
}