// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "SVoxelMetaGraphAssetPickerList.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

class SNewAssetDialog : public SWindow
{
public:
	VOXEL_SLATE_ARGS()
	{
	};

	void Construct(const FArguments& InArgs);

public:
	bool GetUserConfirmedSelection() const
	{
		return bUserConfirmedSelection;
	}

	TOptional<FAssetData> GetSelectedAsset();

private:
	void ConfirmSelection(const FAssetData& AssetData);

	FReply OnConfirmSelection();
	bool IsCreateButtonEnabled() const;
	FReply OnCancelButtonClicked();

private:
	TArray<FAssetData> SelectedAssets;
	bool bUserConfirmedSelection = false;
	TSharedPtr<SAssetPickerList> NewAssetPicker;
};

END_VOXEL_NAMESPACE(MetaGraph)