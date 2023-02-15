// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "SVoxelMetaGraphPreview.h"
#include "VoxelActor.h"
#include "VoxelMetaGraph.h"
#include "VoxelMetaGraphPreviewActor.h"
#include "Nodes/VoxelPositionNodes.h"
#include "Nodes/VoxelExposeDataNode.h"
#include "Widgets/SVoxelMetaGraphPreviewImage.h"
#include "Widgets/SVoxelMetaGraphDepthSlider.h"
#include "Widgets/SVoxelMetaGraphPreviewStats.h"
#include "Widgets/SVoxelMetaGraphPreviewRuler.h"
#include "Widgets/SVoxelMetaGraphPreviewScale.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

void SPreview::Construct(const FArguments& Args)
{
	MetaGraph = Args._MetaGraph;
	check(MetaGraph.IsValid());

	FSlimHorizontalToolBarBuilder LeftToolbarBuilder(nullptr, FMultiBoxCustomization::None);
	LeftToolbarBuilder.SetStyle(&FEditorAppStyle::Get(), "EditorViewportToolBar");
	LeftToolbarBuilder.SetLabelVisibility(EVisibility::Collapsed);

	LeftToolbarBuilder.BeginSection("Preview");
	{
		LeftToolbarBuilder.BeginBlockGroup();

		const auto AddAxisButton = [&](EVoxelAxis Axis)
		{
			FSlateIcon Icon;
			switch (Axis)
			{
			default: ensure(false);
			case EVoxelAxis::X: Icon = FSlateIcon(UE_501_SWITCH(FEditorStyle::GetStyleSetName(), FAppStyle::GetAppStyleSetName()), "StaticMeshEditor.ToggleShowTangents"); break;
			case EVoxelAxis::Y: Icon = FSlateIcon(UE_501_SWITCH(FEditorStyle::GetStyleSetName(), FAppStyle::GetAppStyleSetName()), "StaticMeshEditor.SetShowBinormals"); break;
			case EVoxelAxis::Z: Icon = FSlateIcon(UE_501_SWITCH(FEditorStyle::GetStyleSetName(), FAppStyle::GetAppStyleSetName()), "StaticMeshEditor.SetShowNormals"); break;
			}
			LeftToolbarBuilder.AddToolBarButton(FUIAction(
				MakeLambdaDelegate([=]
				{
					if (!PreviewActor.IsValid())
					{
						return;
					}
					
					const FVoxelTransaction Transaction(PreviewActor, "Set preview axis");

					PreviewActor->Axis = Axis;

					DepthSlider->ResetValue(PreviewActor->GetAxisLocation(), false);
				}),
				MakeLambdaDelegate([=]
				{
					return PreviewActor.IsValid();
				}),
				MakeLambdaDelegate([=]
				{
					return PreviewActor.IsValid() && PreviewActor->Axis == Axis;
				})),
				{},
				UEnum::GetDisplayValueAsText(Axis),
				UEnum::GetDisplayValueAsText(Axis),
				Icon,
				EUserInterfaceActionType::ToggleButton);
		};

		AddAxisButton(EVoxelAxis::X);
		AddAxisButton(EVoxelAxis::Y);
		AddAxisButton(EVoxelAxis::Z);

		LeftToolbarBuilder.EndBlockGroup();
	}

	LeftToolbarBuilder.AddSeparator();

	LeftToolbarBuilder.AddToolBarButton(FUIAction(
		MakeLambdaDelegate([=]
		{
			if (!PreviewActor.IsValid())
			{
				return;
			}
			
			const FVoxelTransaction Transaction(PreviewActor, "Reset view");

			PreviewActor->SetActorLocation(FVector::ZeroVector);
			PreviewActor->SetActorScale3D(FVector::OneVector);

			DepthSlider->ResetValue(PreviewActor->GetAxisLocation(), false);
		}),
		MakeLambdaDelegate([=]
		{
			if (!PreviewActor.IsValid())
			{
				return false;
			}

			return
				!PreviewActor->GetActorLocation().IsNearlyZero() ||
				!PreviewActor->GetActorScale().Equals(FVector::One());
		})),
		{},
		VOXEL_LOCTEXT("Reset view"),
		VOXEL_LOCTEXT("Reset view"),
		FSlateIcon(UE_501_SWITCH(FEditorStyle::GetStyleSetName(), FAppStyle::GetAppStyleSetName()), "GenericCommands.Undo"),
		EUserInterfaceActionType::Button);

	LeftToolbarBuilder.EndSection();

	LeftToolbarBuilder.AddSeparator();

	LeftToolbarBuilder.AddToolBarButton(FUIAction(
		MakeLambdaDelegate([=]
		{
			if (!PreviewActor.IsValid())
			{
				return;
			}

			bNormalizePreviewColors = !bNormalizePreviewColors;
			QueueUpdate();
		}),
		MakeLambdaDelegate([=]
		{
			return PreviewActor.IsValid();
		}),
		MakeLambdaDelegate([=]
		{
			return bNormalizePreviewColors;
		})),
		{},
		VOXEL_LOCTEXT("Normalize preview color values"),
		VOXEL_LOCTEXT("Normalize preview color values"),
		FSlateIcon(UE_501_SWITCH(FEditorStyle::GetStyleSetName(), FAppStyle::GetAppStyleSetName()), "CurveEd.FitHorizontal"),
		EUserInterfaceActionType::ToggleButton);

	LeftToolbarBuilder.EndSection();

	const TSharedRef<SWidget> ResolutionSelector = 
		SNew(SComboButton)
		.ButtonStyle(FEditorAppStyle::Get(), "EditorViewportToolBar.Button")
		.ComboButtonStyle(FEditorAppStyle::Get(), "ViewportPinnedCommandList.ComboButton")
		.ContentPadding(FMargin(4.f, 0.f))
		.ToolTipText(VOXEL_LOCTEXT("Texture resolution for preview. The higher the resolution, the longer it will take to compute preview"))
		.OnGetMenuContent_Lambda([this]
		{
			FMenuBuilder MenuBuilder(true, nullptr);

			MenuBuilder.BeginSection({}, VOXEL_LOCTEXT("Resolutions"));

			static const TArray<int32> PreviewSizes =
			{
				256,
				512,
				1024,
				2048,
				4096
			};

			for (const int32 NewPreviewSize : PreviewSizes)
			{
				const FString Text = FString::Printf(TEXT("%dx%d"), NewPreviewSize, NewPreviewSize);

				MenuBuilder.AddMenuEntry(
					FText::FromString(Text),
					FText::FromString(Text),
					FSlateIcon(),
					FUIAction(
						MakeLambdaDelegate([=]
						{
							const FVoxelTransaction Transaction(MetaGraph, "Change preview size");
							MetaGraph->PreviewSize = NewPreviewSize;

							QueueUpdate();
						}),
						MakeLambdaDelegate([]
						{
							return true;
						}),
						MakeLambdaDelegate([=]
						{
							return MetaGraph->PreviewSize == NewPreviewSize;
						})
					),
					{},
					EUserInterfaceActionType::Check);
			}

			MenuBuilder.EndSection();
			
			return MenuBuilder.MakeWidget();
		})
		.ButtonContent()
		[
			SNew(SHorizontalBox)
			.Clipping(EWidgetClipping::ClipToBounds)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SImage)
				.Image(FEditorAppStyle::GetBrush("CurveEditorTools.ActivateTransformTool"))
				.DesiredSizeOverride(FVector2D(16.f, 16.f))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.Padding(4.f, 0.f)
			.AutoWidth()
			[
				SNew(SVoxelDetailText)
				.Text_Lambda([this]
				{
					return FText::FromString(FString::Printf(TEXT("%dx%d"), MetaGraph->PreviewSize, MetaGraph->PreviewSize));
				})
				.Clipping(EWidgetClipping::ClipToBounds)
			]
		];

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SAssignNew(PreviewScaleBox, SScaleBox)
				.Stretch(EStretch::ScaleToFit)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SAssignNew(PreviewImage, SPreviewImage)
						.Width_Lambda([this]() -> float
						{
							return MetaGraph->PreviewSize;
						})
						.Height_Lambda([this]() -> float
						{
							return MetaGraph->PreviewSize;
						})
						.Cursor(EMouseCursor::Crosshairs)
					]
					+ SOverlay::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Bottom)
					[
						SNew(SBox)
						.Visibility(EVisibility::HitTestInvisible)
						[
							SAssignNew(PreviewScale, SPreviewScale)
							.SizeWidget_Lambda([=]
							{
								return PreviewScaleBox;
							})
							.Resolution_Lambda([=]
							{
								return MetaGraph->PreviewSize;
							})
							.Value_Lambda([=]
							{
								return GetPixelToWorld(MetaGraph->PreviewSize).GetScaleVector().X;
							})
						]
					]
					+ SOverlay::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						SNew(SBox)
						.Visibility(EVisibility::HitTestInvisible)
						[
							SAssignNew(PreviewRuler, SPreviewRuler)
							.SizeWidget_Lambda([=]
							{
								return PreviewScaleBox;
							})
							.Resolution_Lambda([=]
							{
								return MetaGraph->PreviewSize;
							})
							.Value_Lambda([=]
							{
								return GetPixelToWorld(MetaGraph->PreviewSize).GetScaleVector().X;
							})
						]
					]
					+ SOverlay::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						SNew(SBorder)
						.BorderImage(FEditorAppStyle::GetBrush("WhiteBrush"))
						.BorderBackgroundColor(FLinearColor(.0f, .0f, .0f, .3f))
						.Visibility_Lambda([=]
						{
							return
								bUpdateProcessing &&
								FPlatformTime::Seconds() - ProcessingStartTime > 1.
								? EVisibility::Visible
								: EVisibility::Hidden;
							})
						[
							SNew(SScaleBox)
							.IgnoreInheritedScale(true)
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								SNew(SCircularThrobber)
							]
						]
					]
				]
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SBox)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				.Visibility(EVisibility::HitTestInvisible)
				.Padding(FMargin(20.f, 0.f))
				[
					SAssignNew(PreviewMessage, STextBlock)
					.Font(FVoxelEditorUtilities::Font())
					.Visibility(EVisibility::Collapsed)
					.ColorAndOpacity(FLinearColor::White)
					.ShadowOffset(FVector2D(1.f))
					.ShadowColorAndOpacity(FLinearColor::Black)
					.AutoWrapText(true)
					.Justification(ETextJustify::Center)
				]
			]
			+ SOverlay::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(5)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.Padding(20.f, 0.f)
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Top)
					[
						LeftToolbarBuilder.MakeWidget()
					]
					+ SHorizontalBox::Slot()
					.Padding(20.f, 0.f)
					.HAlign(HAlign_Right)
					[
						ResolutionSelector
					]
				]
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FEditorAppStyle::Get().GetBrush("Brushes.Recessed"))
			.Padding(4.0f)
			[
				SNew(SBox)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Fill)
				.WidthOverride(150.f)
				[
					SAssignNew(DepthSlider, SDepthSlider)
					.ValueText(VOXEL_LOCTEXT("Depth"))
					.ToolTipText(VOXEL_LOCTEXT("Depth along the axis being previewed"))
					.Value((!PreviewActor.IsValid() ? 0.f : PreviewActor->GetAxisLocation()))
					.MinValue((!PreviewActor.IsValid() ? 0.f : PreviewActor->GetAxisLocation()) - 100.f)
					.MaxValue((!PreviewActor.IsValid() ? 0.f : PreviewActor->GetAxisLocation()) + 100.f)
					.OnValueChanged_Lambda([&](float NewValue)
					{
						if (!PreviewActor.IsValid())
						{
							return;
						}

						const FVoxelTransaction Transaction(PreviewActor, "Change depth");

						PreviewActor->SetAxisLocation(NewValue);

						QueueUpdate();
					})
				]
			]
		]
	];
}

