// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "SVoxelFoliageInstanceMeshesList.h"
#include "VoxelFoliageInstanceTemplate.h"
#include "Toolkits/VoxelFoliageInstanceTemplateAssetEditorToolkit.h"
#include "SAssetDropTarget.h"
#include "Styling/StyleColors.h"
#include "Framework/Commands/GenericCommands.h"

struct FVoxelFoliageMeshWrapper
{
	TWeakObjectPtr<UVoxelFoliageMesh_New> FoliageMesh;
	TSharedRef<FAssetThumbnail> AssetThumbnail;

	explicit FVoxelFoliageMeshWrapper(UVoxelFoliageMesh_New* FoliageMesh)
		: FoliageMesh(FoliageMesh)
		, AssetThumbnail(MakeShared<FAssetThumbnail>(FoliageMesh ? FoliageMesh->StaticMesh : nullptr, 32, 32, FVoxelEditorUtilities::GetThumbnailPool()))
	{
	}
};

void SVoxelFoliageInstanceMeshesList::Construct(const FArguments& InArgs)
{
	WeakInstance = InArgs._Asset;
	WeakToolkit = InArgs._Toolkit;

	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.bLockable = false;
	Args.bAllowSearch = false;
	Args.bShowOptions = false;
	if (const TSharedPtr<FVoxelSimpleAssetEditorToolkit> Toolkit = WeakToolkit.Pin())
	{
		Args.NotifyHook = Toolkit.Get();
	}
	Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	MeshDetailsView = PropertyModule.CreateDetailView(Args);

	for (UVoxelFoliageMesh_New* Mesh : WeakInstance->Meshes)
	{
		MeshesList.Add(MakeShared<FVoxelFoliageMeshWrapper>(Mesh));
	}

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SSplitter)
			.Orientation(Orient_Vertical)
			+ SSplitter::Slot()
			.Value(.3f)
			[
				SNew(SBorder)
				.BorderImage(FEditorAppStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						[
							SNew(SBorder)
							.BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
							.HAlign(HAlign_Left)
							.Padding(FMargin(12.f, 6.f))
							[
								SNew(STextBlock)
								.TextStyle(FAppStyle::Get(), "ButtonText")
								.Text(VOXEL_LOCTEXT("Meshes"))	
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SButton)
							.ForegroundColor(FSlateColor::UseStyle())
							.ToolTipText(VOXEL_LOCTEXT("Add Foliage Mesh"))
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.OnClicked_Lambda([this]
							{
								AddMesh(nullptr, nullptr);
								return FReply::Handled();
							})
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								.AutoWidth()
								[
									SNew(SImage)
									.Image(FAppStyle::Get().GetBrush("Icons.Plus"))
									.ColorAndOpacity(FStyleColors::AccentGreen)
								]
								+ SHorizontalBox::Slot()
								.Padding(FMargin(3, 0, 0, 0))
								.VAlign(VAlign_Center)
								.AutoWidth()
								[
									SNew(STextBlock)
									.TextStyle(FAppStyle::Get(), "SmallButtonText")
									.Text(VOXEL_LOCTEXT("Add"))
								]
							]
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SSeparator)
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SNew(SAssetDropTarget)
						.OnAreAssetsAcceptableForDropWithReason_Lambda([](TArrayView<FAssetData> AssetDatas, FText& OutReason)
						{
							for (const FAssetData& Asset : AssetDatas)
							{
								if (Asset.GetClass() != UStaticMesh::StaticClass())
								{
									return false;
								}
							}

							return true;
						})
						.OnAssetsDropped_Lambda([this](const FDragDropEvent& Event, TArrayView<FAssetData> AssetDatas)
						{
							for (const FAssetData& Asset : AssetDatas)
							{
								if (Asset.GetClass() != UStaticMesh::StaticClass())
								{
									continue;
								}

								AddMesh(nullptr, Cast<UStaticMesh>(Asset.GetAsset()));
							}
						})
						[
							SAssignNew(MeshesListView, SListView<TSharedPtr<FVoxelFoliageMeshWrapper>>)
							.SelectionMode(ESelectionMode::Single)
							.ListItemsSource(&MeshesList)
							.OnGenerateRow(this, &SVoxelFoliageInstanceMeshesList::MakeWidgetFromOption)
							.OnSelectionChanged_Lambda([this](TSharedPtr<FVoxelFoliageMeshWrapper> InItem, ESelectInfo::Type SelectInfo)
							{
								TArray<UObject*> SelectedObjects;
								if (InItem.IsValid())
								{
									SelectedObjects.Add(InItem->FoliageMesh.Get());
								}
								MeshDetailsView->SetObjects(SelectedObjects);
							})
							.ItemHeight(24.0f)
							.OnContextMenuOpening(this, &SVoxelFoliageInstanceMeshesList::OnContextMenuOpening)
							.HeaderRow
							(
								SNew(SHeaderRow)
								.Visibility(EVisibility::Collapsed)
								+ SHeaderRow::Column(TEXT("Mesh"))
								.HAlignCell(HAlign_Fill)
								.VAlignCell(VAlign_Center)
							)
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(12.f, 6.f))
					[
						SNew(STextBlock)
						.Text_Lambda([this]
						{
							if (!WeakInstance.IsValid())
							{
								return VOXEL_LOCTEXT("0 meshes");
							}

							return FText::Format(VOXEL_LOCTEXT("{0} meshes"), FText::AsNumber(WeakInstance->Meshes.Num()));
						})
					]
				]
			]
			+ SSplitter::Slot()
			.Value(0.7f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBorder)
					.BorderImage(FEditorAppStyle::GetBrush("ToolPanel.GroupBorder"))
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Visibility_Lambda([this]
					{
						return MeshesListView->GetSelectedItems().Num() > 0 ? EVisibility::Collapsed : EVisibility::Visible;
					})
					[
						SNew(STextBlock)
						.Text(VOXEL_LOCTEXT("Select a Mesh"))
					]
				]
				+ SOverlay::Slot()
				[
					MeshDetailsView.ToSharedRef()
				]
			]
		]
	];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UVoxelFoliageMesh_New* SVoxelFoliageInstanceMeshesList::GetSelectedMesh() const
{
	TArray<TSharedPtr<FVoxelFoliageMeshWrapper>> SelectedItems = MeshesListView->GetSelectedItems();
	if (SelectedItems.Num() == 0)
	{
		return nullptr;
	}

	return SelectedItems[0]->FoliageMesh.Get();
}

