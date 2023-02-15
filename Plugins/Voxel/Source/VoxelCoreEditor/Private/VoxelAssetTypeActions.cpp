// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "AssetTypeActions_VoxelBase.h"
#include "Toolkits/VoxelBaseEditorToolkit.h"

VOXEL_RUN_ON_STARTUP_EDITOR(RegisterVoxelAssetTypes)
{
	VOXEL_FUNCTION_COUNTER();
	
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	ForEachObjectOfClass<UClass>([&](UClass* Class)
	{
		if (!Class->HasMetaDataHierarchical(STATIC_FNAME("VoxelAssetType")))
		{
			return;
		}

		TArray<FText> SubMenus;
		FString AssetSubMenu;
		if (Class->GetStringMetaDataHierarchical("AssetSubMenu", &AssetSubMenu))
		{
			TArray<FString> SubMenuStrings;
			AssetSubMenu.ParseIntoArray(SubMenuStrings, TEXT("."));
			
			for (const FString& SubMenu : SubMenuStrings)
			{
				if (!ensure(!SubMenu.IsEmpty()))
				{
					continue;
				}
				
				SubMenus.Add(FText::FromString(SubMenu));
			}
		}

		FColor Color = FColor::Black;
		FString AssetColor;
		if (Class->GetStringMetaDataHierarchical("AssetColor", &AssetColor))
		{
			if (AssetColor == "Orange")
			{
				Color = FColor(255, 140, 0);
			}
			else if (AssetColor == "DarkGreen")
			{
				Color = FColor(0, 192, 0);
			}
			else if (AssetColor == "LightGreen")
			{
				Color = FColor(128, 255, 128);
			}
			else if (AssetColor == "Blue")
			{
				Color = FColor(0, 175, 255);
			}
			else if (AssetColor == "LightBlue")
			{
				Color = FColor(0, 175, 175);
			}
			else if (AssetColor == "Red")
			{
				Color = FColor(128, 0, 64);
			}
		}

		const TSharedRef<FAssetTypeActions_Voxel> Action = MakeShared<FAssetTypeActions_Voxel>(
			Class->GetDisplayNameText(),
			Color,
			Class,
			SubMenus,
			[=]
			{
				return FVoxelBaseEditorToolkit::MakeToolkit(Class);
			});

		AssetTools.RegisterAssetTypeActions(Action);
	});
}