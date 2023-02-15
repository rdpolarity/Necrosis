// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphNodePinArrayCustomization.h"
#include "Nodes/VoxelMetaGraphStructNode.h"
#include "VoxelMetaGraphNodeCustomization.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

void FVoxelNodePinArrayCustomization::GenerateHeaderRowContent(FDetailWidgetRow& NodeRow)
{
	const TSharedRef<SWidget> AddButton = PropertyCustomizationHelpers::MakeAddButton(
		FSimpleDelegate::CreateSP(this, &FVoxelNodePinArrayCustomization::AddNewPin),
		VOXEL_LOCTEXT("Add Element"),
		MakeAttributeSP(this, &FVoxelNodePinArrayCustomization::CanAddNewPin));

	const TSharedRef<SWidget> ClearButton = PropertyCustomizationHelpers::MakeEmptyButton(
		FSimpleDelegate::CreateSP(this, &FVoxelNodePinArrayCustomization::ClearAllPins),
		VOXEL_LOCTEXT("Removes All Elements"),
		MakeAttributeSP(this, &FVoxelNodePinArrayCustomization::CanRemovePin));

	NodeRow
	.NameContent()
	[
		SNew(SVoxelDetailText)
		.Text(FText::FromString(FName::NameToDisplayString(PinName, false)))
	]
	.ValueContent()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			SNew(SVoxelDetailText)
			.Text(FText::FromString(LexToString(Properties.Num()) + " Array elements"))
		]
		+ SHorizontalBox::Slot()
		.Padding(2.0f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			AddButton
		]
		+ SHorizontalBox::Slot()
		.Padding(2.0f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			ClearButton
		]
	];
}

