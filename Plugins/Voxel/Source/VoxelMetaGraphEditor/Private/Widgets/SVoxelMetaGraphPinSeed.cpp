// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "SVoxelMetaGraphPinSeed.h"
#include "VoxelMetaGraphSeed.h"
#include "VoxelPinValue.h"

void SVoxelMetaGraphPinSeed::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

TSharedRef<SWidget>	SVoxelMetaGraphPinSeed::GetDefaultValueWidget()
{
	return
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			SNew(SBox)
			.MinDesiredWidth(18)
			.MaxDesiredWidth(400)
			[
				SNew(SEditableTextBox)
				.Style(FEditorAppStyle::Get(), "Graph.EditableTextBox")
				.Text_Lambda([=]
				{
					const FVoxelPinValue DefaultValue = FVoxelPinValue::MakeFromPinDefaultValue(*GraphPinObj);
					if (!ensure(DefaultValue.Is<FVoxelMetaGraphSeed>()))
					{
						return FText();
					}
					return FText::FromString(DefaultValue.Get<FVoxelMetaGraphSeed>().Seed);
				})
				.SelectAllTextWhenFocused(true)
				.Visibility(this, &SGraphPin::GetDefaultValueVisibility)
				.IsReadOnly_Lambda([=]() -> bool
				{
					return GraphPinObj->bDefaultValueIsReadOnly;
				})
				.OnTextCommitted_Lambda([=](const FText& NewValue, ETextCommit::Type)
				{
					if (!ensure(!GraphPinObj->IsPendingKill()) ||
						GraphPinObj->DefaultValue.Equals(NewValue.ToString()))
					{
						return;
					}
					
					const FVoxelTransaction Transaction(GraphPinObj, "Change Seed Pin Value");

					FVoxelPinValue Value = FVoxelPinValue::Make<FVoxelMetaGraphSeed>();
					Value.Get<FVoxelMetaGraphSeed>().Seed = NewValue.ToString();
					GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, Value.ExportToString());
				})
				.ForegroundColor(FSlateColor::UseForeground())
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.f, 1.f)
		[
			SNew(SBox)
			.WidthOverride(16.f)
			.HeightOverride(16.f)
			.Visibility(this, &SGraphPin::GetDefaultValueVisibility)
			[
				SNew(SButton)
				.ButtonStyle(FVoxelEditorStyle::Get(), "Graph.Seed.Dice")
				.OnClicked_Lambda([&]
				{
					if (!ensure(!GraphPinObj->IsPendingKill()))
					{
						return FReply::Handled();
					}
					
					const FVoxelTransaction Transaction(GraphPinObj, "Randomize Seed Pin Value");

					FVoxelPinValue Value = FVoxelPinValue::Make<FVoxelMetaGraphSeed>();
					Value.Get<FVoxelMetaGraphSeed>().Randomize();
					GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, Value.ExportToString());

					return FReply::Handled();
				})
			]
		];
}