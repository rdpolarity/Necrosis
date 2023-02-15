// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "SVoxelMetaGraphFilterBox.generated.h"

UENUM()
enum class EVoxelMetaGraphScriptSource : uint8
{
	Voxel,
	Game,
	Plugins,
	Developer,
	Unknown
};

BEGIN_VOXEL_NAMESPACE(MetaGraph)

class SSourceFilterCheckBox : public SCheckBox
{
public:
	DECLARE_DELEGATE_TwoParams(FOnSourceStateChanged, EVoxelMetaGraphScriptSource, bool);
	DECLARE_DELEGATE_TwoParams(FOnShiftClicked, EVoxelMetaGraphScriptSource, bool);
	
	VOXEL_SLATE_ARGS()
	{
		SLATE_EVENT(FOnSourceStateChanged, OnSourceStateChanged)
		SLATE_EVENT(FOnShiftClicked, OnShiftClicked)
		SLATE_ATTRIBUTE(ECheckBoxState, IsChecked)
	};

	void Construct(const FArguments& Args, EVoxelMetaGraphScriptSource Source);

	//~ Begin SCheckBox Interface
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	//~ End SCheckBox Interface

private:
	FSlateColor GetTextColor() const;
	FSlateColor GetScriptSourceColor() const;
	FMargin GetFilterNamePadding() const;

private:	
	EVoxelMetaGraphScriptSource Source = EVoxelMetaGraphScriptSource::Voxel;
	
	FOnSourceStateChanged OnSourceStateChanged;
	FOnShiftClicked OnShiftClicked;
};

class SSourceFilterBox : public SCompoundWidget
{
public:
	typedef TMap<EVoxelMetaGraphScriptSource, bool> SourceMap;
	DECLARE_DELEGATE_OneParam(FOnFiltersChanged, const SourceMap&);
	
	VOXEL_SLATE_ARGS()
	{
		SLATE_EVENT(FOnFiltersChanged, OnFiltersChanged)
	};

	void Construct(const FArguments& Args);

public:
	static bool IsFilterActive(EVoxelMetaGraphScriptSource Source)
	{
		if (SourceState.Contains(Source))
		{
			return SourceState[Source];
		}

		return true;
	}

private:
	void BroadcastFiltersChanged() const
	{
		OnFiltersChanged.Execute(SourceState);
	}
	
	static EVoxelMetaGraphScriptSource GetScriptSource(int32 Index)
	{
		static const UEnum* ScriptSourceEnum = StaticEnum<EVoxelMetaGraphScriptSource>();
		return EVoxelMetaGraphScriptSource(ScriptSourceEnum->GetValueByIndex(Index));
	}
	
	static bool IsFilterActive(int32 Index)
	{
		return IsFilterActive(GetScriptSource(Index));
	}

private:
	FOnFiltersChanged OnFiltersChanged;
	TMap<EVoxelMetaGraphScriptSource, TSharedRef<SSourceFilterCheckBox>> SourceButtons;
	
	static TMap<EVoxelMetaGraphScriptSource, bool> SourceState;
};

class SFilterBox : public SCompoundWidget
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_EVENT(SSourceFilterBox::FOnFiltersChanged, OnSourceFiltersChanged)
		SLATE_ARGUMENT(UClass*, Class)
	};

	void Construct(const FArguments& InArgs);

	bool IsSourceFilterActive(const FAssetData& Item) const;

private:
	static EVoxelMetaGraphScriptSource GetScriptSource(const FAssetData& ScriptAssetData);

private:
	TSharedPtr<SSourceFilterBox> SourceFilterBox;
};

END_VOXEL_NAMESPACE(MetaGraph)