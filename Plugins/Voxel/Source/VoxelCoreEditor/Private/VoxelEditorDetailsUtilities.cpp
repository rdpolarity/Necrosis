// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelEditorDetailsUtilities.h"
#include "VoxelEditorMinimal.h"
#include "DetailCategoryBuilderImpl.h"
#include "ContentBrowserModule.h"
#include "EditorViewportClient.h"

FString Voxel_GetCommandsName(FString Name)
{
	Name.RemoveFromStart("F");
	Name.RemoveFromEnd("Commands");
	return Name;
}

void FVoxelEditorUtilities::EnableRealtime()
{
	FViewport* Viewport = GEditor->GetActiveViewport();
	if (Viewport)
	{
		FViewportClient* Client = Viewport->GetClient();
		if (Client)
		{
			for (FEditorViewportClient* EditorViewportClient : GEditor->GetAllViewportClients())
			{
				if (EditorViewportClient == Client)
				{
					EditorViewportClient->SetRealtime(true);
					EditorViewportClient->SetShowStats(true); // Show stats as well
					break;
				}
			}
		}
	}
}

FSlateFontInfo FVoxelEditorUtilities::Font()
{
	return FEditorAppStyle::GetFontStyle(/*PropertyEditorConstants::PropertyFontStyle*/ TEXT("PropertyWindow.NormalFont"));
}

#define private public
#include "PropertyEditor/Private/DetailPropertyRow.h"
#undef private

FSimpleDelegate FVoxelEditorUtilities::MakeRefreshDelegate(const IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	const IDetailsViewPrivate* DetailView = static_cast<const FDetailPropertyRow&>(CustomizationUtils).ParentCategory.Pin()->GetDetailsView();
	return MakeRefreshDelegate(CustomizationUtils.GetPropertyUtilities(), DetailView);
}

FSimpleDelegate FVoxelEditorUtilities::MakeRefreshDelegate(const IDetailLayoutBuilder& DetailLayout)
{
	return MakeRefreshDelegate(DetailLayout.GetPropertyUtilities(), DetailLayout.GetDetailsView());
}

FSimpleDelegate FVoxelEditorUtilities::MakeRefreshDelegate(const IDetailCategoryBuilder& CategoryBuilder)
{
	return MakeRefreshDelegate(CategoryBuilder.GetParentLayout());
}

FSimpleDelegate FVoxelEditorUtilities::MakeRefreshDelegate(const IDetailChildrenBuilder& ChildrenBuilder)
{
	return MakeRefreshDelegate(ChildrenBuilder.GetParentCategory());
}

class FVoxelRefreshDelegateTicker : public FVoxelTicker
{
public:
	TSet<TWeakPtr<IPropertyUtilities>> UtilitiesToRefresh;

	//~ Begin FVoxelTicker Interface
	virtual void Tick() override
	{
		VOXEL_FUNCTION_COUNTER();
		check(IsInGameThread());

		const TSet<TWeakPtr<IPropertyUtilities>> UtilitiesToRefreshCopy = MoveTemp(UtilitiesToRefresh);
		ensure(UtilitiesToRefresh.Num() == 0);

		for (const TWeakPtr<IPropertyUtilities>& Utility : UtilitiesToRefreshCopy)
		{
			const TSharedPtr<IPropertyUtilities> Pinned = Utility.Pin();
			if (!Pinned)
			{
				continue;
			}

			Pinned->ForceRefresh();
		}
	}
	//~ End FVoxelTicker Interface
};

FVoxelRefreshDelegateTicker* GVoxelRefreshDelegateTicker = new FVoxelRefreshDelegateTicker();

FSimpleDelegate FVoxelEditorUtilities::MakeRefreshDelegate(const TSharedPtr<IPropertyUtilities>& PropertyUtilities, const IDetailsView* DetailsView)
{
	if (!ensure(PropertyUtilities) ||
		!ensure(DetailsView))
	{
		return {};
	}

	return MakeLambdaDelegate([WeakUtilities = MakeWeakPtr(PropertyUtilities), WeakDetailView = MakeWeakPtr(DetailsView)]()
	{
		if (!WeakDetailView.IsValid())
		{
			// Extra safety to mitigate UE5 bug
			// See https://voxelplugin.atlassian.net/browse/VP-161
			return;
		}

		// Delay call to avoid all kind of issues with doing the refresh immediately
		GVoxelRefreshDelegateTicker->UtilitiesToRefresh.Add(WeakUtilities);
	});	
}

