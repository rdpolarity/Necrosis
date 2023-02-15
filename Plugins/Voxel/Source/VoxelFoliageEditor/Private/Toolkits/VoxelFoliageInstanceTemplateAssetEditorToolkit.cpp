// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelFoliageInstanceTemplateAssetEditorToolkit.h"
#include "VoxelFoliageEditorUtilities.h"
#include "VoxelFoliageRandomGenerator.h"
#include "Widgets/SVoxelFoliageInstanceMeshesList.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Framework/Commands/GenericCommands.h"

const FName FVoxelFoliageInstanceTemplateAssetEditorToolkit::MeshesTabId(TEXT("VoxelFoliageInstanceTemplateAssetEditor_Meshes"));

DEFINE_VOXEL_TOOLKIT(FVoxelFoliageInstanceTemplateAssetEditorToolkit);

void FVoxelFoliageInstanceTemplateAssetEditorToolkit::BindToolkitCommands()
{
	TVoxelSimpleAssetEditorToolkit<UVoxelFoliageInstanceTemplate>::BindToolkitCommands();

	ToolkitCommands->MapAction(FGenericCommands::Get().Delete,
		FExecuteAction::CreateLambda([this]
		{
			if (!MeshesList)
			{
				return;
			}
			MeshesList->DeleteSelectedMesh();
		}),
		FCanExecuteAction::CreateLambda([this]
		{
			return
				MeshesList &&
				MeshesList->GetSelectedMesh();
		}));

	ToolkitCommands->MapAction(FGenericCommands::Get().Duplicate,
		FExecuteAction::CreateLambda([this]
		{
			if (!MeshesList)
			{
				return;
			}
			MeshesList->DuplicateSelectedMesh();
		}),
		FCanExecuteAction::CreateLambda([this]
		{
			return
				MeshesList &&
				MeshesList->GetSelectedMesh();
		}));
}

void FVoxelFoliageInstanceTemplateAssetEditorToolkit::CreateInternalWidgets()
{
	TVoxelSimpleAssetEditorToolkit<UVoxelFoliageInstanceTemplate>::CreateInternalWidgets();

	MeshesList = 
		SNew(SVoxelFoliageInstanceMeshesList)
		.Asset(&GetAsset())
		.Toolkit(SharedThis(this));
}

TSharedRef<FTabManager::FLayout> FVoxelFoliageInstanceTemplateAssetEditorToolkit::GetLayout() const
{
	return FTabManager::NewLayout("Standalone_VoxelFoliageInstanceTemplateAssetEditor_Layout_v1")
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
					->AddTab(MeshesTabId, ETabState::OpenedTab)
				)
			)
		);
}

void FVoxelFoliageInstanceTemplateAssetEditorToolkit::RegisterTabs(FRegisterTab RegisterTab)
{
	TVoxelSimpleAssetEditorToolkit<UVoxelFoliageInstanceTemplate>::RegisterTabs(RegisterTab);
	RegisterTab(MeshesTabId, VOXEL_LOCTEXT("Meshes"), "ShowFlagsMenu.StaticMeshes", MeshesList);
}

void FVoxelFoliageInstanceTemplateAssetEditorToolkit::UpdatePreview()
{
	FVoxelFoliageEditorUtilities::RenderInstanceTemplate(GetAsset(), GetPreviewScene(), StaticMeshComponents);
}

void FVoxelFoliageInstanceTemplateAssetEditorToolkit::DrawPreviewCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas)
{
	FVoxelFoliageRandomGenerator RandomGenerator;

	const UVoxelFoliageInstanceTemplate& TemplateData = GetAsset();

	const int32 HalfX = InViewport.GetSizeXY().X / 2;
	const int32 HalfY = InViewport.GetSizeXY().Y / 2;

	float TotalValue = 0.f;
	float OverallOffset  = 0.f;
	for (int32 Index = 0; Index < TemplateData.Meshes.Num(); Index++)
	{
		const UVoxelFoliageMesh_New* FoliageMesh = TemplateData.Meshes[Index];
		if (!FoliageMesh ||
			!FoliageMesh->StaticMesh)
		{
			continue;
		}

		TotalValue += FoliageMesh->Strength;

		const FVoxelFoliageScaleSettings& ScaleSettings = FoliageMesh->bOverrideScaleSettings ? FoliageMesh->ScaleSettings : TemplateData.ScaleSettings;

		OverallOffset += FoliageMesh->StaticMesh->GetBoundingBox().GetSize().X * ScaleSettings.GetScale(RandomGenerator).X;
	}

	FVector Offset = FVector(OverallOffset / -2.f, 0.f, 0.f);

	FNumberFormattingOptions Options;
	Options.MinimumFractionalDigits = 2;
	Options.MaximumFractionalDigits = 2;

	RandomGenerator.Reset();

	for (int32 Index = 0; Index < TemplateData.Meshes.Num(); Index++)
	{
		const UVoxelFoliageMesh_New* FoliageMesh = TemplateData.Meshes[Index];
		if (!FoliageMesh ||
			!FoliageMesh->StaticMesh)
		{
			continue;
		}

		const FVoxelFoliageOffsetSettings& OffsetSettings = FVoxelFoliageOffsetSettings(TemplateData.OffsetSettings, FoliageMesh->OffsetSettings);
		const FVoxelFoliageScaleSettings& ScaleSettings = FoliageMesh->bOverrideScaleSettings ? FoliageMesh->ScaleSettings : TemplateData.ScaleSettings;
		const FVector3f Scale = ScaleSettings.GetScale(RandomGenerator);

		Offset.X += FoliageMesh->StaticMesh->GetBoundingBox().GetSize().X * Scale.X / 2.f;

		Offset.Y = OffsetSettings.GlobalPositionOffset.Z + OffsetSettings.LocalPositionOffset.Z;
		Offset.Z = OffsetSettings.GlobalPositionOffset.Z + OffsetSettings.LocalPositionOffset.Z;

		const FPlane Projection = View.Project(Offset);
		if (Projection.W > 0.f)
		{
			const int32 XPos = HalfX + (HalfX * Projection.X);
			const int32 YPos = HalfY + (HalfY * (Projection.Y * -1));

			const float Strength = FMath::IsNearlyZero(TotalValue) ? 0.f : TemplateData.Meshes[Index]->Strength / TotalValue * 100.f;
			FString Text = "Strength: " + FText::AsNumber(Strength, &Options).ToString() + "%";

			const int32 StringWidth = GEngine->GetSmallFont()->GetStringSize(*Text) / 2;
			const int32 StringHeight = GEngine->GetSmallFont()->GetStringHeightSize(*Text);

			FCanvasTextItem TextItem(FVector2D(XPos - StringWidth, YPos - StringHeight), FText::FromString(Text), GEngine->GetSmallFont(), FLinearColor::White);
			TextItem.EnableShadow(FLinearColor::Black);
			Canvas.DrawItem(TextItem);
		}

		Offset.X += FoliageMesh->StaticMesh->GetBoundingBox().GetSize().X * Scale.X / 2.f;
	}
}

void FVoxelFoliageInstanceTemplateAssetEditorToolkit::PostChange()
{
	if (!MeshesList)
	{
		return;
	}

	MeshesList->PostChange();
}