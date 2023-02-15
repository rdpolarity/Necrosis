// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Toolkits/SVoxelSimpleAssetEditorViewport.h"
#include "Toolkits/VoxelSimplePreviewScene.h"
#include "Toolkits/VoxelSimpleAssetEditorToolkit.h"
#include "Widgets/Input/SSlider.h"
#include "PreviewProfileController.h"
#include "SEditorViewportToolBarMenu.h"
#include "Widgets/Text/SRichTextBlock.h"

FVoxelSimpleAssetEditorViewportClient::FVoxelSimpleAssetEditorViewportClient(
	FVoxelSimplePreviewScene* PreviewScene, 
	const TWeakPtr<SVoxelSimpleAssetEditorViewport>& Viewport)
	: FEditorViewportClient(nullptr, PreviewScene, Viewport)
	, PreviewScene(*PreviewScene)
{
}

void FVoxelSimpleAssetEditorViewportClient::Tick(float DeltaSeconds)
{
	FEditorViewportClient::Tick(DeltaSeconds);

	// Tick the preview scene world.
	if (!GIntraFrameDebuggingGameThread)
	{
		PreviewScene.GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
	}
}

void FVoxelSimpleAssetEditorViewportClient::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	if (const TSharedPtr<FVoxelSimpleAssetEditorToolkit> Toolkit = WeakToolkit.Pin())
	{
		Toolkit->DrawPreview(View, PDI);
	}

	FEditorViewportClient::Draw(View, PDI);
}

void FVoxelSimpleAssetEditorViewportClient::DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas)
{
	if (const TSharedPtr<FVoxelSimpleAssetEditorToolkit> Toolkit = WeakToolkit.Pin())
	{
		Toolkit->DrawPreviewCanvas(InViewport, View, Canvas);
	}

	FEditorViewportClient::DrawCanvas(InViewport, View, Canvas);
}

#if VOXEL_ENGINE_VERSION < 501
bool FVoxelSimpleAssetEditorViewportClient::InputKey(FViewport* InViewport, int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed, bool Gamepad)
{
	bool bHandled = FEditorViewportClient::InputKey(InViewport, ControllerId, Key, Event, AmountDepressed, false);

	// Handle viewport screenshot.
	bHandled |= InputTakeScreenshot(InViewport, Key, Event);

	bHandled |= PreviewScene.HandleInputKey(InViewport, ControllerId, Key, Event, AmountDepressed, Gamepad);

	return bHandled;
}
#else
bool FVoxelSimpleAssetEditorViewportClient::InputKey(const FInputKeyEventArgs& EventArgs)
{
	bool bHandled = FEditorViewportClient::InputKey(EventArgs);

	// Handle viewport screenshot.
	bHandled |= InputTakeScreenshot(EventArgs.Viewport, EventArgs.Key, EventArgs.Event);

	bHandled |= PreviewScene.HandleInputKey(EventArgs);

	return bHandled;
}
#endif

bool FVoxelSimpleAssetEditorViewportClient::InputAxis(FViewport* InViewport, UE_501_SWITCH(int32 ControllerId, FInputDeviceId DeviceID), FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
	bool bResult = true;

	if (!bDisableInput)
	{
		bResult = PreviewScene.HandleViewportInput(InViewport, UE_501_SWITCH(ControllerId, DeviceID), Key, Delta, DeltaTime, NumSamples, bGamepad);
		if (bResult)
		{
			Invalidate();
		}
		else
		{
			bResult = FEditorViewportClient::InputAxis(InViewport, UE_501_SWITCH(ControllerId, DeviceID), Key, Delta, DeltaTime, NumSamples, bGamepad);
		}
	}

	return bResult;
}

