// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "SVoxelMetaGraphFilterBox.h"
#include "Widgets/SItemSelector.h"

class FAssetThumbnailPool;

BEGIN_VOXEL_NAMESPACE(MetaGraph)

typedef SItemSelector<FText, FAssetData> SAssetItemSelector;

class SAssetPickerList : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnTemplateAssetActivated, const FAssetData&);
	
public:
	VOXEL_SLATE_ARGS() 
	{
		SLATE_EVENT(FOnTemplateAssetActivated, OnTemplateAssetActivated);
	};

	void Construct(const FArguments& InArgs, UClass* AssetClass);

	TArray<FAssetData> GetSelectedAssets() const;

private:
	TArray<FText> OnGetCategoriesForItem(const FAssetData& Item) const;
	TSharedRef<SWidget> OnGenerateWidgetForCategory(const FText& Category) const;
	TSharedRef<SWidget> OnGenerateWidgetForItem(const FAssetData& Item);

private:
	TArray<FAssetData> GetAssetDataForSelector(const UClass* AssetClass) const;
	void TriggerRefresh(const TMap<EVoxelMetaGraphScriptSource, bool>& SourceState) const;

	static FString GetAssetName(const FAssetData& Item);

private:
	TSharedPtr<SFilterBox> FilterBox;
	TSharedPtr<SAssetItemSelector> ItemSelector;

	FOnTemplateAssetActivated OnTemplateAssetActivated;
	
	static FText VoxelPluginCategory;
	static FText ProjectCategory;
	static FText UncategorizedCategory;
};

END_VOXEL_NAMESPACE(MetaGraph)