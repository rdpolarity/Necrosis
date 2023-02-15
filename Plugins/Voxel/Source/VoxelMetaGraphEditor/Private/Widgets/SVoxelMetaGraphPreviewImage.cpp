// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "SVoxelMetaGraphPreviewImage.h"

VOXEL_INITIALIZE_STYLE(MetaGraphPreviewImage)
{
	Set("Graph.Preview.PositionMarker", new IMAGE_BRUSH("Graphs/Preview_PositionMarker", CoreStyleConstants::Icon16x16));
}

BEGIN_VOXEL_NAMESPACE(MetaGraph)

void SPreviewImage::Construct(const FArguments& InArgs)
{
	PositionMarkerHalfSize = InArgs._PositionMarkerSize / 2.f;

	ChildSlot
	[
		SAssignNew(Box, SBox)
		.MinDesiredWidth(InArgs._Width)
		.MinDesiredHeight(InArgs._Height)
	];
}

void SPreviewImage::SetContent(const TSharedRef<SWidget>& Content)
{
	Box->SetContent(Content);
	ContentWidget = Content;
	bUpdateLockedPosition = true;
}

void SPreviewImage::SetLockedPosition(const FVector& LockedPosition)
{
	bHasLockedPosition = true;
	Position = LockedPosition;
}

bool SPreviewImage::UpdateLockedPosition(const FMatrix& Transform, const FVector2D& Offset)
{
	FVector2D PixelPosition = FVector2D(Transform.TransformPosition(Position));
	PixelPosition.Y = Offset.Y - PixelPosition.Y;
	LocalPosition = PixelPosition;

	if (LocalPosition.X >= PositionMarkerHalfSize &&
		LocalPosition.X <= Offset.X - PositionMarkerHalfSize &&
		LocalPosition.Y >= PositionMarkerHalfSize &&
        LocalPosition.Y <= Offset.Y - PositionMarkerHalfSize)
	{
		return true;
	}

	bHasLockedPosition = false;
	return false;
}

void SPreviewImage::ClearLockedPosition()
{
	bHasLockedPosition = false;
}

int32 SPreviewImage::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (!ContentWidget ||
		!bHasLockedPosition)
	{
		return LayerId;
	}
	LayerId++;

	const float InverseScale = Inverse(AllottedGeometry.Scale);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(TransformVector(InverseScale, FVector2D(PositionMarkerHalfSize * 2.f)), FSlateLayoutTransform(LocalPosition - PositionMarkerHalfSize * InverseScale)),
		FVoxelEditorStyle::GetBrush("Graph.Preview.PositionMarker"));

	return LayerId;
}

END_VOXEL_NAMESPACE(MetaGraph)