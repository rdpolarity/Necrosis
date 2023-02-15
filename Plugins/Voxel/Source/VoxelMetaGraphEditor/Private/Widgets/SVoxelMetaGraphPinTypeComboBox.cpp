// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "SVoxelMetaGraphPinTypeComboBox.h"
#include "SVoxelMetaGraphPinTypeSelector.h"
#include "VoxelNode.h"
#include "VoxelMetaGraphVisuals.h"
#include "Styling/SlateIconFinder.h"
#include "SListViewSelectorDropdownMenu.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

void SPinTypeComboBox::Construct(const FArguments& InArgs)
{
	PinTypesAttribute = InArgs._PinTypes;
	CurrentType = InArgs._CurrentType;
	CurrentInnerType = CurrentType.GetInnerType();
	ReadOnly = InArgs._ReadOnly;

	OnTypeChanged = InArgs._OnTypeChanged;
	ensure(OnTypeChanged.IsBound());

	SAssignNew(MainIcon, SImage)
	.Image(GetIcon(CurrentType))
	.ColorAndOpacity(GetColor(CurrentInnerType));

	SAssignNew(MainTextBlock, STextBlock)
	.Text(FText::FromString(CurrentInnerType.ToString()))
	.Font(FVoxelEditorUtilities::Font())
	.ColorAndOpacity(FSlateColor::UseForeground());

	TSharedPtr<SHorizontalBox> SelectorBox;
	ChildSlot
	[
		SNew(SWidgetSwitcher)
		.WidgetIndex_Lambda([this]
		{
			return ReadOnly.Get() ? 1 : 0;
		})
		+ SWidgetSwitcher::Slot()
		.Padding(InArgs._DetailsWindow ? FMargin(0) : FMargin(-6.0f, 0.0f,0.0f,0.0f))
		[
			SAssignNew(SelectorBox, SHorizontalBox)
			.Clipping(EWidgetClipping::ClipToBoundsAlways)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			[
				SNew(SBox)
				.WidthOverride(InArgs._DetailsWindow ? 125.f : FOptionalSize())
				[
					SAssignNew(TypeComboButton, SComboButton)
					.ComboButtonStyle(FAppStyle::Get(), "ComboButton")
					.OnGetMenuContent(this, &SPinTypeComboBox::GetMenuContent)
					.ContentPadding(0)
					.ForegroundColor(FSlateColor::UseForeground())
					.ButtonContent()
					[
						SNew(SHorizontalBox)
						.Clipping(EWidgetClipping::ClipToBoundsAlways)
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Left)
						.Padding(0.f, 0.f, 2.f, 0.f)
						.AutoWidth()
						[
							MainIcon.ToSharedRef()
						]
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Left)
						.Padding(2.f, 0.f, 0.f, 0.f)
						.AutoWidth()
						[
							MainTextBlock.ToSharedRef()
						]
					]
				]
			]
		]
		+ SWidgetSwitcher::Slot()
		[
			SNew(SHorizontalBox)
			.Clipping(EWidgetClipping::OnDemand)
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			.Padding(2.f, InArgs._WithContainerType ? 4.f : 3.f)
			.AutoWidth()
			[
				MainIcon.ToSharedRef()
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			.Padding(2.f, 0.f, 0.f, 0.f)
			.AutoWidth()
			[
				MainTextBlock.ToSharedRef()
			]
		]
	];

	if (!InArgs._WithContainerType)
	{
		return;
	}

	SelectorBox->AddSlot()
	.AutoWidth()
	.VAlign(VAlign_Center)
	.HAlign(HAlign_Center)
	.Padding(2.f)
	[
		SNew(SComboButton)
		.ComboButtonStyle(FAppStyle::Get(),"BlueprintEditor.CompactVariableTypeSelector")
		.MenuPlacement(MenuPlacement_ComboBoxRight)
		.OnGetMenuContent(this, &SPinTypeComboBox::GetPinContainerTypeMenuContent)
		.ContentPadding(0.f)
		.ButtonContent()
		[
			MainIcon.ToSharedRef()
		]
	];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SPinTypeComboBox::SetType(const FVoxelPinType& NewPinType)
{
	CurrentType = NewPinType;
	CurrentInnerType = CurrentType.GetInnerType();

	MainIcon->SetImage(GetIcon(CurrentType));
	MainIcon->SetColorAndOpacity(GetColor(CurrentInnerType));

	MainTextBlock->SetText(FText::FromString(CurrentInnerType.ToString()));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> SPinTypeComboBox::GetMenuContent()
{
	if (MenuContent)
	{
		PinTypeSelector->ClearSelection();

		return MenuContent.ToSharedRef();
	}

	SAssignNew(MenuContent, SMenuOwner)
	[
		SAssignNew(PinTypeSelector, SPinTypeSelector)
		.PinTypes(PinTypesAttribute)
		.OnTypeChanged_Lambda([this](FVoxelPinType NewType)
		{
			if (CurrentType.IsBuffer())
			{
				NewType = NewType.GetBufferType();
			}
			OnTypeChanged.ExecuteIfBound(NewType);
		})
		.OnCloseMenu_Lambda([this]
		{
			MenuContent->CloseSummonedMenus();
		})
	];

	TypeComboButton->SetMenuContentWidgetToFocus(PinTypeSelector->GetWidgetToFocus());

	return MenuContent.ToSharedRef();
}

TSharedRef<SWidget> SPinTypeComboBox::GetPinContainerTypeMenuContent() const
{
	struct FData
	{
		FText Label;
		const FSlateBrush* Brush;
	};

	static const TMap<EVoxelPinContainerType, FData> Containers
	{
		{
			EVoxelPinContainerType::None,
			{
				VOXEL_LOCTEXT("Single"),
				FAppStyle::Get().GetBrush("Kismet.VariableList.TypeIcon")
			}
		},
		{
			EVoxelPinContainerType::Buffer,
			{
				VOXEL_LOCTEXT("Buffer"),
				FVoxelEditorStyle::GetBrush("Pill.Buffer")
			}
		},
	};

	FMenuBuilder MenuBuilder(true, nullptr);
	for (auto& It : Containers)
	{
		FUIAction Action;
		Action.ExecuteAction.BindSP(this, &SPinTypeComboBox::OnContainerTypeSelectionChanged, It.Key);
		Action.CanExecuteAction.BindSP(this, &SPinTypeComboBox::IsValidContainerType, It.Key);

		MenuBuilder.AddMenuEntry(Action,
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(It.Value.Brush)
				.ColorAndOpacity(GetColor(CurrentInnerType))
			]
			+ SHorizontalBox::Slot()
			.Padding(4.0f,2.0f)
			[
				SNew(STextBlock)
				.Text(It.Value.Label)
			]);
	}

	return MenuBuilder.MakeWidget();
}