TSharedPtr<SPreviewStats> SPreview::ConstructStats() const
{
	return
		SNew(SPreviewStats)
		+ SPreviewStats::Row()
		.Header(VOXEL_LOCTEXT("Location"))
		.Tooltip(VOXEL_LOCTEXT("Currently previewed location"))
		.Value_Lambda([this]
		{
			return FText::FromString(CurrentLocation);
		})
		+ SPreviewStats::Row()
		.Header(VOXEL_LOCTEXT("Value"))
		.Tooltip(VOXEL_LOCTEXT("Value in current location"))
		.Value_Lambda([this]
		{
			return FText::FromString(CurrentValue);
		})
		+ SPreviewStats::Row()
		.Header(VOXEL_LOCTEXT("Min value"))
		.Tooltip(VOXEL_LOCTEXT("Min value found in preview"))
		.Value_Lambda([this]
		{
			return FText::FromString(MinValue);
		})
		+ SPreviewStats::Row()
		.Header(VOXEL_LOCTEXT("Max value"))
		.Tooltip(VOXEL_LOCTEXT("Max value found in preview"))
		.Value_Lambda([this]
		{
			return FText::FromString(MaxValue);
		});
}

void SPreview::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (bUpdateQueued)
	{
		bUpdateQueued = false;
		Update();
	}
}

