// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Misc/NotifyHook.h"
#include "UObject/GCObject.h"
#include "EditorUndoClient.h"
#include "Toolkits/AssetEditorToolkit.h"

class VOXELCOREEDITOR_API FVoxelBaseEditorToolkit
	: public FAssetEditorToolkit
	, public FGCObject
	, public FNotifyHook
	, public FEditorUndoClient
{
public:
	const FString BaseToolkitName;
	
	explicit FVoxelBaseEditorToolkit(const FString& BaseToolkitName)
		: BaseToolkitName(BaseToolkitName)
	{
	}
	virtual ~FVoxelBaseEditorToolkit() override;

	const TSharedPtr<IDetailsView>& GetDetailsView() const
	{
		return DetailsView;
	}

public:
	using FRegisterTab = TFunctionRef<void(FName TabId, FText DisplayName, FName IconName, TSharedPtr<SWidget> Widget)>;
	
	virtual void InitVoxelEditor(EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UObject* ObjectToEdit);
	// Register + bind commands here
	virtual void BindToolkitCommands() {}
	// Hook to add additional menus
	virtual void BuildMenu(FMenuBarBuilder& MenuBarBuilder) {}
	// Add the toolbar buttons
	virtual void BuildToolbar(FToolBarBuilder& ToolbarBuilder) {}
	// eg create graph objects
	virtual void SetupObjectToEdit() {}
	// Creates all internal widgets for the tabs to point at
	virtual void CreateInternalWidgets();
	// The window layout
	virtual TSharedRef<FTabManager::FLayout> GetLayout() const = 0;
	// Register all the tabs here
	virtual void RegisterTabs(FRegisterTab RegisterTab) = 0;

public:
	//~ Begin IToolkit Interface
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& TabManager) final override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& TabManager) final override;
	virtual FText GetBaseToolkitName() const final override;
	virtual FName GetToolkitFName() const final override { return *BaseToolkitName; }
	virtual FString GetWorldCentricTabPrefix() const final override { return BaseToolkitName; }
	virtual FLinearColor GetWorldCentricTabColorScale() const final override { return FLinearColor(0.3f, 0.2f, 0.5f, 0.5f); }
	//~ End IToolkit Interface

	//~ Begin FGCObject Interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return BaseToolkitName; }
	//~ End FGCObject Interface

	template<typename T>
	T& GetAssetAs() const
	{
		return *CastChecked<T>(ObjectBeingEdited);
	}

private:
	UObject* ObjectBeingEdited = nullptr;
	TSharedPtr<IDetailsView> DetailsView;

	TArray<FName> RegisteredTabIds;

public:
	static TSharedPtr<FVoxelBaseEditorToolkit> MakeToolkit(UClass* Class);

	template<typename T>
	static void RegisterToolkit(UClass* Class)
	{
		ensure(!ToolkitFactories.Contains(Class));
		ToolkitFactories.Add(Class, []
		{
			return MakeShared<T>();
		});
	}

private:
	static TMap<UClass*, TFunction<TSharedPtr<FVoxelBaseEditorToolkit>()>> ToolkitFactories;
};