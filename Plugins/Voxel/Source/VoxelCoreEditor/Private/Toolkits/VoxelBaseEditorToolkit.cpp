// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Toolkits/VoxelBaseEditorToolkit.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"

FVoxelBaseEditorToolkit::~FVoxelBaseEditorToolkit()
{
	GEditor->UnregisterForUndo(this);
}

void FVoxelBaseEditorToolkit::InitVoxelEditor(EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UObject* ObjectToEdit)
{
	ObjectBeingEdited = ObjectToEdit;
	ObjectBeingEdited->SetFlags(RF_Transactional);

	GEditor->RegisterForUndo(this);

	SetupObjectToEdit();
	CreateInternalWidgets();
	BindToolkitCommands();

	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = GetLayout();

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(
		Mode, 
		InitToolkitHost, 
		*(BaseToolkitName + TEXT("App")),
		StandaloneDefaultLayout, 
		bCreateDefaultStandaloneMenu, 
		bCreateDefaultToolbar, 
		ObjectToEdit, 
		false);

	{

		TSharedRef<FExtender> MenuExtender = MakeShared<FExtender>();

		MenuExtender->AddMenuBarExtension(
			"Edit",
			EExtensionHook::After,
			GetToolkitCommands(),
			FMenuBarExtensionDelegate::CreateRaw(this, &FVoxelBaseEditorToolkit::BuildMenu));

		AddMenuExtender(MenuExtender);
	}
	
	{
		TSharedRef<FExtender> ToolbarExtender = MakeShared<FExtender>();

		ToolbarExtender->AddToolBarExtension(
			"Asset",
			EExtensionHook::After,
			GetToolkitCommands(),
			FToolBarExtensionDelegate::CreateSP(this, &FVoxelBaseEditorToolkit::BuildToolbar)
		);

		AddToolbarExtender(ToolbarExtender);
	}

	RegenerateMenusAndToolbars();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelBaseEditorToolkit::CreateInternalWidgets()
{
	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.NotifyHook = this;
	Args.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;
	
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	DetailsView = PropertyModule.CreateDetailView(Args);
	DetailsView->SetObject(ObjectBeingEdited);
}

void FVoxelBaseEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(GetBaseToolkitName());

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	RegisterTabs([&](FName TabId, FText DisplayName, FName IconName, TSharedPtr<SWidget> Widget)
	{
		const FOnSpawnTab OnSpawnTab = FOnSpawnTab::CreateLambda([TabId, DisplayName, IconName, WeakWidget = MakeWeakPtr(Widget)](const FSpawnTabArgs& Args)
		{
			check(Args.GetTabId() == TabId);
			
			return SNew(SDockTab)
				.Label(DisplayName)
				[
					WeakWidget.IsValid() ? WeakWidget.Pin().ToSharedRef() : SNullWidget::NullWidget
				];
		});

		InTabManager->RegisterTabSpawner(TabId, OnSpawnTab)
			.SetDisplayName(DisplayName)
			.SetGroup(WorkspaceMenuCategory.ToSharedRef())
			.SetIcon(FSlateIcon(UE_501_SWITCH(FEditorStyle::GetStyleSetName(), FAppStyle::GetAppStyleSetName()), IconName));

		ensure(!RegisteredTabIds.Contains(TabId));
		RegisteredTabIds.Add(TabId);
	});
}

void FVoxelBaseEditorToolkit::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	for (const FName& TabId : RegisteredTabIds)
	{
		InTabManager->UnregisterTabSpawner(TabId);
	}
}

FText FVoxelBaseEditorToolkit::GetBaseToolkitName() const
{
	return FText::FromString(FName::NameToDisplayString(BaseToolkitName, false));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelBaseEditorToolkit::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(ObjectBeingEdited);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TMap<UClass*, TFunction<TSharedPtr<FVoxelBaseEditorToolkit>()>> FVoxelBaseEditorToolkit::ToolkitFactories;

TSharedPtr<FVoxelBaseEditorToolkit> FVoxelBaseEditorToolkit::MakeToolkit(UClass* Class)
{
	while (Class && Class != UObject::StaticClass())
	{
		TFunction<TSharedPtr<FVoxelBaseEditorToolkit>()>* Factory = ToolkitFactories.Find(Class);
		if (Factory)
		{
			return (*Factory)();
		}

		Class = Class->GetSuperClass();
	}

	return nullptr;
}