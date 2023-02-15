// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "SVoxelMetaGraphPinTypeSelector.h"
#include "VoxelNode.h"
#include "VoxelMetaGraphSchema.h"
#include "VoxelMetaGraphVisuals.h"
#include "Styling/SlateIconFinder.h"
#include "SListViewSelectorDropdownMenu.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

VOXEL_INITIALIZE_STYLE(GraphPinTypeSelectorEditor)
{
	Set("Pill.Buffer", new IMAGE_BRUSH_SVG("Graphs/BufferPill", CoreStyleConstants::Icon16x16));
}

BEGIN_VOXEL_NAMESPACE(MetaGraph)

struct FSelectorPinTypeRow
{
public:
	FString Name;
	FVoxelPinType Type;
	TArray<TSharedPtr<FSelectorPinTypeRow>> Types;

	FSelectorPinTypeRow() = default;

	FSelectorPinTypeRow(FVoxelPinType PinType)
		: Name(PinType.ToString())
		, Type(PinType)
	{
	}

	FSelectorPinTypeRow(FString Name)
		: Name(Name)
	{
	}

	FSelectorPinTypeRow(FString Name, const TArray<TSharedPtr<FSelectorPinTypeRow>>& Types)
		: Name(Name)
		, Types(Types)
	{
	}
};

