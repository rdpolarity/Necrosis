// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Widgets/SVoxelMetaGraphFilterBox.h"
#include "VoxelMetaGraph.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

TMap<EVoxelMetaGraphScriptSource, bool> SSourceFilterBox::SourceState
{
	{ EVoxelMetaGraphScriptSource::Voxel, true },
	{ EVoxelMetaGraphScriptSource::Game, true },
	{ EVoxelMetaGraphScriptSource::Plugins, false },
	{ EVoxelMetaGraphScriptSource::Developer, false },
};

void SSourceFilterCheckBox::Construct(const FArguments& Args, EVoxelMetaGraphScriptSource InSource)
{
	Source = InSource;
	OnSourceStateChanged = Args._OnSourceStateChanged;
	OnShiftClicked = Args._OnShiftClicked;

	const UEnum* ScriptSourceEnum = StaticEnum<EVoxelMetaGraphScriptSource>();

	SCheckBox::FArguments ParentArgs;
	
	ParentArgs
	.Style(FEditorAppStyle::Get(), "ContentBrowser.FilterButton")
	.IsChecked(Args._IsChecked)
	.OnCheckStateChanged(FOnCheckStateChanged::CreateLambda([=](ECheckBoxState NewState)
	{
		OnSourceStateChanged.ExecuteIfBound(Source, NewState == ECheckBoxState::Checked);
	}));
	
	SCheckBox::Construct(ParentArgs);

	SetToolTipText(FText::FromString("Display actions from source: " + ScriptSourceEnum->GetAuthoredNameStringByIndex(int64(Source)) + """.\nUse Shift+Click to exclusively select this filter."));

	SetContent(
		SNew(SBorder)
		.Padding(1.0f)
		.BorderImage(FVoxelEditorStyle::GetBrush("Graph.NewAssetDialog.FilterCheckBox.Border"))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("ContentBrowser.FilterImage"))
				.ColorAndOpacity(this, &SSourceFilterCheckBox::GetScriptSourceColor)
			]
			+ SHorizontalBox::Slot()
			.Padding(TAttribute<FMargin>(this, &SSourceFilterCheckBox::GetFilterNamePadding))
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(ScriptSourceEnum->GetDisplayNameTextByIndex(int64(Source)))
				.ColorAndOpacity_Raw(this, &SSourceFilterCheckBox::GetTextColor)
				.TextStyle(FVoxelEditorStyle::Get(), "Graph.NewAssetDialog.ActionFilterTextBlock")
			]
		]
	);
}

FReply SSourceFilterCheckBox::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply Reply = SCheckBox::OnMouseButtonUp(MyGeometry, MouseEvent);

	if (FSlateApplication::Get().GetModifierKeys().IsShiftDown() &&
		MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnShiftClicked.ExecuteIfBound(Source, !IsChecked());
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Reply;
}

FSlateColor SSourceFilterCheckBox::GetTextColor() const
{
	if (IsHovered())
	{
		constexpr float DimFactor = 0.75f;
		return FLinearColor(DimFactor, DimFactor, DimFactor, 1.0f);
	}

	return IsChecked() ? FLinearColor::White : FLinearColor::Gray;
}

FSlateColor SSourceFilterCheckBox::GetScriptSourceColor() const
{
	FLinearColor ScriptSourceColor;
	switch (Source)
	{
	case EVoxelMetaGraphScriptSource::Voxel:
		{
			ScriptSourceColor = FLinearColor(0.494792f, 0.445313f, 0.445313f, 1.f);
			break;
		}
	case EVoxelMetaGraphScriptSource::Game:
		{
			ScriptSourceColor = FLinearColor(0.510417f, 0.300158f, 0.051042f, 1.f);
			break;
		}
	case EVoxelMetaGraphScriptSource::Plugins:
		{
			ScriptSourceColor = FLinearColor(0.048438f, 0.461547f, 0.484375f, 1.f);
			break;
		}
	case EVoxelMetaGraphScriptSource::Developer:
		{
			ScriptSourceColor = FLinearColor(0.070312f, 0.46875f, 0.100547f, 1.f);
			break;
		}
	default:
		{
			ScriptSourceColor = FLinearColor(1.f, 1.f, 1.f, 0.3);
			break;
		}
	}

	if (!IsChecked())
	{
		ScriptSourceColor.A = 0.1f;
	}

	return ScriptSourceColor;
}

FMargin SSourceFilterCheckBox::GetFilterNamePadding() const
{
	return bIsPressed ? FMargin(2, 2, 1, 0) : FMargin(2, 1, 1, 1);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SSourceFilterBox::Construct(const FArguments& Args)
{
	OnFiltersChanged = Args._OnFiltersChanged;

	const TSharedRef<SHorizontalBox> SourceContainer = SNew(SHorizontalBox);

	SourceContainer->AddSlot()
	.AutoWidth()
	.Padding(5.f)
	[
		SNew(SBorder)
        .BorderImage(FVoxelEditorStyle::GetBrush("Graph.NewAssetDialog.FilterCheckBox.Border"))
        .ToolTipText(VOXEL_LOCTEXT("Show all"))
        .Padding(3.f)
		[
			SNew(SCheckBox)
            .Style(FVoxelEditorStyle::Get(), "Graph.NewAssetDialog.FilterCheckBox")
            .BorderBackgroundColor_Lambda([=]() -> FSlateColor
			{
			    bool bChecked = true;
			    for (int32 SourceIndex = 0; SourceIndex < int32(EVoxelMetaGraphScriptSource::Unknown); SourceIndex++)
			    {
			        bChecked &= IsFilterActive(SourceIndex);
			    }

			    return bChecked ? FLinearColor::White : FSlateColor::UseForeground();
			})
            .IsChecked_Lambda([=]() -> ECheckBoxState
			{
			    bool bChecked = true;
			    for (int32 SourceIndex = 0; SourceIndex < int32(EVoxelMetaGraphScriptSource::Unknown); SourceIndex++)
			    {
			        bChecked &= IsFilterActive(SourceIndex);
			    }

			    return bChecked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
            .OnCheckStateChanged(FOnCheckStateChanged::CreateLambda([=](ECheckBoxState NewState)
			{
			    bool bAnyChange = false;
			    for (int32 SourceIndex = 0; SourceIndex < int32(EVoxelMetaGraphScriptSource::Unknown); SourceIndex++)
			    {
			        if (!IsFilterActive(SourceIndex))
			        {
			            bAnyChange = true;
			        }
			    }

			    if (bAnyChange)
			    {
			        for (int32 SourceIndex = 0; SourceIndex < int32(EVoxelMetaGraphScriptSource::Unknown); SourceIndex++)
			        {
			            SourceState.Add(GetScriptSource(SourceIndex), true);
			        }

			        BroadcastFiltersChanged();
			    }
			}))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(2.f)
				[
					SNew(STextBlock)
                    .Text(VOXEL_LOCTEXT("Show all"))
                    .ShadowOffset(0.0f)
                    .ColorAndOpacity_Lambda([=]() -> FSlateColor
					{
					    bool bChecked = true;
					    for (int32 SourceIndex = 0; SourceIndex < int32(EVoxelMetaGraphScriptSource::Unknown); SourceIndex++)
					    {
					        bChecked &= IsFilterActive(SourceIndex);
					    }
                    	
					    return bChecked ? FLinearColor::Black : FLinearColor::Gray;
					})
                    .TextStyle(FVoxelEditorStyle::Get(), "Graph.NewAssetDialog.ActionFilterTextBlock")
				]
			]
		]
	];

	for (int32 SourceIndex = 0; SourceIndex < int32(EVoxelMetaGraphScriptSource::Unknown); SourceIndex++)
	{
		SourceContainer->AddSlot()
		.AutoWidth()
		.Padding(2.f)
		[
			SNew(SBorder)
    		.BorderImage(FEditorAppStyle::GetBrush(TEXT("NoBorder")))
			.Padding(3.f)
			[
				SNew(SSourceFilterCheckBox, SSourceFilterBox::GetScriptSource(SourceIndex))
    			.IsChecked_Lambda([=]
				{
				    return IsFilterActive(SourceIndex) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
    			.OnSourceStateChanged_Lambda([=](EVoxelMetaGraphScriptSource Source, bool bState)
				{
				    SourceState.Add(Source, bState);
				    BroadcastFiltersChanged();
				})
    			.OnShiftClicked_Lambda([=](EVoxelMetaGraphScriptSource ChangedSource, bool bState)
				{
				    TArray<EVoxelMetaGraphScriptSource> Keys;
				    SourceState.GenerateKeyArray(Keys);

				    for (EVoxelMetaGraphScriptSource& Source : Keys)
				    {
				        SourceState.Add(Source, Source == ChangedSource);
				    }

				    BroadcastFiltersChanged();
				})
			]
		];
	}

	ChildSlot
	[
		SourceContainer
	];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SFilterBox::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(3, 1)
			[
				SNew(STextBlock)
				.Text(VOXEL_LOCTEXT("Source Filtering"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(3, 1)
			[
				SAssignNew(SourceFilterBox, SSourceFilterBox)
				.OnFiltersChanged(InArgs._OnSourceFiltersChanged)
			]
		]
	];
}

bool SFilterBox::IsSourceFilterActive(const FAssetData& Item) const
{
	return SourceFilterBox.IsValid() &&
		SourceFilterBox->IsFilterActive(GetScriptSource(Item));
}

EVoxelMetaGraphScriptSource SFilterBox::GetScriptSource(const FAssetData& ScriptAssetData)
{
	TArray<FString> AssetPathParts;
	ScriptAssetData.UE_501_SWITCH(ObjectPath, GetSoftObjectPath()).ToString().ParseIntoArray(AssetPathParts, TEXT("/"));

	if (!ensure(!AssetPathParts.IsEmpty()))
	{
		return EVoxelMetaGraphScriptSource::Unknown;
	}
	
	if (AssetPathParts[0] == "VoxelPlugin")
	{
		return EVoxelMetaGraphScriptSource::Voxel;
	}
	else if (AssetPathParts[0] == "Game")
	{
		if (AssetPathParts.Num() > 1)
		{
			if (AssetPathParts[1] == "Developers")
			{
				return EVoxelMetaGraphScriptSource::Developer;
			}
			else
			{
				return EVoxelMetaGraphScriptSource::Game;
			}
		}
	}

	return EVoxelMetaGraphScriptSource::Plugins;
}

END_VOXEL_NAMESPACE(MetaGraph)