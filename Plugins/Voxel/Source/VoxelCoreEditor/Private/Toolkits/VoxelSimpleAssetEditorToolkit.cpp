// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Toolkits/VoxelSimpleAssetEditorToolkit.h"
#include "Toolkits/SVoxelSimpleAssetEditorViewport.h"

const FName FVoxelSimpleAssetEditorToolkit::DetailsTabId(TEXT("VoxelSimpleAssetEditor_Details"));
const FName FVoxelSimpleAssetEditorToolkit::ViewportTabId(TEXT("VoxelSimpleAssetEditor_Viewport"));

void FVoxelSimpleAssetEditorToolkit::CreateInternalWidgets()
{
	FVoxelBaseEditorToolkit::CreateInternalWidgets();
	
	PreviewScene = MakeShared<FVoxelSimplePreviewScene>(FPreviewScene::ConstructionValues());
	PreviewScene->SetFloorVisibility(ShowFloor(), true);

	SetupPreview();
	CallUpdatePreview();

	// Make sure to make the viewport after UpdatePreview so that the component bounds are correct
	Viewport = SNew(SVoxelSimpleAssetEditorViewport)
		.PreviewScene(PreviewScene)
		.InitialViewRotation(GetInitialViewRotation())
		.InitialViewDistance(GetInitialViewDistance())
		.ErrorMessage_Lambda([this]
		{
			return FText::Join(INVTEXT("\n"), ErrorMessages);
		})
		.Toolkit(SharedThis(this));

	if (!InitialStatsText.IsEmpty())
	{
		Viewport->UpdateStatsText(InitialStatsText);
		InitialStatsText = {};
	}
}

TSharedRef<FTabManager::FLayout> FVoxelSimpleAssetEditorToolkit::GetLayout() const
{
	return FTabManager::NewLayout("Standalone_VoxelSimpleAssetEditor_Layout_v0")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Horizontal)
				->SetSizeCoefficient(0.9f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.3f)
					->AddTab(DetailsTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.7f)
					->AddTab(ViewportTabId, ETabState::OpenedTab)
				)
			)
		);
}

void FVoxelSimpleAssetEditorToolkit::RegisterTabs(FRegisterTab RegisterTab)
{
	RegisterTab(DetailsTabId, VOXEL_LOCTEXT("Details"), "LevelEditor.Tabs.Details", GetDetailsView());
	RegisterTab(ViewportTabId, VOXEL_LOCTEXT("Viewport"), "LevelEditor.Tabs.Viewports", Viewport);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelSimpleAssetEditorToolkit::PostRegenerateMenusAndToolbars()
{
	const TSharedRef<SHorizontalBox> MenuOverlayBox = SNew(SHorizontalBox)
	+ SHorizontalBox::Slot()
	.AutoWidth()
	.VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		.ShadowOffset(FVector2D::UnitVector)
		.Text(VOXEL_LOCTEXT("Class: "))
	]
	+SHorizontalBox::Slot()
	.AutoWidth()
	.VAlign(VAlign_Center)
	[
		SNew(SSpacer)
		.Size(FVector2D(2.0f,1.0f))
	]
	+SHorizontalBox::Slot()
	.AutoWidth()
	.VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.ShadowOffset(FVector2D::UnitVector)
		.Text(GetAssetAs<UObject>().GetClass()->GetDisplayNameText())
		.TextStyle(FEditorAppStyle::Get(), "Common.InheritedFromBlueprintTextStyle")
		.ToolTipText(VOXEL_LOCTEXT("The class that the current voxel is based on"))
	]
	+ SHorizontalBox::Slot()
	.AutoWidth()
	[
		SNew(SButton)
		.VAlign(VAlign_Center)
		.ButtonStyle(FEditorAppStyle::Get(), "HoverHintOnly")
		.OnClicked_Lambda([=]()
		{
			UBlueprintGeneratedClass* ParentBlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(GetAssetAs<UObject>().GetClass());
			if (ParentBlueprintGeneratedClass)
			{
				if (ParentBlueprintGeneratedClass->ClassGeneratedBy)
				{
					TArray<UObject*> Objects;
					Objects.Add(ParentBlueprintGeneratedClass->ClassGeneratedBy);
					GEditor->SyncBrowserToObjects(Objects);
				}
			}
			
			return FReply::Handled();
		})
		.Visibility_Lambda([=]()
		{
			return Cast<UBlueprintGeneratedClass>(GetAssetAs<UObject>().GetClass()) ? EVisibility::Visible : EVisibility::Collapsed;
		})
		.ToolTipText(VOXEL_LOCTEXT("Find parent in Content Browser"))
		.ContentPadding(4.0f)
		.ForegroundColor(FSlateColor::UseForeground())
		[
			SNew(SImage)
			.Image(FEditorAppStyle::GetBrush("Icons.Search"))
		]
	]
	+ SHorizontalBox::Slot()
	.AutoWidth()
	[
		SNew(SButton)
		.VAlign(VAlign_Center)
		.ButtonStyle(FEditorAppStyle::Get(), "HoverHintOnly")
		.OnClicked_Lambda([=]()
		{
			UBlueprintGeneratedClass* ParentBlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(GetAssetAs<UObject>().GetClass());
			if (ParentBlueprintGeneratedClass)
			{
				if (ParentBlueprintGeneratedClass->ClassGeneratedBy)
				{
					GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(ParentBlueprintGeneratedClass->ClassGeneratedBy);
				}
			}

			return FReply::Handled();
		})
		.Visibility_Lambda([=]()
		{
			return Cast<UBlueprintGeneratedClass>(GetAssetAs<UObject>().GetClass()) ? EVisibility::Visible : EVisibility::Collapsed;
		})
		.ToolTipText( VOXEL_LOCTEXT("Open parent in editor") )
		.ContentPadding(4.0f)
		.ForegroundColor( FSlateColor::UseForeground() )
		[
			SNew(SImage)
			.Image(FEditorAppStyle::GetBrush("Icons.Edit"))
		]
	]
	+ SHorizontalBox::Slot()
	.AutoWidth()
	.VAlign(VAlign_Center)
	[
		SNew(SSpacer)
		.Size(FVector2D(8.0f, 1.0f))
	];

	SetMenuOverlay(MenuOverlayBox);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelSimpleAssetEditorToolkit::NotifyPreChange(FProperty* PropertyAboutToChange)
{
	ErrorMessages.Reset();

	ensure(!MessageScope);
	MessageScope = MakeMessageScope();
}

