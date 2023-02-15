// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Widgets/SCompoundWidget.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

class VOXELMETAGRAPHEDITOR_API SPreviewImage : public SCompoundWidget
{
public:
	VOXEL_SLATE_ARGS()
	{
		FArguments() : _PositionMarkerSize(16.f)
		{
		}

		SLATE_ATTRIBUTE(FOptionalSize, Width);
		SLATE_ATTRIBUTE(FOptionalSize, Height);
		SLATE_ARGUMENT(float, PositionMarkerSize);
	};

	void Construct(const FArguments& InArgs);

	void SetContent(const TSharedRef<SWidget>& Content);
	void SetLockedPosition(const FVector& LockedPosition);
	bool UpdateLockedPosition(const FMatrix& Transform, const FVector2D& Offset);
	void ClearLockedPosition();

	//~ Begin SCompoundWidget Interface
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	//~ End SCompoundWidget Interface

private:
	TSharedPtr<SBox> Box;
	TSharedPtr<SWidget> ContentWidget;

	float PositionMarkerHalfSize = 0.f;

	bool bUpdateLockedPosition = false;

	bool bHasLockedPosition = false;
	FVector Position;
	FVector2D LocalPosition;
};

END_VOXEL_NAMESPACE(MetaGraph)