// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelFoliageClusterTemplateEditorToolkit.h"
#include "VoxelFoliageEditorUtilities.h"
#include "VoxelFoliageRandomGenerator.h"
#include "Widgets/SVoxelFoliageClusterEntriesList.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "SEditorViewportToolBarMenu.h"
#include "SEditorViewportToolBarButton.h"
#include "Framework/Commands/GenericCommands.h"

const FName FVoxelFoliageClusterTemplateEditorToolkit::EntriesTabId(TEXT("VoxelFoliageClusterTemplateAssetEditor_Entries"));

DEFINE_VOXEL_TOOLKIT(FVoxelFoliageClusterTemplateEditorToolkit);

void FVoxelFoliageClusterTemplateEditorToolkit::BindToolkitCommands()
{
	TVoxelSimpleAssetEditorToolkit<UVoxelFoliageClusterTemplate>::BindToolkitCommands();

	ToolkitCommands->MapAction(FGenericCommands::Get().Delete,
		FExecuteAction::CreateLambda([this]
		{
			if (!EntriesList)
			{
				return;
			}
			EntriesList->DeleteSelectedEntry();
		}),
		FCanExecuteAction::CreateLambda([this]
		{
			return
				EntriesList &&
				EntriesList->GetSelectedEntry();
		}));

	ToolkitCommands->MapAction(FGenericCommands::Get().Duplicate,
		FExecuteAction::CreateLambda([this]
		{
			if (!EntriesList)
			{
				return;
			}
			EntriesList->DuplicateSelectedEntry();
		}),
		FCanExecuteAction::CreateLambda([this]
		{
			return
				EntriesList &&
				EntriesList->GetSelectedEntry();
		}));
}

void FVoxelFoliageClusterTemplateEditorToolkit::CreateInternalWidgets()
{
	TVoxelSimpleAssetEditorToolkit<UVoxelFoliageClusterTemplate>::CreateInternalWidgets();

	EntriesList = 
		SNew(SVoxelFoliageClusterEntriesList)
		.Asset(&GetAsset())
		.Toolkit(SharedThis(this));
}

TSharedRef<FTabManager::FLayout> FVoxelFoliageClusterTemplateEditorToolkit::GetLayout() const
{
	return FTabManager::NewLayout("Standalone_VoxelFoliageClusterTemplateAssetEditor_Layout_v1")
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
					->SetSizeCoefficient(0.2f)
					->AddTab(DetailsTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.6f)
					->AddTab(ViewportTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.2f)
					->AddTab(EntriesTabId, ETabState::OpenedTab)
				)
			)
		);
}

void FVoxelFoliageClusterTemplateEditorToolkit::RegisterTabs(FRegisterTab RegisterTab)
{
	TVoxelSimpleAssetEditorToolkit<UVoxelFoliageClusterTemplate>::RegisterTabs(RegisterTab);
	RegisterTab(EntriesTabId, VOXEL_LOCTEXT("Entries"), "ShowFlagsMenu.StaticMeshes", EntriesList);
}

void FVoxelFoliageClusterTemplateEditorToolkit::UpdatePreview()
{
	FVoxelFoliageEditorUtilities::FClusterRenderData Data;
	Data.Seed = PreviewSeed;
	Data.PreviewInstancesCount = PreviewInstancesCount;

	FVoxelFoliageEditorUtilities::RenderClusterTemplate(GetAsset(), GetPreviewScene(), Data, MeshComponents);

	SetFloorScale({Data.FloorScale, Data.FloorScale, 1.f});

	UpdateStatsText(
		"Visible instances in preview: " + FText::AsNumber(Data.TotalNumInstances).ToString() + "\n"
		"Overall instances: " + FText::AsNumber(Data.MinOverallInstances).ToString() + " - " + FText::AsNumber(Data.MaxOverallInstances).ToString() + "\n"
		"Mesh components: " + FText::AsNumber(Data.MeshComponentsCount).ToString());
}

void FVoxelFoliageClusterTemplateEditorToolkit::PostChange()
{
	if (!EntriesList)
	{
		return;
	}

	EntriesList->PostChange();
}

void FVoxelFoliageClusterTemplateEditorToolkit::PopulateToolBar(const TSharedPtr<SHorizontalBox>& ToolbarBox, const TSharedPtr<SViewportToolBar>& ParentToolBarPtr)
{
	ToolbarBox->AddSlot()
	.AutoWidth()
	.Padding(2.f, 2.f)
	[
		SNew(SEditorViewportToolbarMenu)
		.Label_Lambda([this]
		{
			FString Text = "Instances count: ";
			switch (PreviewInstancesCount)
			{
			case EPreviewInstancesCount::Min: Text += "Min"; break;
			case EPreviewInstancesCount::Random: Text += "Random"; break;
			case EPreviewInstancesCount::Max: Text += "Max"; break;
			}

			return FText::FromString(Text);
		})
		.Cursor(EMouseCursor::Default)
		.ParentToolBar(ParentToolBarPtr)
		.OnGetMenuContent_Lambda([=]
		{
			FMenuBuilder InMenuBuilder(true, nullptr);
			
			InMenuBuilder.BeginSection({}, VOXEL_LOCTEXT("Instances Count"));
			{
				for (const EPreviewInstancesCount InstancesCountType : TEnumRange<EPreviewInstancesCount>())
				{
					FString Label;
					FString ToolTip = "Preview will show ";
					switch (InstancesCountType)
					{
					default: check(false);
					case EPreviewInstancesCount::Min: Label = "Min"; break;
					case EPreviewInstancesCount::Random: Label = "Random"; break;
					case EPreviewInstancesCount::Max: Label = "Max"; break;
					}
					ToolTip += Label.ToLower() + " amount of instances";

					InMenuBuilder.AddMenuEntry(
						FText::FromString(Label),
						FText::FromString(ToolTip),
						FSlateIcon(),
						FUIAction(MakeLambdaDelegate([=]
						{
							PreviewInstancesCount = InstancesCountType;
							CallUpdatePreview();
						}),
						MakeLambdaDelegate([]
						{
							return true;
						}),
						MakeLambdaDelegate([=]
						{
							return PreviewInstancesCount == InstancesCountType ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
						})),
						{},
						EUserInterfaceActionType::ToggleButton
					);
				}
			}
			InMenuBuilder.EndSection();

			return InMenuBuilder.MakeWidget();
		})
	];

	ToolbarBox->AddSlot()
	.AutoWidth()
	.Padding(2.f, 2.f)
	[
		SNew(SEditorViewportToolBarButton)
		.Cursor(EMouseCursor::Default)
		.ButtonType(EUserInterfaceActionType::Button)
		.ButtonStyle(&FEditorAppStyle::Get().GetWidgetStyle<FButtonStyle>("EditorViewportToolBar.WarningButton"))
		.OnClicked_Lambda([=]
		{
			PreviewSeed = FMath::Rand();
			CallUpdatePreview();
			return FReply::Handled();
		})
		.ToolTipText(VOXEL_LOCTEXT("Randomizes preview seed"))
		.Content()
		[
			SNew(STextBlock)
			.Font(FEditorAppStyle::GetFontStyle("EditorViewportToolBar.Font"))
			.Text(VOXEL_LOCTEXT("Randomize Seed"))
			.ColorAndOpacity(FLinearColor::White)
		]
	];
}
