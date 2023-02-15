// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Tools/UEdMode.h"
#include "InteractiveToolManager.h"
#include "BaseTools/SingleClickTool.h"
#include "VoxelSmoothCanvasEdMode.generated.h"

class FVoxelSmoothCanvasCommands : public TVoxelCommands<FVoxelSmoothCanvasCommands>
{
public:
	TSharedPtr<FUICommandInfo> Select;

	virtual void RegisterCommands() override;
};

UCLASS()
class UVoxelSmoothCanvasSelectToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Test)
	bool bTest = true;

};

UCLASS()
class UVoxelSmoothCanvasSelectTool : public USingleClickTool
{
	GENERATED_BODY()

public:
	//~ Begin USingleClickTool Interface
	virtual void Setup() override;
	virtual FInputRayHit IsHitByClick(const FInputDeviceRay& ClickPos) override;
	virtual void OnClicked(const FInputDeviceRay& ClickPos) override;
	//~ End USingleClickTool Interface

private:
	UPROPERTY(Transient)
	TObjectPtr<UVoxelSmoothCanvasSelectToolProperties> Properties;
};

UCLASS()
class UVoxelSmoothCanvasSelectToolBuilder : public USingleClickToolBuilder
{
	GENERATED_BODY()

public:
	//~ Begin USingleClickToolBuilder Interface
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override
	{
		return true;
	}
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override
	{
		return NewObject<UVoxelSmoothCanvasSelectTool>(SceneState.ToolManager);
	}
	//~ End USingleClickToolBuilder Interface
};

UCLASS(config = EditorPerProjectUserSettings)
class UVoxelSmoothCanvasEdModeSettings : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(config, EditAnywhere, Category = Test)
	int32 Test = 0;
};

UCLASS()
class UVoxelSmoothCanvasEdMode : public UEdMode
{
	GENERATED_BODY()

public:
	UVoxelSmoothCanvasEdMode();

	virtual void Enter() override;
	virtual void Exit() override;

	virtual void CreateToolkit() override;
};