UE::Widget::EWidgetMode FVoxelSimpleAssetEditorViewportClient::GetWidgetMode() const
{
	return UE::Widget::WM_Max;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelSimpleAssetEditorViewportClient::SetToolkit(const TWeakPtr<FVoxelSimpleAssetEditorToolkit>& Toolkit)
{
	WeakToolkit = Toolkit;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelSimpleAssetEditorViewportToolbar::Construct(const FArguments& InArgs, TSharedPtr<ICommonEditorViewportToolbarInfoProvider> InInfoProvider)
{
	WeakToolkit = InArgs._Toolkit;

	SCommonEditorViewportToolbarBase::Construct(SCommonEditorViewportToolbarBase::FArguments().PreviewProfileController(MakeShared<FPreviewProfileController>()), InInfoProvider);
}

void SVoxelSimpleAssetEditorViewportToolbar::ExtendLeftAlignedToolbarSlots(TSharedPtr<SHorizontalBox> MainBoxPtr, TSharedPtr<SViewportToolBar> ParentToolBarPtr) const
{
	if (!MainBoxPtr)
	{
		return;
	}

	const TSharedPtr<FVoxelSimpleAssetEditorToolkit> Toolkit = WeakToolkit.Pin();
	if (!Toolkit)
	{
		return;
	}

	Toolkit->PopulateToolBar(MainBoxPtr, ParentToolBarPtr);

	if (!Toolkit->ShowFullTransformsToolbar())
	{
		FSlimHorizontalToolBarBuilder ToolbarBuilder(GetInfoProvider().GetViewportWidget()->GetCommandList(), FMultiBoxCustomization::None);

		ToolbarBuilder.SetStyle(&FEditorAppStyle::Get(), "EditorViewportToolBar");
		ToolbarBuilder.SetLabelVisibility(EVisibility::Collapsed);
		ToolbarBuilder.SetIsFocusable(false);

		ToolbarBuilder.BeginSection("CameraSpeed");
		{
			const TSharedRef<SEditorViewportToolbarMenu> CameraToolbarMenu =
				SNew(SEditorViewportToolbarMenu)
				.ParentToolBar(SharedThis(VOXEL_CONST_CAST(this)))
				.AddMetaData<FTagMetaData>(FTagMetaData("CameraSpeedButton"))
				.ToolTipText(VOXEL_LOCTEXT("Camera Speed"))
				.LabelIcon(FEditorAppStyle::Get().GetBrush("EditorViewport.CamSpeedSetting"))
				.Label_Lambda([this]() -> FText
				{
					const TSharedRef<SEditorViewport> Viewport = GetInfoProvider().GetViewportWidget();
					if (!Viewport->GetViewportClient().IsValid())
					{
					   return {};
					}

					return FText::AsNumber(Viewport->GetViewportClient()->GetCameraSpeedSetting());
				})
				.OnGetMenuContent(VOXEL_CONST_CAST(this), &SVoxelSimpleAssetEditorViewportToolbar::FillCameraSpeedMenu);

			ToolbarBuilder.AddWidget(
				CameraToolbarMenu,
				STATIC_FNAME("CameraSpeed"),
				false,
				HAlign_Fill,
				FNewMenuDelegate::CreateLambda([this](FMenuBuilder& InMenuBuilder)
				{
					InMenuBuilder.AddWrapperSubMenu(
						VOXEL_LOCTEXT("Camera Speed Settings"),
						VOXEL_LOCTEXT("Adjust the camera navigation speed"),
						FOnGetContent::CreateSP(VOXEL_CONST_CAST(this), &SVoxelSimpleAssetEditorViewportToolbar::FillCameraSpeedMenu),
						FSlateIcon(UE_501_SWITCH(FEditorStyle::GetStyleSetName(), FAppStyle::GetAppStyleSetName()), "EditorViewport.CamSpeedSetting")
					);
				}
			));
		}
		ToolbarBuilder.EndSection();
		
		MainBoxPtr->AddSlot()
		.Padding(4.f, 1.f)
		.HAlign(HAlign_Right)
		[
			ToolbarBuilder.MakeWidget()
		];
	}
}

TSharedRef<SWidget> SVoxelSimpleAssetEditorViewportToolbar::GenerateShowMenu() const
{
	const TSharedPtr<FVoxelSimpleAssetEditorToolkit> Toolkit = WeakToolkit.Pin();
	if (!Toolkit)
	{
		return SNullWidget::NullWidget;
	}

	return Toolkit->PopulateToolBarShowMenu();
}

TSharedRef<SWidget> SVoxelSimpleAssetEditorViewportToolbar::FillCameraSpeedMenu()
{
	TSharedRef<SWidget> ReturnWidget = SNew(SBorder)
	.BorderImage(FEditorAppStyle::GetBrush(("Menu.Background")))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(8.f, 2.f, 60.f, 2.f))
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Text(VOXEL_LOCTEXT("Camera Speed"))
			.Font(FEditorAppStyle::GetFontStyle("MenuItem.Font"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(8.f, 4.f))
		[	
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1)
			.Padding(FMargin(0.f, 2.f))
			[
				SAssignNew(CamSpeedSlider, SSlider)
				.Value_Lambda([this]
				{
					const TSharedRef<SEditorViewport> Viewport = GetInfoProvider().GetViewportWidget();
					if (!Viewport->GetViewportClient().IsValid())
					{
						return 0.f;
					}

					return (Viewport->GetViewportClient()->GetCameraSpeedSetting() - 1.f) / (float(FEditorViewportClient::MaxCameraSpeeds) - 1.f);
				})
				.OnValueChanged_Lambda([this](float NewValue)
				{
					const TSharedRef<SEditorViewport> Viewport = GetInfoProvider().GetViewportWidget();
					if (!Viewport->GetViewportClient().IsValid())
					{
						return;
					}

					const int32 OldSpeedSetting = Viewport->GetViewportClient()->GetCameraSpeedSetting();
					const int32 NewSpeedSetting = NewValue * (float(FEditorViewportClient::MaxCameraSpeeds) - 1.f) + 1;

					if (OldSpeedSetting != NewSpeedSetting)
					{
						Viewport->GetViewportClient()->SetCameraSpeedSetting(NewSpeedSetting);
					}
				})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(8.f, 2.f, 0.f, 2.f)
			[
				SNew(STextBlock)
				.Text_Lambda([this]() -> FText
				{
					const TSharedRef<SEditorViewport> Viewport = GetInfoProvider().GetViewportWidget();
					if (!Viewport->GetViewportClient().IsValid())
					{
						return {};
					}

					return FText::AsNumber(Viewport->GetViewportClient()->GetCameraSpeedSetting());
				})
				.Font(FEditorAppStyle::GetFontStyle("MenuItem.Font"))
			]
		] // Camera Speed Scalar
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(8.f, 2.f, 60.f, 2.f))
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Text(VOXEL_LOCTEXT("Camera Speed Scalar"))
			.Font(FEditorAppStyle::GetFontStyle("MenuItem.Font"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(8.f, 4.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(FMargin(0.f, 2.f))
			[
				SAssignNew(CamSpeedScalarBox, SSpinBox<float>)
				.MinValue(1.f)
				.MaxValue(TNumericLimits<int32>::Max())
				.MinSliderValue(1.f)
				.MaxSliderValue(128.f)
				.Value_Lambda([this]
				{
					const TSharedRef<SEditorViewport> Viewport = GetInfoProvider().GetViewportWidget();
					if (!Viewport->GetViewportClient().IsValid())
					{
						return 1.f;
					}

					return Viewport->GetViewportClient()->GetCameraSpeedScalar();
				})
				.OnValueChanged_Lambda([this](float NewValue)
				{
					const TSharedRef<SEditorViewport> Viewport = GetInfoProvider().GetViewportWidget();
					if (!Viewport->GetViewportClient().IsValid())
					{
						return;
					}

					Viewport->GetViewportClient()->SetCameraSpeedScalar(NewValue);
				})
				.ToolTipText(VOXEL_LOCTEXT("Scalar to increase camera movement range"))
			]
		]
	];

	return ReturnWidget;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelSimpleAssetEditorViewport::Construct(const FArguments& Args)
{
	PreviewScene = Args._PreviewScene;
	InitialViewRotation = Args._InitialViewRotation;
	InitialViewDistance = Args._InitialViewDistance;
	WeakToolkit = Args._Toolkit;
	if (const TSharedPtr<FVoxelSimpleAssetEditorToolkit> Toolkit = WeakToolkit.Pin())
	{
		bShowFullTransformsToolbar = Toolkit->ShowFullTransformsToolbar();
	}

	SEditorViewport::Construct(SEditorViewport::FArguments());

	ViewportOverlay->AddSlot(0)
	.Padding(10, 0)
	.VAlign(VAlign_Top)
	.HAlign(HAlign_Left)
	[
		SNew(STextBlock)
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 13))
		.Text(Args._ErrorMessage)
		.ColorAndOpacity(FLinearColor::Red)
	];
}

void SVoxelSimpleAssetEditorViewport::UpdateStatsText(const FString& NewText)
{
	bStatsVisible = true;
	OverlayText->SetText(FText::FromString(NewText));
}

void SVoxelSimpleAssetEditorViewport::OnFocusViewportToSelection()
{
	if (PreviewScene.IsValid())
	{
		GetViewportClient()->FocusViewportOnBox(PreviewScene->GetComponentsBounds());
	}
}

TSharedRef<FEditorViewportClient> SVoxelSimpleAssetEditorViewport::MakeEditorViewportClient()
{
	const FBox Bounds = PreviewScene->GetComponentsBounds();

	const TSharedRef<FVoxelSimpleAssetEditorViewportClient> ViewportClient = MakeShared<FVoxelSimpleAssetEditorViewportClient>(PreviewScene.Get(), SharedThis(this));
	ViewportClient->SetRealtime(true);
	ViewportClient->SetViewRotation(InitialViewRotation);
	ViewportClient->SetViewLocationForOrbiting(Bounds.GetCenter(), InitialViewDistance.Get(Bounds.GetExtent().GetMax() * 2));
	ViewportClient->SetToolkit(WeakToolkit);

	return ViewportClient;
}

TSharedPtr<SWidget> SVoxelSimpleAssetEditorViewport::MakeViewportToolbar()
{
	return
		SNew(SVoxelSimpleAssetEditorViewportToolbar, SharedThis(this))
		.Toolkit(WeakToolkit);
}

void SVoxelSimpleAssetEditorViewport::PopulateViewportOverlays(TSharedRef<SOverlay> Overlay)
{
	SEditorViewport::PopulateViewportOverlays(Overlay);

	Overlay->AddSlot()
	.VAlign(VAlign_Top)
	.HAlign(HAlign_Left)
	.Padding(FMargin(6.f, 36.f, 6.f, 6.f))
	[
		SNew(SBorder)
		.Visibility_Lambda([this]
		{
			return bStatsVisible ? EVisibility::Visible : EVisibility::Collapsed;
		})
		.BorderImage(FEditorAppStyle::Get().GetBrush("FloatingBorder"))
		.Padding(4.f)
		[
			SAssignNew(OverlayText, SRichTextBlock)
		]
	];
}

EVisibility SVoxelSimpleAssetEditorViewport::GetTransformToolbarVisibility() const
{
	if (bShowFullTransformsToolbar)
	{
		return SEditorViewport::GetTransformToolbarVisibility();
	}

	return EVisibility::Collapsed;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SEditorViewport> SVoxelSimpleAssetEditorViewport::GetViewportWidget()
{
	return SharedThis(this);
}

TSharedPtr<FExtender> SVoxelSimpleAssetEditorViewport::GetExtenders() const
{
	TSharedPtr<FExtender> Result(MakeShared<FExtender>());
	return Result;
}

void SVoxelSimpleAssetEditorViewport::OnFloatingButtonClicked()
{
}
