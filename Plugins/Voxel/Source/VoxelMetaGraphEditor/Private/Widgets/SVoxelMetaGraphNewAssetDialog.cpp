// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "SVoxelMetaGraphNewAssetDialog.h"
#include "VoxelMetaGraph.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

void SNewAssetDialog::Construct(const FArguments& InArgs)
{
	SWindow::Construct(
		SWindow::FArguments()
		.Title(VOXEL_LOCTEXT("Pick a starting point for your Meta Graph"))
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(FVector2D(450, 600))
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
			.Padding(10.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.FillHeight(1.f)
				[
					SNew(SBox)
					[
						SAssignNew(NewAssetPicker, SAssetPickerList, UVoxelMetaGraph::StaticClass())
						.OnTemplateAssetActivated(this, &SNewAssetDialog::ConfirmSelection)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				.Padding(0.0f, 10.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.Padding(FCoreStyle::Get().GetMargin("StandardDialog.SlotPadding"))
					[
						SNew(SButton)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
						.TextStyle(&FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("DialogButtonText"))
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.ContentPadding(FCoreStyle::Get().GetMargin("StandardDialog.ContentPadding"))
						.IsEnabled(this, &SNewAssetDialog::IsCreateButtonEnabled)
						.OnClicked(this, &SNewAssetDialog::OnConfirmSelection)
						.ToolTipText(VOXEL_LOCTEXT("Create graph from selected template"))
						.Text(VOXEL_LOCTEXT("Create"))
					]
					+ SHorizontalBox::Slot()
					.Padding(FCoreStyle::Get().GetMargin("StandardDialog.SlotPadding"))
					[
						SNew(SButton)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
						.TextStyle(&FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("DialogButtonText"))
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.ContentPadding(FCoreStyle::Get().GetMargin("StandardDialog.ContentPadding"))
						.OnClicked(this, &SNewAssetDialog::OnCancelButtonClicked)
						.Text(VOXEL_LOCTEXT("Cancel"))
					]
				]
			]
		]);
	
	FSlateApplication::Get().SetKeyboardFocus(NewAssetPicker);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TOptional<FAssetData> SNewAssetDialog::GetSelectedAsset()
{
	if (SelectedAssets.Num() > 0)
	{
		return SelectedAssets[0];
	}
	return TOptional<FAssetData>();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SNewAssetDialog::ConfirmSelection(const FAssetData& AssetData)
{
	SelectedAssets.Add(AssetData);
	
	bUserConfirmedSelection = true;
	RequestDestroyWindow();
}

FReply SNewAssetDialog::OnConfirmSelection()
{
	SelectedAssets = NewAssetPicker->GetSelectedAssets();
	ensureMsgf(!SelectedAssets.IsEmpty(), TEXT("No assets selected when dialog was confirmed."));
	
	bUserConfirmedSelection = true;
	RequestDestroyWindow();

	return FReply::Handled();
}

bool SNewAssetDialog::IsCreateButtonEnabled() const
{
	return !NewAssetPicker->GetSelectedAssets().IsEmpty();
}

FReply SNewAssetDialog::OnCancelButtonClicked()
{
	bUserConfirmedSelection = false;
	SelectedAssets.Empty();

	RequestDestroyWindow();

	return FReply::Handled();
}

END_VOXEL_NAMESPACE(MetaGraph)