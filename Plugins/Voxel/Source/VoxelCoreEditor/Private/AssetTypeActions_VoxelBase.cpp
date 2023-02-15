// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "AssetTypeActions_VoxelBase.h"
#include "Toolkits/VoxelBaseEditorToolkit.h"

EAssetTypeCategories::Type GVoxelAssetCategory;

VOXEL_RUN_ON_STARTUP_EDITOR(RegisterVoxelAssetCategory)
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	GVoxelAssetCategory = AssetTools.RegisterAdvancedAssetCategory("Voxel", VOXEL_LOCTEXT("Voxel"));
}

uint32 FAssetTypeActions_VoxelBase::GetCategories()
{
	return GVoxelAssetCategory;
}

void FAssetTypeActions_VoxelBase::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
	UClass* Class = GetSupportedClass();

	bool bCanReimport = true;
	for (UObject* Object : InObjects)
	{
		if (!Object || !Object->IsA(Class))
		{
			return;
		}

		bCanReimport &= FReimportManager::Instance()->CanReimport(Object);
	}

	if (bCanReimport)
	{
		MenuBuilder.AddMenuEntry(
			VOXEL_LOCTEXT("Reimport"),
			VOXEL_LOCTEXT("Reimport the selected asset(s)."),
			FSlateIcon(UE_501_SWITCH(FEditorStyle::GetStyleSetName(), FAppStyle::GetAppStyleSetName()), "ContentBrowser.AssetActions.ReimportAsset"),
			FUIAction(MakeLambdaDelegate([=]
			{
				for (UObject* Object : InObjects)
				{
					FReimportManager::Instance()->Reimport(Object, true);
				}
			})));
	}

	for (UFunction* Function : GetClassFunctions(Class))
	{
		if (!Function->HasMetaData(STATIC_FNAME("ShowInContextMenu")))
		{
			continue;
		}
		if (Function->Children)
		{
			ensureMsgf(false, TEXT("Function %s has ShowInContextMenu but has parameters!"), *Function->GetDisplayNameText().ToString());
			Function->RemoveMetaData(STATIC_FNAME("ShowInContextMenu"));
			return;
		}

		MenuBuilder.AddMenuEntry(
			Function->GetDisplayNameText(),
			Function->GetToolTipText(),
			FSlateIcon(NAME_None, NAME_None),
			FUIAction(MakeLambdaDelegate([=]
			{
				for (UObject* Object : InObjects)
				{
					FVoxelObjectUtilities::InvokeFunctionWithNoParameters(Object, Function);
				}
			})));
	}
}

void FAssetTypeActions_VoxelBase::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	for (UObject* Object : InObjects)
	{
		const TSharedPtr<FVoxelBaseEditorToolkit> NewVoxelEditor = MakeToolkit();
		if (!NewVoxelEditor)
		{
			FAssetTypeActions_Base::OpenAssetEditor(InObjects, EditWithinLevelEditor);
			return;
		}

		if (ensure(Object) &&
			ensure(NewVoxelEditor))
		{
			const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;
			NewVoxelEditor->InitVoxelEditor(Mode, EditWithinLevelEditor, Object);
		}
	}
}