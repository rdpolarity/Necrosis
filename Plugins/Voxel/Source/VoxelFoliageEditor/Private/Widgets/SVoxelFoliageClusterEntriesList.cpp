// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "SVoxelFoliageClusterEntriesList.h"
#include "VoxelFoliageClusterTemplate.h"
#include "Toolkits/VoxelFoliageInstanceTemplateAssetEditorToolkit.h"
#include "SAssetDropTarget.h"
#include "Styling/StyleColors.h"
#include "Framework/Commands/GenericCommands.h"

struct FVoxelFoliageClusterEntryWrapper
{
	TWeakObjectPtr<UVoxelFoliageClusterEntry> ClusterEntry;
	TSharedRef<FAssetThumbnail> AssetThumbnail;

	explicit FVoxelFoliageClusterEntryWrapper(UVoxelFoliageClusterEntry* ClusterEntry)
		: ClusterEntry(ClusterEntry)
		, AssetThumbnail(MakeShared<FAssetThumbnail>(ClusterEntry ? ClusterEntry->Instance : nullptr, 32, 32, FVoxelEditorUtilities::GetThumbnailPool()))
	{
	}
};

void SVoxelFoliageClusterEntriesList::Construct(const FArguments& InArgs)
{
	WeakCluster = InArgs._Asset;
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
	EntryDetailsView = PropertyModule.CreateDetailView(Args);

	for (UVoxelFoliageClusterEntry* Entry : WeakCluster->Entries)
	{
		EntriesList.Add(MakeShared<FVoxelFoliageClusterEntryWrapper>(Entry));
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
								.Text(VOXEL_LOCTEXT("Entries"))	
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SButton)
							.ForegroundColor(FSlateColor::UseStyle())
							.ToolTipText(VOXEL_LOCTEXT("Add Cluster Entry"))
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.OnClicked_Lambda([this]
							{
								AddEntry(nullptr, nullptr);
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
								if (Asset.GetClass() != UVoxelFoliageInstanceTemplate::StaticClass())
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
								if (Asset.GetClass() != UVoxelFoliageInstanceTemplate::StaticClass())
								{
									continue;
								}

								AddEntry(nullptr, Cast<UVoxelFoliageInstanceTemplate>(Asset.GetAsset()));
							}
						})
						[
							SAssignNew(EntriesListView, SListView<TSharedPtr<FVoxelFoliageClusterEntryWrapper>>)
							.SelectionMode(ESelectionMode::Single)
							.ListItemsSource(&EntriesList)
							.OnGenerateRow(this, &SVoxelFoliageClusterEntriesList::MakeWidgetFromOption)
							.OnSelectionChanged_Lambda([this](TSharedPtr<FVoxelFoliageClusterEntryWrapper> InItem, ESelectInfo::Type SelectInfo)
							{
								TArray<UObject*> SelectedObjects;
								if (InItem.IsValid())
								{
									SelectedObjects.Add(InItem->ClusterEntry.Get());
								}
								EntryDetailsView->SetObjects(SelectedObjects);
							})
							.ItemHeight(24.0f)
							.OnContextMenuOpening(this, &SVoxelFoliageClusterEntriesList::OnContextMenuOpening)
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
							if (!WeakCluster.IsValid())
							{
								return VOXEL_LOCTEXT("0 entries");
							}

							return FText::Format(VOXEL_LOCTEXT("{0} entries"), FText::AsNumber(WeakCluster->Entries.Num()));
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
						return EntriesListView->GetSelectedItems().Num() > 0 ? EVisibility::Collapsed : EVisibility::Visible;
					})
					[
						SNew(STextBlock)
						.Text(VOXEL_LOCTEXT("Select a Mesh"))
					]
				]
				+ SOverlay::Slot()
				[
					EntryDetailsView.ToSharedRef()
				]
			]
		]
	];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UVoxelFoliageClusterEntry* SVoxelFoliageClusterEntriesList::GetSelectedEntry() const
{
	TArray<TSharedPtr<FVoxelFoliageClusterEntryWrapper>> SelectedItems = EntriesListView->GetSelectedItems();
	if (SelectedItems.Num() == 0)
	{
		return nullptr;
	}

	return SelectedItems[0]->ClusterEntry.Get();
}

