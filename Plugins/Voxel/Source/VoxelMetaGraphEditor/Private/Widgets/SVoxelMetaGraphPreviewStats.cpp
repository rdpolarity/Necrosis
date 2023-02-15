// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "SVoxelMetaGraphPreviewStats.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

void SPreviewStats::Construct(const FArguments& InArgs)
{
	for (const FStatsRow::FSlotArguments& Row : InArgs._Rows)
	{
		MessagesList.Add(MakeShared<FStatsRow>(Row._Header, Row._Tooltip, Row._Value));
	}

	const TSharedRef<SScrollBar> ScrollBar = SNew(SScrollBar);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FEditorAppStyle::Get().GetBrush("Brushes.Recessed"))
		.Padding(15.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				[
					SNew(SScrollBox)
					.Orientation(Orient_Horizontal)
					+ SScrollBox::Slot()
					[
						SAssignNew(MessageListView, SListView<TSharedPtr<FStatsRow>>)
						.ListItemsSource(&MessagesList)
						.OnGenerateRow(this, &SPreviewStats::CreateRow)
						.ExternalScrollbar(ScrollBar)
						.ItemHeight(24.0f)
						.ConsumeMouseWheel(EConsumeMouseWheel::Always)
	
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(FOptionalSize(16))
					[
						ScrollBar
					]
				]
			]
		]
	];
}

TSharedRef<ITableRow> SPreviewStats::CreateRow(TSharedPtr<FStatsRow> StatsRow, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return
		SNew(STableRow<TSharedPtr<FStatsRow>>, OwnerTable)
		[
			SNew(SVerticalBox)
			.ToolTipText(StatsRow->Tooltip)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(2.f)
				[
					SNew(SBox)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.WidthOverride(16.f)
					.HeightOverride(16.f)
					[
						SNew(SImage)
						.Image(FEditorAppStyle::GetBrush("Icons.BulletPoint"))
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(StatsRow->Header.ToString() + ": "))
					.ColorAndOpacity(FSlateColor::UseForeground())
					.TextStyle(FEditorAppStyle::Get(), "MessageLog")
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				.Padding(0.f, 0.f, 5.f, 0.f)
				[
					SNew(STextBlock)
					.Text(StatsRow->Value)
					.ColorAndOpacity(FSlateColor::UseForeground())
					.TextStyle(FEditorAppStyle::Get(), "MessageLog")
				]
			]
		];
}

END_VOXEL_NAMESPACE(MetaGraph)