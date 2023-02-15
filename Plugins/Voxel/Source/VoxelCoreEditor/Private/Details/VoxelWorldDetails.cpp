// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelEditorMinimal.h"

// TODO
VOXEL_RUN_ON_STARTUP_EDITOR(RegisterVoxelWorldSections)
{
	static const FName PropertyEditor("PropertyEditor");
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditor);

	{
		TSharedRef<FPropertySection> Section = PropertyModule.FindOrCreateSection("VoxelWorld", "General", VOXEL_LOCTEXT("General"));
		Section->AddCategory("Default");
		Section->AddCategory("General");
		Section->AddCategory("Storage");
	}

	{
		TSharedRef<FPropertySection> Section = PropertyModule.FindOrCreateSection("VoxelWorld", "Generation", VOXEL_LOCTEXT("Generation"));
		Section->AddCategory("Generation");
		Section->AddCategory("Presets");
		Section->AddCategory("Subsystems");
	}

	{
		TSharedRef<FPropertySection> Section = PropertyModule.FindOrCreateSection("VoxelWorld", "Rendering", VOXEL_LOCTEXT("Rendering"));
		Section->AddCategory("Material");
		Section->AddCategory("Rendering");
		Section->AddCategory("LOD");
	}

	{
		TSharedRef<FPropertySection> Section = PropertyModule.FindOrCreateSection("VoxelWorld", "Physics", VOXEL_LOCTEXT("Physics"));
		Section->AddCategory("Collision");
	}

	{
		TSharedRef<FPropertySection> Section = PropertyModule.FindOrCreateSection("VoxelWorld", "Misc", VOXEL_LOCTEXT("Misc"));
		Section->AddCategory("Misc");
		Section->AddCategory("Actor");
		Section->AddCategory("Mesh Generation");
	}
}