void SVoxelFoliageClusterEntriesList::DeleteSelectedEntry()
{
	UVoxelFoliageClusterEntry* SelectedEntry = GetSelectedEntry();
	const TSharedPtr<FVoxelSimpleAssetEditorToolkit> Toolkit = WeakToolkit.Pin();

	if (!SelectedEntry ||
		!WeakCluster.IsValid() ||
		!Toolkit)
	{
		return;
	}

	Toolkit->NotifyPreChange(nullptr);

	WeakCluster->PreEditChange(nullptr);

	WeakCluster->Entries.Remove(SelectedEntry);
	EntriesList.RemoveAll([SelectedEntry](const TSharedPtr<FVoxelFoliageClusterEntryWrapper>& FoliageMeshWrapper)
	{
		return FoliageMeshWrapper->ClusterEntry == SelectedEntry;
	});
	EntriesListView->RequestListRefresh();
	
	WeakCluster->PostEditChange();
	
	const FPropertyChangedEvent PropertyChangedEvent(nullptr);
	Toolkit->NotifyPostChange(PropertyChangedEvent, nullptr);
}

void SVoxelFoliageClusterEntriesList::DuplicateSelectedEntry()
{
	const UVoxelFoliageClusterEntry* SelectedEntry = GetSelectedEntry();
	if (!SelectedEntry)
	{
		return;
	}

	AddEntry(SelectedEntry, nullptr);
}

void SVoxelFoliageClusterEntriesList::PostChange()
{
	const UVoxelFoliageClusterEntry* SelectedEntry = GetSelectedEntry();
	if (!SelectedEntry)
	{
		return;
	}

	for (const TSharedPtr<FVoxelFoliageClusterEntryWrapper>& MeshWrapper : EntriesList)
	{
		if (MeshWrapper->ClusterEntry == SelectedEntry)
		{
			MeshWrapper->AssetThumbnail->SetAsset(MeshWrapper->ClusterEntry->GetFirstInstanceMesh());
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<ITableRow> SVoxelFoliageClusterEntriesList::MakeWidgetFromOption(TSharedPtr<FVoxelFoliageClusterEntryWrapper> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FVoxelFoliageClusterEntryWrapper>>, OwnerTable)
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
					if (!InItem->ClusterEntry.IsValid() ||
						!InItem->ClusterEntry->Instance)
					{
						return VOXEL_LOCTEXT("None");
					}

					return FText::FromString(InItem->ClusterEntry->Instance->GetName());
				})
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
		];
}

TSharedPtr<SWidget> SVoxelFoliageClusterEntriesList::OnContextMenuOpening()
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

void SVoxelFoliageClusterEntriesList::AddEntry(const UVoxelFoliageClusterEntry* EntryToCopy, UVoxelFoliageInstanceTemplate* DefaultTemplate)
{
	const TSharedPtr<FVoxelSimpleAssetEditorToolkit> Toolkit = WeakToolkit.Pin();
	if (!WeakCluster.IsValid() ||
		!Toolkit)
	{
		return;
	}

	const FScopedTransaction Transaction(VOXEL_LOCTEXT("Add Foliage Cluster Entry"));

	UVoxelFoliageClusterEntry* NewMesh;
	if (EntryToCopy)
	{
		NewMesh = DuplicateObject<UVoxelFoliageClusterEntry>(EntryToCopy, WeakCluster.Get());
	}
	else
	{
		NewMesh = NewObject<UVoxelFoliageClusterEntry>(WeakCluster.Get());
	}
	check(NewMesh);

	if (DefaultTemplate)
	{
		NewMesh->Instance = DefaultTemplate;
	}

	Toolkit->NotifyPreChange(nullptr);

	WeakCluster->PreEditChange(nullptr);
	WeakCluster->Entries.Add(NewMesh);
	WeakCluster->PostEditChange();
	
	const FPropertyChangedEvent PropertyChangedEvent(nullptr);
	Toolkit->NotifyPostChange(PropertyChangedEvent, nullptr);

	const TSharedPtr<FVoxelFoliageClusterEntryWrapper> MeshWrapper = MakeShared<FVoxelFoliageClusterEntryWrapper>(NewMesh);
	EntriesList.Add(MeshWrapper);

	EntriesListView->RequestListRefresh();
	EntriesListView->SetSelection(MeshWrapper);
}