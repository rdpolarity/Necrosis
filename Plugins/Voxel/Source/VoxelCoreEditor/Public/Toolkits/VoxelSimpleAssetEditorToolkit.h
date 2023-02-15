// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Toolkits/VoxelBaseEditorToolkit.h"
#include "Toolkits/VoxelSimplePreviewScene.h"

class SViewportToolBar;
class SVoxelSimpleAssetEditorViewport;

class VOXELCOREEDITOR_API FVoxelSimpleAssetEditorToolkit : public FVoxelBaseEditorToolkit, public FTickableEditorObject
{
public:
	explicit FVoxelSimpleAssetEditorToolkit(const FString& BaseToolkitName)
		: FVoxelBaseEditorToolkit(BaseToolkitName)
	{
	}

public:
	//~ Begin FVoxelBaseEditorToolkit Interface
	virtual void CreateInternalWidgets() override;
	virtual TSharedRef<FTabManager::FLayout> GetLayout() const override;
	virtual void RegisterTabs(FRegisterTab RegisterTab) override;
	//~ End FVoxelBaseEditorToolkit Interface

	//~ Begin IToolkit Interface
	virtual void PostRegenerateMenusAndToolbars() override;
	//~ End IToolkit Interface

	//~ Begin FNotifyHook Interface
	virtual void NotifyPreChange(FProperty* PropertyAboutToChange) override;
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) final override;
	//~ End FNotifyHook Interface
	
	//~ Begin FTickableGameObject Interface
	virtual void Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
	virtual TStatId GetStatId() const override;
	//~ End FTickableGameObject Interface

	FVoxelSimplePreviewScene& GetPreviewScene() const
	{
		return *PreviewScene;
	}

	virtual void SetupPreview() {}
	virtual void UpdatePreview() {}
	virtual void DrawPreview(const FSceneView* View, FPrimitiveDrawInterface* PDI) {}
	virtual void DrawPreviewCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas) {}
	virtual void PostChange() {}

	virtual void PopulateToolBar(const TSharedPtr<SHorizontalBox>& ToolbarBox, const TSharedPtr<SViewportToolBar>& ParentToolBarPtr) {}
	virtual TSharedRef<SWidget> PopulateToolBarShowMenu() { return SNullWidget::NullWidget; }

	virtual bool ShowFloor() const { return true; }
	virtual bool ShowFullTransformsToolbar() const { return false; }
	virtual FRotator GetInitialViewRotation() const { return FRotator(-20.0f, 45.0f, 0.f); }
	virtual TOptional<float> GetInitialViewDistance() const { return {}; }

protected:
	static const FName DetailsTabId;
	static const FName ViewportTabId;
	
private:

	bool bPreviewQueued = false;
	TArray<FText> ErrorMessages;

	TUniquePtr<FVoxelScopedMessageConsumer> MessageScope;
	
	TSharedPtr<FVoxelSimplePreviewScene> PreviewScene;
	TSharedPtr<SVoxelSimpleAssetEditorViewport> Viewport;

	FString InitialStatsText;

protected:
	void CallUpdatePreview();
	void ClearErrorMessages();
	TUniquePtr<FVoxelScopedMessageConsumer> MakeMessageScope();
	void UpdateStatsText(const FString& Message);

	void BindToggleCommand(const TSharedPtr<FUICommandInfo>& UICommandInfo, bool& bValue);

	void SetFloorScale(const FVector& Scale) const;
};

template<typename T>
class TVoxelSimpleAssetEditorToolkit : public FVoxelSimpleAssetEditorToolkit
{
public:
	TVoxelSimpleAssetEditorToolkit()
		: FVoxelSimpleAssetEditorToolkit(T::StaticClass()->GetName())
	{
	}
	explicit TVoxelSimpleAssetEditorToolkit(const FString& BaseToolkitName)
		: FVoxelSimpleAssetEditorToolkit(BaseToolkitName)
	{
	}

	T& GetAsset() const
	{
		return GetAssetAs<T>();
	}

	static UClass* StaticClass()
	{
		return T::StaticClass();
	}
};

#define DEFINE_VOXEL_TOOLKIT(Name) \
	VOXEL_RUN_ON_STARTUP_EDITOR(Register ## Name) \
	{ \
		FVoxelBaseEditorToolkit::RegisterToolkit<Name>(Name::StaticClass()); \
	}