void FVoxelEditorUtilities::SetSortOrder(IDetailLayoutBuilder& DetailLayout, FName Name, ECategoryPriority::Type Priority, int32 PriorityOffset)
{
	static_cast<FDetailCategoryImpl&>(DetailLayout.EditCategory(Name)).SetSortOrder(int32(Priority) * 1000 + PriorityOffset);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelEditorUtilities::RegisterAssetContextMenu(UClass* Class, const FText& Label, const FText& ToolTip, TFunction<void(UObject*)> Lambda)
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	ContentBrowserModule.GetAllAssetViewContextMenuExtenders().Add(FContentBrowserMenuExtender_SelectedAssets::CreateLambda([=](const TArray<FAssetData>& SelectedAssets)
	{
		const TSharedRef<FExtender> Extender = MakeShared<FExtender>();

		for (const FAssetData& Asset : SelectedAssets)
		{
			if (!Asset.GetClass()->IsChildOf(Class))
			{
				return Extender;
			}
		}
		
		Extender->AddMenuExtension(
			"CommonAssetActions",
			EExtensionHook::After,
			nullptr,
			FMenuExtensionDelegate::CreateLambda([=](FMenuBuilder& MenuBuilder)
			{
				MenuBuilder.AddMenuEntry(
				Label,
				ToolTip,
				FSlateIcon(NAME_None, NAME_None),
				FUIAction(FExecuteAction::CreateLambda([=]
				{
					for (const FAssetData& Asset : SelectedAssets)
					{
						UObject* Object = Asset.GetAsset();
						if (ensure(Object) && ensure(Object->IsA(Class)))
						{
							Lambda(Object);
						}
					}
				})));
			}));

		return Extender;
	}));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TArray<FName> FVoxelEditorUtilities::GetPropertyOptions(const TSharedRef<IPropertyHandle>& PropertyHandle)
{
	TArray<TSharedPtr<FString>> SharedOptions;
	TArray<FText> Tooltips;
	TArray<bool> RestrictedItems;
	PropertyHandle->GeneratePossibleValues(SharedOptions, Tooltips, RestrictedItems);

	TArray<FName> Options;
	for (const TSharedPtr<FString>& Option : SharedOptions)
	{
		Options.Add(**Option);
	}
	return Options;
}

TArray<TSharedPtr<IPropertyHandle>> FVoxelEditorUtilities::GetChildHandlesRecursive(const TSharedPtr<IPropertyHandle>& PropertyHandle)
{
	TArray<TSharedPtr<IPropertyHandle>> ChildProperties;

	const TFunction<void(const TSharedPtr<IPropertyHandle>)> AddHandle = [&](const TSharedPtr<IPropertyHandle>& Handle)
	{
		if (!ensure(Handle))
		{
			return;
		}

		ChildProperties.Add(Handle);

		uint32 NumChildren = 0;
		if (!ensure(Handle->GetNumChildren(NumChildren) == FPropertyAccess::Success))
		{
			return;
		}

		for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ChildIndex++)
		{
			AddHandle(Handle->GetChildHandle(ChildIndex));
		}
	};

	AddHandle(PropertyHandle);

	return ChildProperties;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelEditorUtilities::RegisterClassLayout(const UClass* Class, FOnGetDetailCustomizationInstance Delegate)
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	const FName Name = Class->GetFName();
	ensure(!PropertyModule.GetClassNameToDetailLayoutNameMap().Contains(Name));

	PropertyModule.RegisterCustomClassLayout(Name, Delegate);
	PropertyModule.NotifyCustomizationModuleChanged();
}

void FVoxelEditorUtilities::RegisterStructLayout(const UScriptStruct* Struct, FOnGetPropertyTypeCustomizationInstance Delegate, bool bRecursive)
{
	RegisterStructLayout(Struct, Delegate, bRecursive, nullptr);
}

void FVoxelEditorUtilities::RegisterStructLayout(const UScriptStruct* Struct, FOnGetPropertyTypeCustomizationInstance Delegate, bool bRecursive, const TSharedPtr<IPropertyTypeIdentifier>& Identifier)
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PropertyModule.RegisterCustomPropertyTypeLayout(Struct->GetFName(), Delegate, Identifier);

	if (bRecursive)
	{
		VOXEL_FUNCTION_COUNTER();

		ForEachObjectOfClass<UScriptStruct>([&](const UScriptStruct* StructIt)
		{
			if (StructIt->IsChildOf(Struct))
			{
				PropertyModule.RegisterCustomPropertyTypeLayout(StructIt->GetFName(), Delegate, Identifier);
			}
		});
	}
	PropertyModule.NotifyCustomizationModuleChanged();
}

bool FVoxelEditorUtilities::GetRayInfo(FEditorViewportClient* ViewportClient, FVector& OutStart, FVector& OutEnd)
{
	if (!ensure(ViewportClient))
	{
		return false;
	}

	FViewport* Viewport = ViewportClient->Viewport;
	if (!ensure(Viewport))
	{
		return false;
	}

	const int32 MouseX = Viewport->GetMouseX();
	const int32 MouseY = Viewport->GetMouseY();

	FSceneViewFamilyContext ViewFamily(FSceneViewFamilyContext::ConstructionValues(Viewport, ViewportClient->GetScene(), ViewportClient->EngineShowFlags)
		.SetRealtimeUpdate(ViewportClient->IsRealtime()));

	const FSceneView* View = ViewportClient->CalcSceneView(&ViewFamily);

	const FViewportCursorLocation MouseViewportRay(View, ViewportClient, MouseX, MouseY);
	const FVector MouseViewportRayDirection = MouseViewportRay.GetDirection();

	OutStart = MouseViewportRay.GetOrigin();
	OutEnd = OutStart + WORLD_MAX * MouseViewportRayDirection;

	if (ViewportClient->IsOrtho())
	{
		OutStart -= WORLD_MAX * MouseViewportRayDirection;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<FAssetThumbnailPool> FVoxelEditorUtilities::GetThumbnailPool()
{
	class FVoxelThumbnailTicker : public FVoxelTicker
	{
	public:
		const TSharedRef<FAssetThumbnailPool> Pool = MakeShared<FAssetThumbnailPool>(48);

		//~ Begin FVoxelTicker Interface
		virtual void Tick() override
		{
			VOXEL_FUNCTION_COUNTER();

			Pool->Tick(FApp::GetDeltaTime());
		}
		//~ End FVoxelTicker Interface
	};

	static FVoxelThumbnailTicker* Ticker = new FVoxelThumbnailTicker();

	return Ticker->Pool;
}