void FVoxelNodePinArrayCustomization::GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder)
{
	const TSharedPtr<FVoxelNodeCustomization> Customization = WeakCustomization.Pin();
	if (!ensure(Customization))
	{
		return;
	}

	int32 Index = 0;
	for (const TSharedPtr<IPropertyHandle>& Handle : Properties)
	{
		const TSharedPtr<IPropertyHandle> KeyHandle = Handle->GetKeyHandle();

		if (!ensure(KeyHandle))
		{
			continue;
		}

		FName ElementPinName;
		KeyHandle->GetValue(ElementPinName);

		if (!ensure(!ElementPinName.IsNone()))
		{
			continue;
		}

		const FVoxelPinValue& PinValue = FVoxelEditorUtilities::GetStructPropertyValue<FVoxelPinValue>(Handle);

		Customization->AddPinValueCustomization(
			ChildrenBuilder,
			Handle.ToSharedRef(),
			PinValue,
			LexToString(Index++),
			"",
			[this, Customization, WeakNode = WeakStructNode, ElementPinName](FDetailWidgetRow& Row, const TSharedRef<IPropertyHandle>& Handle, const TSharedRef<SWidget>& NameWidget, const TSharedRef<SWidget>& ValueWidget)
			{
				Row
				.NameContent()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.Padding(0, 0, 3, 0)
					.AutoWidth()
					[
						SNew(SVoxelDetailText)
						.Text(VOXEL_LOCTEXT("Index"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.Padding(0, 0, 3, 0)
					.AutoWidth()
					[
						SNew(SVoxelDetailText)
						.Text(VOXEL_LOCTEXT("["))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.Padding(0, 0, 3, 0)
					.AutoWidth()
					[
						NameWidget
					]
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						SNew(SVoxelDetailText)
						.Text(VOXEL_LOCTEXT("]"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				]
				.ValueContent()
				.HAlign(HAlign_Fill)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						[
							SNew(SBox)
							.Visibility_Lambda([WeakNode, ElementPinName]
							{
								const UVoxelMetaGraphStructNode* TargetNode = WeakNode.Get();
								if (!ensure(TargetNode))
								{
									return EVisibility::Visible;
								}

								return TargetNode->Struct->ExposedPins.Contains(ElementPinName) ? EVisibility::Collapsed : EVisibility::Visible;
							})
							.MinDesiredWidth(125.f)
							[
								ValueWidget
							]
						]
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						[
							SNew(SBox)
							.Visibility_Lambda([WeakNode, ElementPinName]
							{
								const UVoxelMetaGraphStructNode* TargetNode = WeakNode.Get();
								if (!ensure(TargetNode))
								{
									return EVisibility::Collapsed;
								}

								return TargetNode->Struct->ExposedPins.Contains(ElementPinName) ? EVisibility::Visible : EVisibility::Collapsed;
							})
							.MinDesiredWidth(125.f)
							[
								SNew(SVoxelDetailText)
								.Text(VOXEL_LOCTEXT("Pin is exposed"))
								.ColorAndOpacity(FSlateColor::UseSubduedForeground())
							]
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.Padding(4.0f, 1.0f, 0.0f, 1.0f)
					[
						CreateElementEditButton(ElementPinName)
					]
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					.FillWidth(1.f)
					[
						Customization->CreateExposePinButton(WeakNode, ElementPinName)
					]
				];
			});
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNodePinArrayCustomization::AddNewPin()
{
	UVoxelMetaGraphStructNode* Node = WeakStructNode.Get();
	if (!Node)
	{
		return;
	}

	if (!Node->GetNodeDefinition()->CanAddToCategory(FName(PinName)))
	{
		return;
	}

	const FVoxelTransaction Transaction(Node, "Add Pin to Array");
	Node->GetNodeDefinition()->AddToCategory(FName(PinName));
	Node->ReconstructNode(false);
}

bool FVoxelNodePinArrayCustomization::CanAddNewPin() const
{
	UVoxelMetaGraphStructNode* Node = WeakStructNode.Get();
	if (!Node)
	{
		return false;
	}

	return Node->GetNodeDefinition()->CanAddToCategory(FName(PinName));
}

void FVoxelNodePinArrayCustomization::ClearAllPins()
{
	UVoxelMetaGraphStructNode* Node = WeakStructNode.Get();
	if (!Node)
	{
		return;
	}

	const FVoxelTransaction Transaction(Node, "Clear Pins Array");
	for (int32 Index = 0; Index < Properties.Num(); Index++)
	{
		if (!Node->GetNodeDefinition()->CanRemoveFromCategory(FName(PinName)))
		{
			break;
		}

		Node->GetNodeDefinition()->RemoveFromCategory(FName(PinName));
	}
	Node->ReconstructNode(false);
}

bool FVoxelNodePinArrayCustomization::CanRemovePin() const
{
	UVoxelMetaGraphStructNode* Node = WeakStructNode.Get();
	if (!Node)
	{
		return false;
	}

	return Node->GetNodeDefinition()->CanRemoveFromCategory(FName(PinName));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> FVoxelNodePinArrayCustomization::CreateElementEditButton(const FName EntryPinName)
{
	FMenuBuilder MenuContentBuilder( true, nullptr, nullptr, true );
	{
		FUIAction InsertAction(
			FExecuteAction::CreateSP(this, &FVoxelNodePinArrayCustomization::InsertPinBefore, EntryPinName));
		MenuContentBuilder.AddMenuEntry(VOXEL_LOCTEXT("Insert"), {}, FSlateIcon(), InsertAction);

		FUIAction DeleteAction(
			FExecuteAction::CreateSP(this, &FVoxelNodePinArrayCustomization::DeletePin, EntryPinName),
			FCanExecuteAction::CreateSP(this, &FVoxelNodePinArrayCustomization::CanDeletePin, EntryPinName));
		MenuContentBuilder.AddMenuEntry(VOXEL_LOCTEXT("Delete"), {}, FSlateIcon(), DeleteAction);

		FUIAction DuplicateAction(FExecuteAction::CreateSP(this, &FVoxelNodePinArrayCustomization::DuplicatePin, EntryPinName));
		MenuContentBuilder.AddMenuEntry( VOXEL_LOCTEXT("Duplicate"), {}, FSlateIcon(), DuplicateAction );
	}

	return
		SNew(SComboButton)
		.ComboButtonStyle( FAppStyle::Get(), "SimpleComboButton" )
		.ContentPadding(2)
		.ForegroundColor( FSlateColor::UseForeground() )
		.HasDownArrow(true)
		.MenuContent()
		[
			MenuContentBuilder.MakeWidget()
		];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNodePinArrayCustomization::InsertPinBefore(const FName EntryPinName)
{
	const TSharedPtr<IVoxelNodeDefinition> NodeDefinition = WeakNodeDefinition.Pin();
	if (!NodeDefinition)
	{
		return;
	}

	NodeDefinition->InsertPinBefore(EntryPinName);
}

void FVoxelNodePinArrayCustomization::DeletePin(const FName EntryPinName)
{
	const TSharedPtr<IVoxelNodeDefinition> NodeDefinition = WeakNodeDefinition.Pin();
	if (!NodeDefinition)
	{
		return;
	}

	if (!NodeDefinition->CanRemoveSelectedPin(EntryPinName))
	{
		return;
	}

	NodeDefinition->RemoveSelectedPin(EntryPinName);
}

bool FVoxelNodePinArrayCustomization::CanDeletePin(const FName EntryPinName)
{
	const TSharedPtr<IVoxelNodeDefinition> NodeDefinition = WeakNodeDefinition.Pin();
	if (!NodeDefinition)
	{
		return false;
	}

	return NodeDefinition->CanRemoveSelectedPin(EntryPinName);
}

void FVoxelNodePinArrayCustomization::DuplicatePin(const FName EntryPinName)
{
	const TSharedPtr<IVoxelNodeDefinition> NodeDefinition = WeakNodeDefinition.Pin();
	if (!NodeDefinition)
	{
		return;
	}

	NodeDefinition->DuplicatePin(EntryPinName);
}

END_VOXEL_NAMESPACE(MetaGraph)