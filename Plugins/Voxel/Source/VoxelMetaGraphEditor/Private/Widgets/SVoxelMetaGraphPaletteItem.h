// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "SGraphPalette.h"

class FVoxelMetaGraphEditorToolkit;
struct FVoxelPinType;
struct FVoxelMetaGraphParameter;
struct FVoxelMetaGraphMemberSchemaAction;

BEGIN_VOXEL_NAMESPACE(MetaGraph)

class SMembers;

class SPaletteItem : public SGraphPaletteItem
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_ARGUMENT(TSharedPtr<SMembers>, MembersWidget)
		SLATE_ARGUMENT(TWeakPtr<FVoxelMetaGraphEditorToolkit>, Toolkit)
	};

	void Construct(const FArguments& InArgs, FCreateWidgetForActionData* const InCreateData);

protected:
	//~ Begin SGraphPaletteItem Interface
	virtual TSharedRef<SWidget> CreateTextSlotWidget(FCreateWidgetForActionData* const InCreateData, TAttribute<bool> bIsReadOnly) override;
	virtual void OnNameTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit) override;
	virtual FText GetDisplayText() const override;
	//~ End SGraphPaletteItem Interface

private:
	TSharedPtr<SMembers> MembersWidget;
	TWeakPtr<FVoxelMetaGraphEditorToolkit> WeakToolkit;

	int32 CategoriesCount = 0;
};

END_VOXEL_NAMESPACE(MetaGraph)