void SPinTypeSelector::Construct(const FArguments& InArgs)
{
	PinTypesAttribute = InArgs._PinTypes;

	OnTypeChanged = InArgs._OnTypeChanged;
	ensure(OnTypeChanged.IsBound());
	OnCloseMenu = InArgs._OnCloseMenu;

	FillTypesList();

	SAssignNew(TypeTreeView, SPinTypeTreeView)
	.TreeItemsSource(&FilteredTypesList)
	.SelectionMode(ESelectionMode::Single)
	.OnGenerateRow(this, &SPinTypeSelector::GenerateTypeTreeRow)
	.OnSelectionChanged(this, &SPinTypeSelector::OnTypeSelectionChanged)
	.OnGetChildren(this, &SPinTypeSelector::GetTypeChildren);

	SAssignNew(FilterTextBox, SSearchBox)
	.OnTextChanged(this, &SPinTypeSelector::OnFilterTextChanged)
	.OnTextCommitted(this, &SPinTypeSelector::OnFilterTextCommitted);

	ChildSlot
	[
		SNew(SListViewSelectorDropdownMenu<TSharedPtr<FSelectorPinTypeRow>>, FilterTextBox, TypeTreeView)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.f)
			[
				FilterTextBox.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.f)
			[
				SNew(SBox)
				.HeightOverride(400.f)
				.WidthOverride(300.f)
				[
					TypeTreeView.ToSharedRef()
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(8.f, 0.f, 8.f, 4.f)
			[
				SNew(STextBlock)
				.Text_Lambda([this]
				{
					const FString ItemText = FilteredTypesCount == 1 ? " item" : " items";
					return FText::FromString(FText::AsNumber(FilteredTypesCount).ToString() + ItemText);
				})
			]
		]
	];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SPinTypeSelector::ClearSelection() const
{
	TypeTreeView->SetSelection(nullptr, ESelectInfo::OnNavigation);
	TypeTreeView->ClearExpandedItems();
}

TSharedPtr<SWidget> SPinTypeSelector::GetWidgetToFocus() const
{
	return FilterTextBox;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<ITableRow> SPinTypeSelector::GenerateTypeTreeRow(TSharedPtr<FSelectorPinTypeRow> RowItem, const TSharedRef<STableViewBase>& OwnerTable) const
{
	const FLinearColor Color =!RowItem->Types.IsEmpty() ? FLinearColor::White : GetColor(RowItem->Type);
	const FSlateBrush* Image = !RowItem->Types.IsEmpty() ? FEditorAppStyle::GetBrush("NoBrush") : GetIcon(RowItem->Type);

	return
		SNew(STableRow<TSharedPtr<FSelectorPinTypeRow>>, OwnerTable)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(1.f)
			[
				SNew(SImage)
				.Image(Image)
				.ColorAndOpacity(Color)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(1.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(RowItem->Name))
				.HighlightText_Lambda([this]
				{
					return SearchText;
				})
				.Font(!RowItem->Types.IsEmpty() ? FEditorAppStyle::GetFontStyle(TEXT("Kismet.TypePicker.CategoryFont")) : FEditorAppStyle::GetFontStyle(TEXT("Kismet.TypePicker.NormalFont")))
			]
		];
}

void SPinTypeSelector::OnTypeSelectionChanged(TSharedPtr<FSelectorPinTypeRow> Selection, ESelectInfo::Type SelectInfo) const
{
	if (SelectInfo == ESelectInfo::OnNavigation)
	{
		if (TypeTreeView->WidgetFromItem(Selection).IsValid())
		{
			OnCloseMenu.ExecuteIfBound();
		}

		return;
	}

	if (!Selection)
	{
		return;
	}

	if (Selection->Types.Num() == 0)
	{
		OnCloseMenu.ExecuteIfBound();
		OnTypeChanged.ExecuteIfBound(bConvertToBuffer ? Selection->Type.GetBufferType() : Selection->Type);
		return;
	}

	TypeTreeView->SetItemExpansion(Selection, !TypeTreeView->IsItemExpanded(Selection));

	if (SelectInfo == ESelectInfo::OnMouseClick)
	{
		TypeTreeView->ClearSelection();
	}
}

void SPinTypeSelector::GetTypeChildren(TSharedPtr<FSelectorPinTypeRow> PinTypeRow, TArray<TSharedPtr<FSelectorPinTypeRow>>& PinTypeRows) const
{
	PinTypeRows = PinTypeRow->Types;
}

void SPinTypeSelector::OnFilterTextChanged(const FText& NewText)
{
	SearchText = NewText;
	FilteredTypesList = {};

	GetChildrenMatchingSearch(NewText);
	TypeTreeView->RequestTreeRefresh();
}

void SPinTypeSelector::OnFilterTextCommitted(const FText& NewText, ETextCommit::Type CommitInfo) const
{
	if (CommitInfo != ETextCommit::OnEnter)
	{
		return;
	}

	TArray<TSharedPtr<FSelectorPinTypeRow>> SelectedItems = TypeTreeView->GetSelectedItems();
	if (SelectedItems.Num() > 0)
	{
		TypeTreeView->SetSelection(SelectedItems[0]);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SPinTypeSelector::GetChildrenMatchingSearch(const FText& InSearchText)
{
	FilteredTypesCount = 0;

	TArray<FString> FilterTerms;
	TArray<FString> SanitizedFilterTerms;

	if (InSearchText.IsEmpty())
	{
		FilteredTypesList = TypesList;
		FilteredTypesCount = TotalTypesCount;
		return;
	}

	FText::TrimPrecedingAndTrailing(InSearchText).ToString().ParseIntoArray(FilterTerms, TEXT(" "), true);

	for (int32 Index = 0; Index < FilterTerms.Num(); Index++)
	{
		FString EachString = FName::NameToDisplayString(FilterTerms[Index], false);
		EachString = EachString.Replace(TEXT(" "), TEXT(""));
		SanitizedFilterTerms.Add(EachString);
	}

	ensure(SanitizedFilterTerms.Num() == FilterTerms.Num());

	const auto SearchMatches = [&FilterTerms, &SanitizedFilterTerms](const TSharedPtr<FSelectorPinTypeRow>& TypeRow) -> bool
	{
		FString ItemName = TypeRow->Name;
		ItemName = ItemName.Replace(TEXT(" "), TEXT(""));

		for (int32 Index = 0; Index < FilterTerms.Num(); ++Index)
		{
			if (ItemName.Contains(FilterTerms[Index]) ||
				ItemName.Contains(SanitizedFilterTerms[Index]))
			{
				return true;
			}
		}

		return false;
	};

	const auto LookThroughList = [&](const TArray<TSharedPtr<FSelectorPinTypeRow>>& UnfilteredList, TArray<TSharedPtr<FSelectorPinTypeRow>>& OutFilteredList, auto& Lambda) -> bool
	{
		bool bReturnVal = false;
		for (const TSharedPtr<FSelectorPinTypeRow>& TypeRow : UnfilteredList)
		{
			const bool bMatchesItem = SearchMatches(TypeRow);
			if (TypeRow->Types.Num() == 0 ||
				bMatchesItem)
			{
				if (bMatchesItem)
				{
					OutFilteredList.Add(TypeRow);

					if (TypeRow->Types.Num() > 0 &&
						TypeTreeView.IsValid())
					{
						TypeTreeView->SetItemExpansion(TypeRow, true);
					}

					FilteredTypesCount += FMath::Max(1, TypeRow->Types.Num());

					bReturnVal = true;
				}
				continue;
			}

			TArray<TSharedPtr<FSelectorPinTypeRow>> ValidChildren;
			if (Lambda(TypeRow->Types, ValidChildren, Lambda))
			{
				TSharedRef<FSelectorPinTypeRow> NewCategory = MakeShared<FSelectorPinTypeRow>(TypeRow->Name, ValidChildren);
				OutFilteredList.Add(NewCategory);
				
				if (TypeTreeView.IsValid())
				{
					TypeTreeView->SetItemExpansion(NewCategory, true);
				}

				bReturnVal = true;
			}
		}

		return bReturnVal;
	};

	LookThroughList(TypesList, FilteredTypesList, LookThroughList);
}

void SPinTypeSelector::FillTypesList()
{
	TypesList = {};

	TMap<FString, TSharedPtr<FSelectorPinTypeRow>> Categories;

	int32 IgnoredTypesCount = 0;

	TArray<FVoxelPinType> Types = PinTypesAttribute.Get();

	bConvertToBuffer = true;
	for (const FVoxelPinType& Type : Types)
	{
		if (!Type.IsBuffer())
		{
			bConvertToBuffer = false;
			break;
		}
	}

	if (bConvertToBuffer)
	{
		const TArray<FVoxelPinType> OriginalTypes = MoveTemp(Types);
		ensure(Types.Num() == 0);

		for (const FVoxelPinType& Type : OriginalTypes)
		{
			Types.Add(Type.GetInnerType());
		}
	}

	for (const FVoxelPinType& Type : Types)
	{
		if (Type.IsBuffer())
		{
			IgnoredTypesCount++;
			continue;
		}

		const FVoxelPinType ExposedType = Type.GetExposedType();

		FString TargetCategory = "Default";

		if (ExposedType.IsEnum())
		{
			TargetCategory = "Enums";
		}
		else if (ExposedType.IsObject())
		{
			TargetCategory = "Objects";
		}
		else if (ExposedType.IsStruct())
		{
			const UScriptStruct* Struct = ExposedType.GetStruct();
			if (Struct->HasMetaData("TypeCategory"))
			{
				TargetCategory = Struct->GetMetaData("TypeCategory");
			}
			else if (
				!ExposedType.Is<FVector2D>() &&
				!ExposedType.Is<FVector>() &&
				!ExposedType.Is<FRotator>() &&
				!ExposedType.Is<FLinearColor>() &&
				!ExposedType.Is<FIntPoint>() &&
				!ExposedType.Is<FIntVector>() &&
				!ExposedType.Is<FQuat>())
			{
				TargetCategory = "Structs";
			}
		}

		if (TargetCategory.IsEmpty() ||
			TargetCategory == "Default")
		{
			TypesList.Add(MakeShared<FSelectorPinTypeRow>(Type));
			continue;
		}

		if (!Categories.Contains(TargetCategory))
		{
			Categories.Add(TargetCategory, MakeShared<FSelectorPinTypeRow>(TargetCategory));
		}

		Categories[TargetCategory]->Types.Add(MakeShared<FSelectorPinTypeRow>(Type));
	}

	for (const auto& It : Categories)
	{
		TypesList.Add(It.Value);
	}
	FilteredTypesList = TypesList;

	TotalTypesCount = Types.Num() - IgnoredTypesCount;
	FilteredTypesCount = Types.Num() - IgnoredTypesCount;
}

const FSlateBrush* SPinTypeSelector::GetIcon(const FVoxelPinType& PinType) const
{
	return FVoxelMetaGraphVisuals::GetPinIcon(PinType).GetIcon();
}

FLinearColor SPinTypeSelector::GetColor(const FVoxelPinType& PinType) const
{
	return FVoxelMetaGraphVisuals::GetPinColor(PinType);
}

END_VOXEL_NAMESPACE(MetaGraph)