void FVoxelSimpleAssetEditorToolkit::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	ensure(MessageScope);
	MessageScope.Reset();

	bPreviewQueued = true;

	TUniquePtr<FVoxelScopedMessageConsumer> Scope = MakeMessageScope();
	PostChange();
}

void FVoxelSimpleAssetEditorToolkit::Tick(float DeltaTime)
{
	if (bPreviewQueued)
	{
		CallUpdatePreview();
		bPreviewQueued = false;
	}
}

TStatId FVoxelSimpleAssetEditorToolkit::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FVoxelSimpleAssetEditorToolkit, STATGROUP_Tickables);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelSimpleAssetEditorToolkit::CallUpdatePreview()
{
	TUniquePtr<FVoxelScopedMessageConsumer> Scope = MakeMessageScope();
	UpdatePreview();
}

void FVoxelSimpleAssetEditorToolkit::ClearErrorMessages()
{
	ErrorMessages.Reset();
}

TUniquePtr<FVoxelScopedMessageConsumer> FVoxelSimpleAssetEditorToolkit::MakeMessageScope()
{
	return MakeUnique<FVoxelScopedMessageConsumer>([&](const TSharedRef<FTokenizedMessage>& Message)
	{
		FString Error = Message->ToText().ToString();

		// Remove AssetName: from errors
		const FString AssetName = GetAssetAs<UObject>().GetName();
		Error.RemoveFromStart(AssetName + " : ");

		// Remove the asset path from errors
		const FString AssetPathName = GetAssetAs<UObject>().GetPathName();
		Error.ReplaceInline(*(AssetPathName + "."), TEXT(""));

		ErrorMessages.Add(FText::FromString(Error));
	});
}

void FVoxelSimpleAssetEditorToolkit::UpdateStatsText(const FString& Message)
{
	if (!Viewport)
	{
		InitialStatsText = Message;
		return;
	}

	Viewport->UpdateStatsText(Message);
}

void FVoxelSimpleAssetEditorToolkit::BindToggleCommand(const TSharedPtr<FUICommandInfo>& UICommandInfo, bool& bValue)
{
	ToolkitCommands->MapAction(
		UICommandInfo,
		MakeWeakPtrDelegate(this, [=, &bValue]
		{
			bValue = !bValue;
			UpdatePreview();
		}),
		{},
		MakeWeakPtrDelegate(this, [&bValue]
		{
			return bValue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		}));
}

void FVoxelSimpleAssetEditorToolkit::SetFloorScale(const FVector& Scale) const
{
	VOXEL_CONST_CAST(GetPreviewScene().GetFloorMeshComponent())->SetWorldScale3D(Scale);
}