// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "VoxelFoliageInstanceTemplate.h"

VOXEL_CUSTOMIZE_STRUCT_CHILDREN(FVoxelFoliageSpawnRestriction)(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	const TSharedRef<IPropertyHandle> EnableSlopeRestrictionHandle = PropertyHandle->GetChildHandleStatic(FVoxelFoliageSpawnRestriction, bEnableSlopeRestriction);
	const TSharedRef<IPropertyHandle> GroundSlopeAngleHandle = PropertyHandle->GetChildHandleStatic(FVoxelFoliageSpawnRestriction, GroundSlopeAngle);

	const TSharedRef<IPropertyHandle> EnableHeightRestrictionHandle = PropertyHandle->GetChildHandleStatic(FVoxelFoliageSpawnRestriction, bEnableHeightRestriction);
	const TSharedRef<IPropertyHandle> HeightRestrictionHandle = PropertyHandle->GetChildHandleStatic(FVoxelFoliageSpawnRestriction, HeightRestriction);
	const TSharedRef<IPropertyHandle> HeightRestrictionFalloffHandle = PropertyHandle->GetChildHandleStatic(FVoxelFoliageSpawnRestriction, HeightRestrictionFalloff);

	{
		IDetailGroup& Group = ChildBuilder.AddGroup("Slope", VOXEL_LOCTEXT("Slope"));

		Group.HeaderRow()
		.NameContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.f, 0.f, 2.f, 0.f)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				EnableSlopeRestrictionHandle->CreatePropertyValueWidget()
			]
			+ SHorizontalBox::Slot()
			.Padding(2.f, 0.f, 0.f, 0.f)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Font(FEditorAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				.Text(VOXEL_LOCTEXT("Slope"))
				.ToolTipText(VOXEL_LOCTEXT("Slope"))
			]
		];

		Group.AddPropertyRow(GroundSlopeAngleHandle);
	}

	{
		IDetailGroup& Group = ChildBuilder.AddGroup("Height", VOXEL_LOCTEXT("Height"));

		Group.HeaderRow()
		.NameContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.f, 0.f, 2.f, 0.f)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				EnableHeightRestrictionHandle->CreatePropertyValueWidget()
			]
			+ SHorizontalBox::Slot()
			.Padding(2.f, 0.f, 0.f, 0.f)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Font(FEditorAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				.Text(VOXEL_LOCTEXT("Height"))
				.ToolTipText(VOXEL_LOCTEXT("Height"))
			]
		];

		Group.AddPropertyRow(HeightRestrictionHandle);
		Group.AddPropertyRow(HeightRestrictionFalloffHandle);
	}
}