FReply SPreview::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		bLockCoordinate = true;
		return FReply::Handled();
	}

	const bool bIsMiddleMouseButtonDown = MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton);
	if (!bIsMiddleMouseButtonDown ||
		!PreviewActor.IsValid())
	{
		return FReply::Unhandled();
	}

	PreviewRuler->StartRuler(MouseEvent.GetScreenSpacePosition(), PreviewImage->GetCachedGeometry().AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()));

	return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
}

FReply SPreview::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (bLockCoordinate)
		{
			const AVoxelActor* VoxelActor = GetVoxelActor();
			if (!VoxelActor)
			{
				PreviewImage->ClearLockedPosition();
				bIsCoordinateLocked = false;

				return FReply::Handled();
			}

			const FMatrix PixelToWorld = GetPixelToWorld(MetaGraph->PreviewSize);
			const FMatrix LocalToWorld = VoxelActor->ActorToWorld().ToMatrixWithScale();
			const FMatrix WorldToLocal = LocalToWorld.Inverse();

			const FMatrix LocalToPixel = LocalToWorld * PixelToWorld.Inverse();

			FVector2D MouseImagePosition = PreviewImage->GetCachedGeometry().AbsoluteToLocal(CurrentMousePosition);
			MouseImagePosition.Y = PreviewImage->GetCachedGeometry().Size.Y - MouseImagePosition.Y;

			if (bIsCoordinateLocked)
			{
				const FVector2D OldPixelPosition = FVector2D(LocalToPixel.TransformPosition(LockedCoordinate));
				if (FVector2D::Distance(OldPixelPosition, MouseImagePosition) < 20.f)
				{
					PreviewImage->ClearLockedPosition();

					bIsCoordinateLocked = false;
					return FReply::Handled();
				}
			}

			const FMatrix PixelToLocal = PixelToWorld * WorldToLocal;
			LockedCoordinate = FVector(PixelToLocal.TransformPosition(FVector(MouseImagePosition.X, MouseImagePosition.Y, 0.f)));
			bIsCoordinateLocked = true;

			PreviewImage->SetLockedPosition(LockedCoordinate);

			if (!PreviewImage->UpdateLockedPosition(LocalToPixel, PreviewImage->GetCachedGeometry().Size))
			{
				bIsCoordinateLocked = false;
			}
		}
		return FReply::Handled();
	}

	const bool bIsMiddleMouseButtonDown = MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton);
	if (bIsMiddleMouseButtonDown ||
		!PreviewActor.IsValid())
	{
		return FReply::Unhandled();
	}

	PreviewRuler->StopRuler();

	return SCompoundWidget::OnMouseButtonUp(MyGeometry, MouseEvent);
}

