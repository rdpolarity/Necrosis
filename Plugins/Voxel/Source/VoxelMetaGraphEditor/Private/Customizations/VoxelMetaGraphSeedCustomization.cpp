// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "VoxelMetaGraphSeed.h"

VOXEL_CUSTOMIZE_STRUCT_HEADER(FVoxelMetaGraphSeed)(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	const TSharedRef<IPropertyHandle>& SeedHandle = PropertyHandle->GetChildHandleStatic(FVoxelMetaGraphSeed, Seed);
	
	HeaderRow.
	NameContent()
	[
		SeedHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		[
			SeedHandle->CreatePropertyValueWidget()
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.f, 2.f)
		[
			SNew(SBox)
			.WidthOverride(16.f)
			.HeightOverride(16.f)
			[
				SNew(SButton)
				.ButtonStyle(FVoxelEditorStyle::Get(), "Graph.Seed.Dice")
				.OnClicked_Lambda([WeakHandle = MakeWeakPtr(PropertyHandle)]
				{
					const TSharedPtr<IPropertyHandle>& Handle = WeakHandle.Pin();
					if (!Handle)
					{
						return FReply::Handled();
					}

					FVoxelMetaGraphSeed NewSeed;
					NewSeed.Randomize();
					Handle->GetChildHandleStatic(FVoxelMetaGraphSeed, Seed)->SetValue(NewSeed.Seed);
					
					return FReply::Handled();
				})
			]
		]
	];
}