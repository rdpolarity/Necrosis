// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "VoxelRuntime/VoxelSubsystem.h"
#include "VoxelRuntime/VoxelRuntimeUtilities.h"
#include "LevelEditor.h"

class FVoxelEditorCommands : public TVoxelCommands<FVoxelEditorCommands>
{
public:
	TSharedPtr<FUICommandInfo> RefreshVoxelWorlds;

	virtual void RegisterCommands() override;
};

DEFINE_VOXEL_COMMANDS(FVoxelEditorCommands);

void FVoxelEditorCommands::RegisterCommands()
{
	VOXEL_UI_COMMAND(RefreshVoxelWorlds, "Refresh", "Refresh the voxel worlds in the scene", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::F5));
}

VOXEL_RUN_ON_STARTUP_EDITOR(RegisterVoxelEditorCommands)
{
	FVoxelEditorCommands::Register();

	const FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	FUICommandList& Actions = *LevelEditorModule.GetGlobalLevelEditorActions();

	Actions.MapAction(FVoxelEditorCommands::Get().RefreshVoxelWorlds, MakeLambdaDelegate([]
	{
		FVoxelRuntimeUtilities::ForeachRuntime(nullptr, [](const FVoxelRuntime& Runtime)
		{
			FVoxelRuntimeUtilities::RecreateRuntime(Runtime);
		});
	}));
}