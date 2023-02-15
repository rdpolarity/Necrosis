// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "SVoxelMetaGraphPaletteItem.h"
#include "SVoxelMetaGraphMembers.h"
#include "SVoxelMetaGraphPinTypeComboBox.h"
#include "SchemaActions/VoxelMetaGraphMemberSchemaAction.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

void SPaletteItem::Construct(const FArguments& InArgs, FCreateWidgetForActionData* const InCreateData)
{
	MembersWidget = InArgs._MembersWidget;
	WeakToolkit = InArgs._Toolkit;

	ActionPtr = InCreateData->Action;
	const TSharedPtr<FEdGraphSchemaAction> Action = ActionPtr.Pin();
	if (!Action)
	{
		return;
	}

	{
		TArray<FString> Categories;
		Action->GetCategory().ToString().ParseIntoArray(Categories, TEXT("|"));
		CategoriesCount = Categories.Num();
	}

	const FMembersColumnSizeData& ColumnSizeData = MembersWidget->GetColumnSizeData();

	const FVoxelPinType Type = StaticCastSharedPtr<FVoxelMetaGraphMembersBaseSchemaAction>(Action)->GetPinType();
	ChildSlot
	[
		SNew(SBox)
		.Padding(FMargin(0.f, -2.f))
		[
			SNew(SSplitter)
			.Style(FVoxelEditorStyle::Get(), "Members.Splitter")
			.PhysicalSplitterHandleSize(1.0f)
			.HitDetectionSplitterHandleSize(5.0f)
			.HighlightedHandleIndex(ColumnSizeData.HoveredSplitterIndex)
			.OnHandleHovered(ColumnSizeData.OnSplitterHandleHovered)
			+ SSplitter::Slot()
			.Value_Lambda([&]()
			{
				return MembersWidget->GetColumnSizeData().NameColumnWidth.Execute(CategoriesCount, GetCachedGeometry().GetAbsoluteSize().X);
			})
			.OnSlotResized(ColumnSizeData.OnNameColumnResized)
			[
				SNew(SBox)
				.Padding(FMargin(0.f, 2.f, 4.f, 2.f))
				[
					CreateTextSlotWidget(InCreateData, false)
				]
			]
			+ SSplitter::Slot()
			.Value_Lambda([&]()
			{
				return MembersWidget->GetColumnSizeData().ValueColumnWidth.Execute(CategoriesCount, GetCachedGeometry().GetAbsoluteSize().X);
			})
			.OnSlotResized_Lambda([&](float NewValue)
			{
				MembersWidget->GetColumnSizeData().OnValueColumnResized.ExecuteIfBound(NewValue, CategoriesCount, GetCachedGeometry().GetAbsoluteSize().X);
			})
			[
				SNew(SBox)
				.Padding(FMargin(8.f, 2.f, 0.f, 2.f))
				.HAlign(HAlign_Left)
				[
					SNew(SPinTypeComboBox)
					.CurrentType(Type)
					.OnTypeChanged_Lambda([this](FVoxelPinType NewValue)
					{
						const TSharedPtr<FEdGraphSchemaAction> Action = ActionPtr.Pin();
						if (!Action)
						{
							return;
						}

						StaticCastSharedPtr<FVoxelMetaGraphMembersBaseSchemaAction>(Action)->SetPinType(NewValue);
					})
					.PinTypes_Lambda([this]
					{
						const TSharedPtr<FEdGraphSchemaAction> Action = ActionPtr.Pin();
						if (!Action)
						{
							return TArray<FVoxelPinType>();
						}
						
						return StaticCastSharedPtr<FVoxelMetaGraphMembersBaseSchemaAction>(Action)->GetPinTypes();
					})
					.DetailsWindow(false)
					.ReadOnly_Lambda([this]
					{
						return !IsHovered();
					})
					.WithContainerType(GetSection(Action->SectionID) != EMembersNodeSection::Parameters)
				]
			]
		]
	];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> SPaletteItem::CreateTextSlotWidget(FCreateWidgetForActionData* const InCreateData, TAttribute<bool> bIsReadOnly)
{
	if (InCreateData->bHandleMouseButtonDown)
	{
		MouseButtonDownDelegate = InCreateData->MouseButtonDownDelegate;
	}

	TSharedPtr<SOverlay> DisplayWidget;
	
	SAssignNew(DisplayWidget, SOverlay)
	+ SOverlay::Slot()
	[
		SAssignNew(InlineRenameWidget, SInlineEditableTextBlock)
		.Text(this, &SPaletteItem::GetDisplayText)
		.HighlightText(InCreateData->HighlightText)
		.OnVerifyTextChanged(this, &SPaletteItem::OnNameTextVerifyChanged)
		.OnTextCommitted(this, &SPaletteItem::OnNameTextCommitted)
		.IsSelected(InCreateData->IsRowSelectedDelegate)
		.IsReadOnly(bIsReadOnly)
	];

	InCreateData->OnRenameRequest->BindSP(InlineRenameWidget.Get(), &SInlineEditableTextBlock::EnterEditingMode);

	return DisplayWidget.ToSharedRef();
}

void SPaletteItem::OnNameTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
{
	const TSharedPtr<FEdGraphSchemaAction> Action = ActionPtr.Pin();
	if (!Action)
	{
		return;
	}

	StaticCastSharedPtr<FVoxelMetaGraphMembersBaseSchemaAction>(Action)->SetName(NewText.ToString(), MembersWidget);
}

FText SPaletteItem::GetDisplayText() const
{
	const TSharedPtr<FEdGraphSchemaAction> Action = ActionPtr.Pin();
	if (!Action)
	{
		return {};
	}

	return FText::FromName(StaticCastSharedPtr<FVoxelMetaGraphMembersBaseSchemaAction>(Action)->GetName());
}

END_VOXEL_NAMESPACE(MetaGraph)