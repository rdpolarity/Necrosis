// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphFactoryNew.h"
#include "VoxelMetaGraph.h"
#include "Widgets/SVoxelMetaGraphNewAssetDialog.h"
#include "Interfaces/IMainFrameModule.h"

UVoxelMetaGraphFactoryNew::UVoxelMetaGraphFactoryNew(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SupportedClass = UVoxelMetaGraph::StaticClass();
	bEditAfterNew = true;
	bCreateNew = true;
}

bool UVoxelMetaGraphFactoryNew::ConfigureProperties()
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	const IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
	const TSharedPtr<SWindow> ParentWindow = MainFrame.GetParentWindow();

	const TSharedRef<SNewAssetDialog> NewGraphDialog = SNew(SNewAssetDialog);
	FSlateApplication::Get().AddModalWindow(NewGraphDialog, ParentWindow);

	if (!NewGraphDialog->GetUserConfirmedSelection())
	{
		return false;
	}

	GraphToCopy = nullptr;

	TOptional<FAssetData> SelectedGraphAsset = NewGraphDialog->GetSelectedAsset();
	if (SelectedGraphAsset.IsSet())
	{
		GraphToCopy = Cast<UVoxelMetaGraph>(SelectedGraphAsset->GetAsset());
		if (GraphToCopy)
		{
			return true;
		}

		const FText Title = VOXEL_LOCTEXT("Create Default?");
		const EAppReturnType::Type DialogResult = FMessageDialog::Open(
			EAppMsgType::OkCancel,
			EAppReturnType::Cancel,
			VOXEL_LOCTEXT("The selected graph failed to load\nWould you like to create an empty meta graph?"),
			&Title);
		
		if (DialogResult == EAppReturnType::Cancel)
		{
			return false;
		}
	}

	return true;
}

UObject* UVoxelMetaGraphFactoryNew::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	check(Class->IsChildOf(UVoxelMetaGraph::StaticClass()));

	UVoxelMetaGraph* NewGraph;
	if (GraphToCopy)
	{
		NewGraph = Cast<UVoxelMetaGraph>(StaticDuplicateObject(GraphToCopy, InParent, Name, Flags, Class));
	}
	else
	{
		NewGraph = NewObject<UVoxelMetaGraph>(InParent, Class, Name, Flags | RF_Transactional);
	}

	return NewGraph;
}