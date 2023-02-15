// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "SVoxelMetaGraphAssetPickerList.h"
#include "VoxelMetaGraph.h"
#include "AssetRegistry/AssetRegistryModule.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

FText SAssetPickerList::VoxelPluginCategory = VOXEL_LOCTEXT("Voxel Plugin Provided");
FText SAssetPickerList::ProjectCategory = VOXEL_LOCTEXT("Project");
FText SAssetPickerList::UncategorizedCategory = VOXEL_LOCTEXT("Uncategorized");

void SAssetPickerList::Construct(const FArguments& InArgs, UClass* AssetClass)
{
	OnTemplateAssetActivated = InArgs._OnTemplateAssetActivated;

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(3.f)
		[
			SAssignNew(FilterBox, SFilterBox)
			.OnSourceFiltersChanged(this, &SAssetPickerList::TriggerRefresh)
			.Class(AssetClass)
		]
		+ SVerticalBox::Slot()
		[
			SAssignNew(ItemSelector, SAssetItemSelector)
			.Items(GetAssetDataForSelector(AssetClass))
			.AllowMultiselect(false)
			.ClickActivateMode(EItemSelectorClickActivateMode::DoubleClick)
			.OnGetCategoriesForItem(this, &SAssetPickerList::OnGetCategoriesForItem)
			.OnCompareCategoriesForEquality_Lambda([](const FText& CategoryA, const FText& CategoryB)
			{
				return CategoryA.CompareTo(CategoryB) == 0;
			})
			.OnCompareCategoriesForSorting_Lambda([](const FText& CategoryA, const FText& CategoryB)
			{
				if (CategoryB.EqualTo(VoxelPluginCategory))
				{
					return false;
				}
				else if (CategoryA.EqualTo(VoxelPluginCategory))
				{
					return true;
				}

				return CategoryA.CompareTo(CategoryB) < 0;
			})
			.OnCompareItemsForSorting_Lambda([](const FAssetData& ItemA, const FAssetData& ItemB)
			{
				return GetAssetName(ItemA) < GetAssetName(ItemB);
			})
			.OnDoesItemMatchFilterText_Lambda([](const FText& FilterText, const FAssetData& Item)
			{
				TArray<FString> FilterStrings;
				FilterText.ToString().ParseIntoArrayWS(FilterStrings, TEXT(","));

				const FString AssetNameString = GetAssetName(Item);
				for (const FString& FilterString : FilterStrings)
				{
					if (!AssetNameString.Contains(FilterString))
					{
						return false;
					}
				}

				return true;
			})
			.OnGenerateWidgetForCategory(this, &SAssetPickerList::OnGenerateWidgetForCategory)
			.OnGenerateWidgetForItem(this, &SAssetPickerList::OnGenerateWidgetForItem)
			.OnItemActivated_Lambda([&](const FAssetData& Item)
			{
				OnTemplateAssetActivated.ExecuteIfBound(Item);
			})
			.OnDoesItemPassCustomFilter_Lambda([&](const FAssetData& Item)
			{
				return FilterBox->IsSourceFilterActive(Item);
			})
			.ExpandInitially(true)
		]
	];

	FSlateApplication::Get().SetKeyboardFocus(ItemSelector->GetSearchBox());
}

TArray<FAssetData> SAssetPickerList::GetSelectedAssets() const
{
	return ItemSelector->GetSelectedItems();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TArray<FText> SAssetPickerList::OnGetCategoriesForItem(const FAssetData& Item) const
{
	TArray<FText> Categories;

	TArray<FString> AssetPathParts;
	Item.UE_501_SWITCH(ObjectPath, GetSoftObjectPath()).ToString().ParseIntoArray(AssetPathParts, TEXT("/"));
	
	if (ensure(!AssetPathParts.IsEmpty()))
	{
		if (AssetPathParts[0] == TEXT("VoxelPlugin"))
		{
			Categories.Add(VoxelPluginCategory);
		}
		else if (AssetPathParts[0] == TEXT("Game"))
		{
			Categories.Add(ProjectCategory);
		}
		else
		{
			Categories.Add(FText::FromString("Plugin - " + AssetPathParts[0]));
		}
	}
	else
	{
		Categories.Add(UncategorizedCategory);
	}

	return Categories;
}

TSharedRef<SWidget> SAssetPickerList::OnGenerateWidgetForCategory(const FText& Category) const
{
	return SNew(SBox)
		.Padding(FMargin(5, 5, 5, 3))
		[
			SNew(STextBlock)
			.TextStyle(FVoxelEditorStyle::Get(), "Graph.NewAssetDialog.AssetPickerAssetCategoryText")
			.Text(Category)
		];
}

TSharedRef<SWidget> SAssetPickerList::OnGenerateWidgetForItem(const FAssetData& Item)
{
	const TSharedRef<FAssetThumbnail> AssetThumbnail = MakeShared<FAssetThumbnail>(Item, 72, 72, FVoxelEditorUtilities::GetThumbnailPool());
	
	FAssetThumbnailConfig ThumbnailConfig;
	ThumbnailConfig.bAllowFadeIn = false;
	
	FText AssetDescription;
	if (!Item.GetTagValue(GET_MEMBER_NAME_CHECKED(UVoxelMetaGraph, Description), AssetDescription) &&
		Item.IsAssetLoaded())
	{
		const UVoxelMetaGraph* GraphAsset = Cast<UVoxelMetaGraph>(Item.GetAsset());
		if (GraphAsset != nullptr)
		{
			AssetDescription = FText::FromString(GraphAsset->Description);
		}
	}

	return
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(15, 3, 10, 3)
		[
			SNew(SBox)
			.WidthOverride(72)
			.HeightOverride(72)
			[
				AssetThumbnail->MakeThumbnailWidget(ThumbnailConfig)
			]
		]
		+ SHorizontalBox::Slot()
		.Padding(5, 0, 5, 0)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.TextStyle(FVoxelEditorStyle::Get(), "Graph.NewAssetDialog.AssetPickerBoldAssetNameText")
				.Text(FText::FromString(FName::NameToDisplayString(GetAssetName(Item), false)))
				.HighlightText_Lambda([&]
				{
					return ItemSelector->GetFilterText();
				})
			]
			+ SVerticalBox::Slot()
			[
				SNew(STextBlock)
				.Text(AssetDescription)
				.TextStyle(FVoxelEditorStyle::Get(), "Graph.NewAssetDialog.AssetPickerAssetNameText")
				.AutoWrapText(true)
			]
		];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TArray<FAssetData> SAssetPickerList::GetAssetDataForSelector(const UClass* AssetClass) const
{
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	TArray<FAssetData> MetaGraphAssets;
#if VOXEL_ENGINE_VERSION < 501
	AssetRegistryModule.Get().GetAssetsByClass(AssetClass->GetFName(), MetaGraphAssets);
#else
	AssetRegistryModule.Get().GetAssetsByClass(AssetClass->GetClassPathName(), MetaGraphAssets);
#endif

	return MetaGraphAssets;
}

void SAssetPickerList::TriggerRefresh(const TMap<EVoxelMetaGraphScriptSource, bool>& SourceState) const
{
	ItemSelector->RefreshAllCurrentItems();
	ItemSelector->ExpandTree();
}

FString SAssetPickerList::GetAssetName(const FAssetData& Item)
{
	FString Result = Item.AssetName.ToString();
	Result.RemoveFromStart("VMG_");
	return Result;
}

END_VOXEL_NAMESPACE(MetaGraph)