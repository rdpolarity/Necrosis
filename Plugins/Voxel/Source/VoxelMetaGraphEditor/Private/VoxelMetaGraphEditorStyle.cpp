﻿// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "Styling/SlateStyleMacros.h"
#include "Styling/StyleColors.h"

VOXEL_INITIALIZE_STYLE(MetaGraphEditor)
{
	{
		Set("Graph.Preview.Scale", new BOX_BRUSH("Graphs/Preview_Scale", 4.f / 16.f));
		Set("Graph.Preview.Ruler", new BOX_BRUSH("Graphs/Preview_Ruler", 4.f / 16.f));
	
		Set("Graph.Seed.Dice", FButtonStyle()
			.SetNormal(IMAGE_BRUSH("Graphs/Dice", CoreStyleConstants::Icon32x32, FLinearColor(.5f,.5f,.5f)))
			.SetHovered(IMAGE_BRUSH("Graphs/Dice", CoreStyleConstants::Icon32x32, FLinearColor(.8f,.8f,.8f)))
			.SetPressed(IMAGE_BRUSH("Graphs/Dice", CoreStyleConstants::Icon32x32, FLinearColor(1.f,1.f,1.f))));
	}

	{
		const FTextBlockStyle NormalText = FEditorAppStyle::GetWidgetStyle<FTextBlockStyle>("NormalText");

		Set("Graph.NewAssetDialog.AssetPickerAssetNameText", FTextBlockStyle(NormalText)
			.SetColorAndOpacity(FLinearColor::White)
			.SetFont(DEFAULT_FONT("Regular", 9)));
		Set("Graph.NewAssetDialog.AssetPickerBoldAssetNameText", FTextBlockStyle(NormalText)
			.SetColorAndOpacity(FLinearColor::White)
			.SetFont(DEFAULT_FONT("Bold", 9)));
		Set("Graph.NewAssetDialog.AssetPickerAssetCategoryText", FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Bold", 11)));

		Set("Graph.NewAssetDialog.ActiveOptionBorderColor", FLinearColor(FColor(96, 96, 96)));
		
		Set("Graph.NewAssetDialog.FilterCheckBox", FCheckBoxStyle()
			.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
			.SetUncheckedImage(FSlateNoResource())
			.SetUncheckedHoveredImage(CORE_BOX_BRUSH("Common/RoundedSelection_16x", 4.0f/16.0f, FLinearColor(0.7f, 0.7f, 0.7f)))
			.SetUncheckedPressedImage(CORE_BOX_BRUSH("Common/RoundedSelection_16x", 4.0f/16.0f, FLinearColor(0.8f, 0.8f, 0.8f)))
			.SetCheckedImage(CORE_BOX_BRUSH("Common/RoundedSelection_16x", 4.0f/16.0f, FLinearColor(0.9f, 0.9f, 0.9f)))
			.SetCheckedHoveredImage(CORE_BOX_BRUSH("Common/RoundedSelection_16x", 4.0f/16.0f, FLinearColor(1.f, 1.f, 1.f)))
			.SetCheckedPressedImage(CORE_BOX_BRUSH("Common/RoundedSelection_16x", 4.0f/16.0f, FLinearColor(1.f, 1.f, 1.f))));
		Set("Graph.NewAssetDialog.FilterCheckBox.Border", new FSlateRoundedBoxBrush(FStyleColors::Secondary, 3.f));

		Set("Graph.NewAssetDialog.ActionFilterTextBlock", FTextBlockStyle(NormalText)
			.SetColorAndOpacity(FSlateColor::UseForeground())
			.SetHighlightColor(FLinearColor(0.02f, 0.3f, 0.0f))
			.SetShadowOffset(FVector2D(1.f, 1.f))
			.SetFont(DEFAULT_FONT("Bold", 9)));
	}

	{
		Set( "Members.Splitter", FSplitterStyle()
			.SetHandleNormalBrush(CORE_IMAGE_BRUSH("Common/SplitterHandleHighlight", CoreStyleConstants::Icon8x8, FLinearColor(.1f, .1f, .1f)))
			.SetHandleHighlightBrush(CORE_IMAGE_BRUSH("Common/SplitterHandleHighlight", CoreStyleConstants::Icon8x8, FLinearColor(.2f, .2f, .2f))));
	}
}