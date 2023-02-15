// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphVariableCollectionCustomization.h"
#include "VoxelMetaGraphVariableCollection.h"
#include "EditorCategoryUtils.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

void FGeneratorCategoryDetails::GenerateHeaderRowContent(FDetailWidgetRow& NodeRow)
{
	NodeRow
	.NameContent()
	[
		SNew(SVoxelDetailText)
		.Text(FText::FromString(FEditorCategoryUtils::GetCategoryDisplayString(Category)))
		.Font(FEditorAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
	];
}

void FGeneratorCategoryDetails::GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder)
{
	const TSharedPtr<FVariableCollectionCustomization> Customization = WeakCustomization.Pin();
	if (!ensure(Customization))
	{
		return;
	}
	
	Properties.KeySort(TLess<int32>());

	for (auto& It : InnerGroups)
	{
		ChildrenBuilder.AddCustomBuilder(It.Value.ToSharedRef());
	}

	for (const TTuple<int32, TSharedPtr<IPropertyHandle>>& It : Properties)
	{
		Customization->AddVariable(nullptr, &ChildrenBuilder, It.Value, false);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVariableCollectionCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	RefreshDelegate = FVoxelEditorUtilities::MakeRefreshDelegate(CustomizationUtils);

	FVoxelEditorUtilities::ForeachData<FVoxelMetaGraphVariableCollection>(PropertyHandle, [&](FVoxelMetaGraphVariableCollection& Collection)
	{
		Collection.RefreshDetails.Add(RefreshDelegate);
	});
}

void FVariableCollectionCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	const TSharedRef<IPropertyHandle> VariablesHandle = InPropertyHandle->GetChildHandleStatic(FVoxelMetaGraphVariableCollection, Variables);
	const TSharedRef<IPropertyHandle> CategoriesHandle = InPropertyHandle->GetChildHandleStatic(FVoxelMetaGraphVariableCollection, Categories);

	uint32 NumVariablesChildren = 0;
	uint32 NumCategoriesChildren = 0;
	if (!ensure(VariablesHandle->GetNumChildren(NumVariablesChildren) == FPropertyAccess::Success) ||
		!ensure(CategoriesHandle->GetNumChildren(NumCategoriesChildren) == FPropertyAccess::Success))
	{
		return;
	}

	IDetailLayoutBuilder& ParentLayout = ChildBuilder.GetParentCategory().GetParentLayout();

	const auto GetCategory = [this](const int32 Index, const TArray<FString>& CategoryChain, TMap<FString, TSharedPtr<FGeneratorCategoryDetails>>& TargetCategories, bool& bNewCategory)
	{
		if (const TSharedPtr<FGeneratorCategoryDetails> TargetDetails = TargetCategories.FindRef(CategoryChain[Index]))
		{
			bNewCategory = false;
			return TargetDetails;
		}

		const TSharedPtr<FGeneratorCategoryDetails> CategoryDetails = MakeShared<FGeneratorCategoryDetails>();
		CategoryDetails->Category = CategoryChain[Index];
		CategoryDetails->WeakCustomization = SharedThis(this);

		TargetCategories.Add(CategoryChain[Index], CategoryDetails);

		bNewCategory = true;

		return CategoryDetails;
	};

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(STATIC_FNAME("PropertyEditor"));
	TSharedRef<FPropertySection> Section = PropertyModule.FindOrCreateSection("VoxelActor", "Voxel", VOXEL_LOCTEXT("Voxel"));
	Section->AddCategory("Voxel");
	Section->AddCategory(*(CategoriesPrefix + "Default"));
	CategoriesOrder.Add(CategoriesPrefix + "Default");

	TMap<FString, TMap<FString, TSharedPtr<FGeneratorCategoryDetails>>> Categories;
	for (uint32 ChildIndex = 0; ChildIndex < NumCategoriesChildren; ChildIndex++)
	{
		const TSharedPtr<IPropertyHandle> ChildHandle = CategoriesHandle->GetChildHandle(ChildIndex);
		
		FString Category;
		if (!ensure(ChildHandle->GetValue(Category) == FPropertyAccess::Success))
		{
			continue;
		}

		if (Category.IsEmpty())
		{
			continue;
		}

		TArray<FString> CategoryChain = GetCategoryChain(Category);
		Section->AddCategory(*(CategoriesPrefix + CategoryChain[0]));
		CategoriesOrder.Add(CategoriesPrefix + CategoryChain[0]);

		TSharedRef<FPropertySection> CategorySection = PropertyModule.FindOrCreateSection("VoxelActor", *CategoryChain[0], FText::FromString(CategoryChain[0]));
		CategorySection->AddCategory(*(CategoriesPrefix + CategoryChain[0]));

		if (CategoryChain.Num() == 1)
		{
			continue;
		}

		TSharedPtr<FGeneratorCategoryDetails> TargetCategoryDetails;
		for (int32 Index = 1; Index < CategoryChain.Num(); Index++)
		{
			bool bNewCategory;
			TargetCategoryDetails = GetCategory(Index, CategoryChain, TargetCategoryDetails ? TargetCategoryDetails->InnerGroups : Categories.FindOrAdd(CategoryChain[0]), bNewCategory);

			if (Index == 1 &&
				bNewCategory)
			{
				IDetailCategoryBuilder& TargetCategory = ParentLayout.EditCategory(*(CategoriesPrefix + CategoryChain[0]));
				TargetCategory.AddCustomBuilder(TargetCategoryDetails.ToSharedRef());
			}
		}
	}

	TMap<int32, TSharedPtr<IPropertyHandle>> RootPropertyHandles;
	for (uint32 ChildIndex = 0; ChildIndex < NumVariablesChildren; ChildIndex++)
	{
		const TSharedPtr<IPropertyHandle> ChildHandle = VariablesHandle->GetChildHandle(ChildIndex);
		
		const FVoxelMetaGraphVariable& Variable = FVoxelEditorUtilities::GetStructPropertyValue<FVoxelMetaGraphVariable>(ChildHandle);
		if (Variable.Category.IsEmpty())
		{
			RootPropertyHandles.Add(Variable.SortIndex, ChildHandle);
			continue;
		}

		TArray<FString> CategoryChain = GetCategoryChain(Variable.Category);

		IDetailCategoryBuilder& TargetCategory = ParentLayout.EditCategory(*(CategoriesPrefix + CategoryChain[0]));
		if (CategoryChain.Num() == 1)
		{
			AddVariable(&TargetCategory, nullptr, ChildHandle, true);
			continue;
		}

		TSharedPtr<FGeneratorCategoryDetails> TargetCategoryDetails;
		for (int32 Index = 1; Index < CategoryChain.Num(); Index++)
		{
			bool bNewCategory;
			TargetCategoryDetails = GetCategory(Index, CategoryChain, TargetCategoryDetails ? TargetCategoryDetails->InnerGroups : Categories.FindOrAdd(CategoryChain[0]), bNewCategory);

			if (Index == 1 &&
				bNewCategory)
			{
				TargetCategory.AddCustomBuilder(TargetCategoryDetails.ToSharedRef());
			}
		}

		if (ensure(TargetCategoryDetails))
		{
			TargetCategoryDetails->Properties.Add(Variable.SortIndex, ChildHandle);
		}
	}

	if (RootPropertyHandles.Num() > 0)
	{
		RootPropertyHandles.KeySort(TLess<int32>());

		IDetailCategoryBuilder& DefaultCategory = ParentLayout.EditCategory(*(CategoriesPrefix + "Default"));
		for (const TTuple<int32, TSharedPtr<IPropertyHandle>>& It : RootPropertyHandles)
		{
			AddVariable(&DefaultCategory, nullptr, It.Value, true);
		}
		
		TSharedRef<FPropertySection> CategorySection = PropertyModule.FindOrCreateSection("VoxelActor", "Default", FText::FromString("Default"));
		CategorySection->AddCategory(*(CategoriesPrefix + "Default"));
	}

	ParentLayout.SortCategories([this](const TMap<FName, IDetailCategoryBuilder*>& AllCategoryMap)
	{
		int32 SortIndexBase = 0;
		if (const IDetailCategoryBuilder* GraphCategory = AllCategoryMap.FindRef("Voxel"))
		{
			SortIndexBase = GraphCategory->GetSortOrder();
		}

		SortIndexBase++;

		for (const auto& It : AllCategoryMap)
		{
			const int32 CategoryIndex = CategoriesOrder.IndexOfByPredicate([&It](const FString& Category)
			{
				return Category == It.Key.ToString();
			});

			if (CategoryIndex == -1)
			{
				continue;
			}

			It.Value->SetSortOrder(SortIndexBase + CategoryIndex);
		}
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVariableCollectionCustomization::PostUndo(bool bSuccess)
{
	// Needed to handle undoing adding/removing orphan variables properly
	RefreshDelegate.ExecuteIfBound();
}
void FVariableCollectionCustomization::PostRedo(bool bSuccess)
{
	// Needed to handle undoing adding/removing orphan variables properly
	RefreshDelegate.ExecuteIfBound();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<FPinValueCustomization> FVariableCollectionCustomization::GetSelfSharedPtr()
{
	return StaticCastSharedRef<FVariableCollectionCustomization>(AsShared());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVariableCollectionCustomization::AddVariable(IDetailCategoryBuilder* CategoryBuilder, IDetailChildrenBuilder* ChildBuilder, const TSharedPtr<IPropertyHandle>& PropertyHandle, bool bUsesCategoryBuilder)
{
	const TSharedRef<IPropertyHandle> ValueHandle = PropertyHandle->GetChildHandleStatic(FVoxelMetaGraphVariable, Value);
	const FVoxelMetaGraphVariable& Variable = FVoxelEditorUtilities::GetStructPropertyValue<FVoxelMetaGraphVariable>(PropertyHandle);

	const auto SetupRow = [&](FDetailWidgetRow& Row, const TSharedRef<IPropertyHandle>& Handle, const TSharedRef<SWidget>& NameWidget, const TSharedRef<SWidget>& ValueWidget)
	{
		Row
		.NameContent()
		[
			SNew(SVoxelDetailText)
			.ColorAndOpacity(Variable.bIsOrphan ? FSlateColor(FLinearColor::Red) : FSlateColor::UseForeground())
			.Text(FText::FromName(Variable.Name))
			.ToolTipText(FText::FromString(Variable.Description))
		]
		.ValueContent()
		[
			ValueWidget
		]
		.OverrideResetToDefault(FResetToDefaultOverride::Create(
			TAttribute<bool>::CreateLambda([PropertyHandle]
			{
				// This happens when refreshing the details panel
				if (!PropertyHandle->GetProperty())
				{
					return false;
				}

				const FVoxelMetaGraphVariable& LocalVariable = FVoxelEditorUtilities::GetStructPropertyValue<FVoxelMetaGraphVariable>(PropertyHandle);
				return !LocalVariable.IsDefault();
			}),
			MakeLambdaDelegate([&Variable, Handle, this]()
			{
				FScopedTransaction Transaction(FText::FromString("Reset " + Variable.Name.ToString() + " to Default"));

				Handle->NotifyPreChange();

				VOXEL_CONST_CAST(Variable).Value = Variable.DefaultValue;

				SyncAllEditableInstancesFromSource();

				Handle->NotifyPostChange(EPropertyChangeType::ValueSet);
				Handle->NotifyFinishedChangingProperties();
			}),
			true
		));
	};

	if (CategoryBuilder)
	{
		AddPinValueCustomization(
			*CategoryBuilder,
			ValueHandle,
			Variable.Value,
			FName::NameToDisplayString(Variable.Name.ToString(), false),
			Variable.Description,
			SetupRow);
	}
	else
	{
		AddPinValueCustomization(
			*ChildBuilder,
			ValueHandle,
			Variable.Value,
			FName::NameToDisplayString(Variable.Name.ToString(), false),
			Variable.Description,
			SetupRow);
	}
}

TArray<FString> FVariableCollectionCustomization::GetCategoryChain(const FString& Category) const
{
	const TCHAR* CategoryDelim = TEXT("|");
	
	TArray<FString> CategoryChain;
	FEditorCategoryUtils::GetCategoryDisplayString(Category).ParseIntoArray(CategoryChain, CategoryDelim, true);

	return CategoryChain;
}

DEFINE_VOXEL_STRUCT_LAYOUT(FVoxelMetaGraphVariableCollection, FVariableCollectionCustomization);

END_VOXEL_NAMESPACE(MetaGraph)