FReply SPreview::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const bool bIsMouseButtonDown =
		MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) ||
		MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton);
	const bool bIsMiddleMouseButtonDown = MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton);

	CurrentMousePosition = MouseEvent.GetScreenSpacePosition();
	
	if ((!bIsMouseButtonDown && !bIsMiddleMouseButtonDown) ||
		!PreviewActor.IsValid())
	{
		return FReply::Unhandled();
	}

	if (bIsMiddleMouseButtonDown)
	{
		if (!MouseEvent.GetCursorDelta().IsNearlyZero())
		{
			PreviewRuler->UpdateRuler(CurrentMousePosition, PreviewImage->GetCachedGeometry().AbsoluteToLocal(CurrentMousePosition));
		}
		return FReply::Handled();
	}

	const FVector2D PixelDelta = TransformVector(Inverse(PreviewImage->GetCachedGeometry().GetAccumulatedRenderTransform()), MouseEvent.GetCursorDelta());
	const FVector WorldDelta = GetPixelToWorld(MetaGraph->PreviewSize).TransformVector(FVector(-PixelDelta.X, PixelDelta.Y, 0));

	if (WorldDelta.IsNearlyZero())
	{
		return FReply::Handled();
	}

	bLockCoordinate = false;

	PreviewActor->SetActorLocation(PreviewActor->GetActorLocation() + WorldDelta);
	DepthSlider->ResetValue(PreviewActor->GetAxisLocation(), true);
	QueueUpdate();

	return FReply::Handled();
}

