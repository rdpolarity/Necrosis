// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelSmoothCanvasEdMode.h"
#include "VoxelSmoothCanvasToolkit.h"
#include "EdModeInteractiveToolsContext.h"

DEFINE_VOXEL_COMMANDS(FVoxelSmoothCanvasCommands);

void FVoxelSmoothCanvasCommands::RegisterCommands()
{
	VOXEL_UI_COMMAND(Select, "Select", "Select the canvas to sculpt", EUserInterfaceActionType::ToggleButton, {});
}

void UVoxelSmoothCanvasSelectTool::Setup()
{
	Super::Setup();

	Properties = NewObject<UVoxelSmoothCanvasSelectToolProperties>(this);
	Properties->RestoreProperties(this);
	AddToolPropertySource(Properties);
}

FInputRayHit UVoxelSmoothCanvasSelectTool::IsHitByClick(const FInputDeviceRay& ClickPos)
{
	FViewport* FocusedViewport = GetToolManager()->GetContextQueriesAPI()->GetFocusedViewport();
	if (!FocusedViewport)
	{
		return {};
	}

	HHitProxy* HitProxy = FocusedViewport->GetHitProxy(ClickPos.ScreenPosition.X, ClickPos.ScreenPosition.Y);
	if (!HitProxy)
	{
		return {};
	}

	HActor* ActorHitProxy = HitProxyCast<HActor>(HitProxy);
	if (!ActorHitProxy)
	{
		return {};
	}

	UE_DEBUG_BREAK();
	return {};
}

void UVoxelSmoothCanvasSelectTool::OnClicked(const FInputDeviceRay& ClickPos)
{
	
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UVoxelSmoothCanvasEdMode::UVoxelSmoothCanvasEdMode()
{
	SettingsClass = UVoxelSmoothCanvasEdModeSettings::StaticClass();

	Info = FEditorModeInfo(
		"VoxelSmoothCanvasEdMode",
		VOXEL_LOCTEXT("Voxel Smooth Canvas"),
		FSlateIcon(FVoxelEditorStyle::GetStyleSetName(), "VoxelIcon"),
		true);
}

void UVoxelSmoothCanvasEdMode::Enter()
{
	Super::Enter();
	
	const FVoxelSmoothCanvasCommands& Commands = FVoxelSmoothCanvasCommands::Get();

	RegisterTool(
		Commands.Select,
		GetClassName<UVoxelSmoothCanvasSelectTool>(),
		NewObject<UVoxelSmoothCanvasSelectToolBuilder>(this));
	
	GetInteractiveToolsContext()->StartTool(GetClassName<UVoxelSmoothCanvasSelectTool>());
}

void UVoxelSmoothCanvasEdMode::Exit()
{
	Super::Exit();
}

void UVoxelSmoothCanvasEdMode::CreateToolkit()
{
	Toolkit = MakeShared<FVoxelSmoothCanvasToolkit>();
}