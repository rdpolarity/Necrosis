// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

struct FStatsRow : public TSlotBase<FStatsRow>
{
	SLATE_SLOT_BEGIN_ARGS(FStatsRow, TSlotBase<FStatsRow>)
		SLATE_ARGUMENT(FText, Header)
		SLATE_ARGUMENT(FText, Tooltip)
		SLATE_ATTRIBUTE(FText, Value)
	SLATE_SLOT_END_ARGS()

	FStatsRow()
	{
	}
	FStatsRow(const FText Header, const FText Tooltip, const TAttribute<FText> Value)
		: Header(Header)
		, Tooltip(Tooltip)
		, Value(Value)
	{
	}

	FText Header;
	FText Tooltip;
	TAttribute<FText> Value;
};

class SPreviewStats : public SCompoundWidget
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_SLOT_ARGUMENT(FStatsRow, Rows)
	};

	static FStatsRow::FSlotArguments Row()
	{
		return FStatsRow::FSlotArguments(MakeUnique<FStatsRow>());
	}

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<ITableRow> CreateRow(TSharedPtr<FStatsRow> StatsRow, const TSharedRef<STableViewBase>& OwnerTable) const;

	TArray<TSharedPtr<FStatsRow>> MessagesList;
	TSharedPtr<SListView<TSharedPtr<FStatsRow>>> MessageListView;
};

END_VOXEL_NAMESPACE(MetaGraph)