FReply SPreview::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const float Delta = MouseEvent.GetWheelDelta();
	if (FMath::IsNearlyZero(Delta) ||
		!PreviewActor.IsValid())
	{
		return FReply::Handled();
	}

	constexpr float MinZoom = 0.001f;

	const FVector NewScale = PreviewActor->GetActorScale3D() * (1.f - FMath::Clamp(Delta, -0.5f, 0.5f));
	PreviewActor->SetActorScale3D(FVoxelUtilities::ComponentMin(FVector(1e6), FVoxelUtilities::ComponentMax(FVector(MinZoom), NewScale)));
	QueueUpdate();

	return FReply::Handled();
}

void SPreview::Update()
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(MetaGraph.IsValid()))
	{
		UpdateMessage("");
		return;
	}

	const AVoxelActor* VoxelActor = GetVoxelActor();
	if (!VoxelActor)
	{
		if (MetaGraph->bIsMacroGraph)
		{
			UpdateMessage("Preview does not work for macros");
		}
		else
		{
			UpdateMessage("Graph must be used in Voxel Actor to preview it");
		}
		return;
	}

	TArray<AVoxelMetaGraphPreviewActor*> PreviewActors;
	for (AVoxelMetaGraphPreviewActor* Actor : TActorRange<AVoxelMetaGraphPreviewActor>(GWorld))
	{
		PreviewActors.Add(Actor);
	}

	if (PreviewActors.Num() == 0)
	{
		PreviewActors.Add(GWorld->SpawnActor<AVoxelMetaGraphPreviewActor>());
	}
	if (PreviewActors.Num() > 1)
	{
		VOXEL_MESSAGE(Warning, "More than one meta graph preview actor: {0}", PreviewActors);
	}

	PreviewActor = PreviewActors[0];
	if (!ensure(PreviewActor.IsValid()))
	{
		UpdateMessage("Preview actor is not spawned, make a change to graph to spawn it");
		return;
	}

	PreviewActor->OnChanged.RemoveAll(this);
	PreviewActor->OnChanged.Add(MakeWeakPtrDelegate(this, [=]
	{
		bUpdateQueued = true;
	}));

	MetaGraph->OnParametersChanged.RemoveAll(this);
	MetaGraph->OnParametersChanged.Add(MakeWeakPtrDelegate(this, [=]
	{
		bUpdateQueued = true;
	}));
	
	const TSharedRef<const FVoxelExposedDataSubsystem> ExposedDataSubsystem = VoxelActor->GetRuntime()->GetSubsystem<FVoxelExposedDataSubsystem>().AsShared();

	const EVoxelAxis PreviewAxis = PreviewActor->Axis;
	const int32 PreviewSize = MetaGraph->PreviewSize;

	const FMatrix PixelToWorld = GetPixelToWorld(PreviewSize);
	const FMatrix LocalToWorld = VoxelActor->ActorToWorld().ToMatrixWithScale();
	const FMatrix WorldToLocal = LocalToWorld.Inverse();
	const FMatrix PixelToLocal = PixelToWorld * WorldToLocal;
	const FMatrix LocalToPixel = LocalToWorld * PixelToWorld.Inverse();

	if (bUpdateProcessing)
	{
		bUpdateQueued = true;
		return;
	}

	bUpdateProcessing = true;
	ProcessingStartTime = FPlatformTime::Seconds();

	AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, MakeWeakPtrLambda(this, [=]
	{
		const FVoxelPinType Type = ExposedDataSubsystem->GetType("Internal_Preview");

		const FVoxelMetaGraphPreviewFactory* Factory = FVoxelMetaGraphPreviewFactory::FindFactory(Type);
		if (!Factory)
		{
			AsyncTask(ENamedThreads::GameThread, MakeWeakPtrLambda(this, [=]
			{
				UpdateMessage("Pin type not supported for preview");
				PreviewImage->SetContent(SNullWidget::NullWidget);
				Preview.Reset();
			
				ensure(bUpdateProcessing);
				bUpdateProcessing = false;	
			}));
			return;
		}

		FVoxelMetaGraphPreviewFactory::FParameters Parameters;
		Parameters.PreviewSize = PreviewSize;
		Parameters.PixelToLocal = PixelToLocal;
		Parameters.bNormalize = bNormalizePreviewColors;
		Parameters.ExecuteQuery = [=](const FVoxelQuery& Query)
		{
			FVoxelPinType ExpectedType = Type;
			if (Type.IsBuffer())
			{
				ExpectedType = ExpectedType.GetViewType();
			}
			ExpectedType = ExpectedType.WithoutTag();

			const FVoxelFutureValue Value = ExposedDataSubsystem->GetData("Internal_Preview", Query);
			if (!Value.IsValid())
			{
				return FVoxelFutureValue(FVoxelSharedPinValue(ExpectedType));
			}

			return FVoxelTask::New(
				MakeShared<FVoxelTaskStat>(),
				ExpectedType,
				"CheckValue",
				EVoxelTaskThread::AnyThread,
				{ Value },
				[=]
				{
					if (Value.Get_CheckCompleted().GetType() != ExpectedType)
					{
						return FVoxelFutureValue(FVoxelSharedPinValue(ExpectedType));
					}

					return Value;
				});
		};
		Parameters.PreviousPreview = Preview;
		Parameters.Finalize = [=](const TSharedRef<FVoxelMetaGraphPreview>& NewPreview)
		{
			AsyncTask(ENamedThreads::GameThread, MakeWeakPtrLambda(this, [=]
			{
				const TSharedPtr<SWidget> Widget = NewPreview->MakeWidget();
				UpdateMessage("");
				PreviewImage->SetContent(Widget ? Widget.ToSharedRef() : SNullWidget::NullWidget);
				if (!PreviewImage->UpdateLockedPosition(LocalToPixel, PreviewImage->GetCachedGeometry().Size))
				{
					bIsCoordinateLocked = false;
				}

				Widget->SetOnMouseMove(FPointerEventHandler::CreateLambda([=](const FGeometry&, const FPointerEvent) -> FReply
				{
					if (bIsCoordinateLocked)
					{
						const FVector2D MouseImagePosition = FVector2D(LocalToPixel.TransformPosition(LockedCoordinate));

						CurrentValue = NewPreview->GetValue(MouseImagePosition);
						CurrentLocation = FVector(PixelToLocal.TransformPosition(FVector(MouseImagePosition.X, MouseImagePosition.Y, 0.f))).ToString();
					}
					else
					{
						FVector2D MouseImagePosition = PreviewImage->GetCachedGeometry().AbsoluteToLocal(CurrentMousePosition);
						MouseImagePosition.Y = PreviewImage->GetCachedGeometry().Size.Y - MouseImagePosition.Y;

						CurrentValue = NewPreview->GetValue(MouseImagePosition);
						CurrentLocation = FVector(PixelToLocal.TransformPosition(FVector(MouseImagePosition.X, MouseImagePosition.Y, 0.f))).ToString();
					}
					return FReply::Unhandled();
				}));
	
				MinValue = NewPreview->GetMinValue();
				MaxValue = NewPreview->GetMaxValue();
	
				PreviewScale->Enable();

				Preview = NewPreview;

				ensure(bUpdateProcessing);
				bUpdateProcessing = false;
			}));
		};

		Factory->Create(Parameters);
	}));
}

