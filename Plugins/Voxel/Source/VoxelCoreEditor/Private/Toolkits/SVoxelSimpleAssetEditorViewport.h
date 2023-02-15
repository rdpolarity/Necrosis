// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "SEditorViewport.h"
#include "Widgets/Input/SSpinBox.h"
#include "SCommonEditorViewportToolbarBase.h"

class SSlider;
class SRichTextBlock;
class FVoxelSimplePreviewScene;
class FVoxelSimpleAssetEditorToolkit;
class SVoxelSimpleAssetEditorViewport;

class FVoxelSimpleAssetEditorViewportClient : public FEditorViewportClient
{
public:
	FVoxelSimplePreviewScene& PreviewScene;
	
	FVoxelSimpleAssetEditorViewportClient(
		FVoxelSimplePreviewScene* PreviewScene, 
		const TWeakPtr<SVoxelSimpleAssetEditorViewport>& Viewport);

	// TODO Shared voxel base
	//~ Begin FEditorViewportClient Interface
	virtual void Tick(float DeltaSeconds) override;
	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual void DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas) override;
#if VOXEL_ENGINE_VERSION < 501
	virtual bool InputKey(FViewport* Viewport, int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed, bool bGamepad) override;
#else
	virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;
#endif
	virtual bool InputAxis(FViewport* Viewport, UE_501_SWITCH(int32 ControllerId, FInputDeviceId DeviceID), FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad) override;
	virtual UE::Widget::EWidgetMode GetWidgetMode() const override;
	//~ End FEditorViewportClient Interface

	void SetToolkit(const TWeakPtr<FVoxelSimpleAssetEditorToolkit>& Toolkit);

private:
	TWeakPtr<FVoxelSimpleAssetEditorToolkit> WeakToolkit;
};


class SVoxelSimpleAssetEditorViewportToolbar : public SCommonEditorViewportToolbarBase
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_ARGUMENT(TWeakPtr<FVoxelSimpleAssetEditorToolkit>, Toolkit)
	};

	void Construct(const FArguments& InArgs, TSharedPtr<class ICommonEditorViewportToolbarInfoProvider> InInfoProvider);

private:
	virtual void ExtendLeftAlignedToolbarSlots(TSharedPtr<SHorizontalBox> MainBoxPtr, TSharedPtr<SViewportToolBar> ParentToolBarPtr) const override;
	virtual TSharedRef<SWidget> GenerateShowMenu() const override;

private:
	TSharedRef<SWidget> FillCameraSpeedMenu();

private:
	TWeakPtr<FVoxelSimpleAssetEditorToolkit> WeakToolkit;

	TSharedPtr<SSlider> CamSpeedSlider;
	mutable TSharedPtr<SSpinBox<float>> CamSpeedScalarBox;
};

class SVoxelSimpleAssetEditorViewport
	: public SEditorViewport
	, public ICommonEditorViewportToolbarInfoProvider
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_ARGUMENT(TSharedPtr<FVoxelSimplePreviewScene>, PreviewScene)
		SLATE_ARGUMENT(FRotator, InitialViewRotation)
		SLATE_ARGUMENT(TOptional<float>, InitialViewDistance)
		SLATE_ATTRIBUTE(FText, ErrorMessage)
		SLATE_ARGUMENT(TWeakPtr<FVoxelSimpleAssetEditorToolkit>, Toolkit)
	};

	void Construct(const FArguments& Args);
	void UpdateStatsText(const FString& NewText);
	
protected:
	//~ Begin SEditorViewport Interface
	virtual void OnFocusViewportToSelection() override;
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual TSharedPtr<SWidget> MakeViewportToolbar() override;
	virtual void PopulateViewportOverlays(TSharedRef<SOverlay> Overlay) override;
	virtual EVisibility GetTransformToolbarVisibility() const override;
	//~ End SEditorViewport Interface
	
	//~ Begin ICommonEditorViewportToolbarInfoProvider Interface
	virtual TSharedRef<class SEditorViewport> GetViewportWidget() override;
	virtual TSharedPtr<FExtender> GetExtenders() const override;
	virtual void OnFloatingButtonClicked() override;
	//~ End ICommonEditorViewportToolbarInfoProvider Interface

private:
	FRotator InitialViewRotation;
	TOptional<float> InitialViewDistance;
	TSharedPtr<FVoxelSimplePreviewScene> PreviewScene;
	TSharedPtr<SRichTextBlock> OverlayText;
	TWeakPtr<FVoxelSimpleAssetEditorToolkit> WeakToolkit;

	bool bStatsVisible = false;
	bool bShowFullTransformsToolbar = false;
};