bool SPinTypeComboBox::IsValidContainerType(EVoxelPinContainerType ContainerType) const
{
	switch (ContainerType)
	{
	default: check(false);
	case EVoxelPinContainerType::None: return true;
	case EVoxelPinContainerType::Buffer: return
		CurrentType.GetBufferType() != CurrentType ||
		CurrentType.GetInnerType() != CurrentType;
	}
}

void SPinTypeComboBox::OnContainerTypeSelectionChanged(EVoxelPinContainerType ContainerType) const
{
	switch (ContainerType)
	{
	default: check(false);
	case EVoxelPinContainerType::None: OnTypeChanged.ExecuteIfBound(CurrentType.GetInnerType()); break;
	case EVoxelPinContainerType::Buffer: OnTypeChanged.ExecuteIfBound(CurrentType.GetBufferType()); break;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

const FSlateBrush* SPinTypeComboBox::GetIcon(const FVoxelPinType& PinType) const
{
	return FVoxelMetaGraphVisuals::GetPinIcon(PinType).GetIcon();
}

FLinearColor SPinTypeComboBox::GetColor(const FVoxelPinType& PinType) const
{
	return FVoxelMetaGraphVisuals::GetPinColor(PinType);
}

END_VOXEL_NAMESPACE(MetaGraph)