void SVoxelFoliageInstanceMeshesList::DeleteSelectedMesh()
{
	UVoxelFoliageMesh_New* SelectedMesh = GetSelectedMesh();
	const TSharedPtr<FVoxelSimpleAssetEditorToolkit> Toolkit = WeakToolkit.Pin();

	if (!SelectedMesh ||
		!WeakInstance.IsValid() ||
		!Toolkit)
	{
		return;
	}

	Toolkit->NotifyPreChange(nullptr);

	WeakInstance->PreEditChange(nullptr);

	WeakInstance->Meshes.Remove(SelectedMesh);
	MeshesList.RemoveAll([SelectedMesh](const TSharedPtr<FVoxelFoliageMeshWrapper>& FoliageMeshWrapper)
	{
		return FoliageMeshWrapper->FoliageMesh == SelectedMesh;
	});
	MeshesListView->RequestListRefresh();
	
	WeakInstance->PostEditChange();
	
	const FPropertyChangedEvent PropertyChangedEvent(nullptr);
	Toolkit->NotifyPostChange(PropertyChangedEvent, nullptr);
}

void SVoxelFoliageInstanceMeshesList::DuplicateSelectedMesh()
{
	const UVoxelFoliageMesh_New* SelectedMesh = GetSelectedMesh();
	if (!SelectedMesh)
	{
		return;
	}

	AddMesh(SelectedMesh, nullptr);
}

void SVoxelFoliageInstanceMeshesList::PostChange()
{
	const UVoxelFoliageMesh_New* SelectedMesh = GetSelectedMesh();
	if (!SelectedMesh)
	{
		return;
	}

	for (const TSharedPtr<FVoxelFoliageMeshWrapper>& MeshWrapper : MeshesList)
	{
		if (MeshWrapper->FoliageMesh == SelectedMesh)
		{
			MeshWrapper->AssetThumbnail->SetAsset(MeshWrapper->FoliageMesh->StaticMesh);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<ITableRow> SVoxelFoliageInstanceMeshesList::MakeWidgetFromOption(TSharedPtr<FVoxelFoliageMeshWrapper> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FVoxelFoliageMeshWrapper>>, OwnerTable)
		.Padding(4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(15.f, 3.f, 10.f, 3.f)
			[
				SNew(SBox)
				.WidthOverride(32.f)
				.HeightOverride(32.f)
				[
					InItem->AssetThumbnail->MakeThumbnailWidget()
				]
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([InItem]
				{
					return FText::FromString(InItem->FoliageMesh.IsValid() && InItem->FoliageMesh->StaticMesh ? InItem->FoliageMesh->StaticMesh->GetName() : "None");
				})
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
		];
}

TSharedPtr<SWidget> SVoxelFoliageInstanceMeshesList::OnContextMenuOpening()
{
	const TSharedPtr<FVoxelSimpleAssetEditorToolkit> Toolkit = WeakToolkit.Pin();

	FMenuBuilder MenuBuilder(true, Toolkit->GetToolkitCommands());
	{
		MenuBuilder.BeginSection("BasicOperations");
		{
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Delete);
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Duplicate);
		}
		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

void SVoxelFoliageInstanceMeshesList::AddMesh(const UVoxelFoliageMesh_New* FoliageMeshToCopy, UStaticMesh* DefaultMesh)
{
	const TSharedPtr<FVoxelSimpleAssetEditorToolkit> Toolkit = WeakToolkit.Pin();
	if (!WeakInstance.IsValid() ||
		!Toolkit)
	{
		return;
	}

	const FScopedTransaction Transaction(VOXEL_LOCTEXT("Add Foliage Mesh"));

	UVoxelFoliageMesh_New* NewMesh;
	if (FoliageMeshToCopy)
	{
		NewMesh = DuplicateObject<UVoxelFoliageMesh_New>(FoliageMeshToCopy, WeakInstance.Get());
	}
	else
	{
		NewMesh = NewObject<UVoxelFoliageMesh_New>(WeakInstance.Get());
	}
	check(NewMesh);

	if (DefaultMesh)
	{
		NewMesh->StaticMesh = DefaultMesh;
	}

	Toolkit->NotifyPreChange(nullptr);

	WeakInstance->PreEditChange(nullptr);
	WeakInstance->Meshes.Add(NewMesh);
	WeakInstance->PostEditChange();
	
	const FPropertyChangedEvent PropertyChangedEvent(nullptr);
	Toolkit->NotifyPostChange(PropertyChangedEvent, nullptr);

	const TSharedPtr<FVoxelFoliageMeshWrapper> MeshWrapper = MakeShared<FVoxelFoliageMeshWrapper>(NewMesh);
	MeshesList.Add(MeshWrapper);

	MeshesListView->RequestListRefresh();
	MeshesListView->SetSelection(MeshWrapper);
}