FMatrix SPreview::GetPixelToWorld(int32 InPreviewSize) const
{
	if (!PreviewActor.IsValid())
	{
		return FMatrix::Identity;
	}

	return
		FScaleMatrix(1.f / InPreviewSize) *
		FScaleMatrix(2.f) *
		FTranslationMatrix(-FVector::OneVector) *
		FScaleMatrix(PreviewActor->BoxComponent->GetUnscaledBoxExtent()) *
		PreviewActor->ActorToWorld().ToMatrixWithScale();
}

AVoxelActor* SPreview::GetVoxelActor()
{
	TArray<AVoxelActor*> VoxelActors;
	for (AVoxelActor* VoxelActor : TActorRange<AVoxelActor>(GWorld))
	{
		if (!VoxelActor->GetRuntime() ||
			VoxelActor->MetaGraph != MetaGraph.Get())
		{
			continue;
		}

		VoxelActors.Add(VoxelActor);
	}
	
	if (VoxelActors.IsEmpty())
	{
		return nullptr;
	}
	
	if (VoxelActors.Num() > 1)
	{
		VOXEL_MESSAGE(Warning,
			"More than one voxel actor using {0} as meta graph: {1}\n"
			"Only the first one will be used for preview",
			MetaGraph,
			VoxelActors);
	}

	return VoxelActors[0];
}

void SPreview::UpdateMessage(const FString& Message) const
{
	if (Message.IsEmpty())
	{
		PreviewMessage->SetVisibility(EVisibility::Collapsed);
		return;
	}

	PreviewMessage->SetVisibility(EVisibility::HitTestInvisible);
	PreviewMessage->SetText(FText::FromString(Message));
}

END_VOXEL_NAMESPACE(MetaGraph)