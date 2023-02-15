// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "SVoxelMetaGraphPinEnum.h"
#include "SGraphPinComboBox.h"
#include "VoxelPinType.h"

void SVoxelMetaGraphPinEnum::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

TSharedRef<SWidget> SVoxelMetaGraphPinEnum::GetDefaultValueWidget()
{
	const FVoxelPinType Type = FVoxelPinType(GraphPinObj->PinType).GetExposedType();
	if (!ensure(Type.IsEnum()))
	{
		return SNullWidget::NullWidget;
	}

	const UEnum* Enum = Type.GetEnum();
	if (!ensure(Enum))
	{
		return SNullWidget::NullWidget;
	}

	TArray<TSharedPtr<int32>> Items;

	// NumEnums() - 1 is _MAX
	for (int32 Index = 0; Index < Enum->NumEnums() - 1; Index++)
	{
		if (Enum->HasMetaData(TEXT("Hidden"), Index))
		{
			continue;
		}

		Items.Add(MakeSharedCopy(Index));
	}

	return SNew(SPinComboBox)
		.ComboItemList(Items)
		.VisibleText_Lambda([=]
		{
			const FString SelectedValue = GraphPinObj->GetDefaultAsString();
	
			// NumEnums() - 1 is _MAX
			for (int32 Index = 0; Index < Enum->NumEnums() - 1; Index++)
			{
				if (Enum->HasMetaData(TEXT("Hidden"), Index))
				{
					continue;
				}

				if (SelectedValue == Enum->GetNameStringByIndex(Index))
				{
					return Enum->GetDisplayNameTextByIndex(Index).ToString();
				}
			}

			return SelectedValue + " (INVALID)";
		})
		.OnSelectionChanged_Lambda([=](const TSharedPtr<int32>& NewSelection, ESelectInfo::Type)
		{
			FString NewValue;
			if (NewSelection.IsValid())
			{
				NewValue = Enum->GetNameStringByIndex(*NewSelection);
			}
			else
			{
				NewValue = FName(NAME_None).ToString();
			}

			if (GraphPinObj->GetDefaultAsString() == NewValue)
			{
				return;
			}
			
			const FVoxelTransaction Transaction(GraphPinObj, "Change Enum Pin Value");
			GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, NewValue);
		})
		.IsEnabled(this, &SGraphPin::GetDefaultValueIsEditable)
		.Visibility(this, &SGraphPin::GetDefaultValueVisibility)
		.OnGetDisplayName_Lambda([=](const int32 Index)
		{
			return Enum->GetDisplayNameTextByIndex(Index);
		})
		.OnGetTooltip_Lambda([=](const int32 Index)
		{
			return Enum->GetToolTipTextByIndex(Index);
		});
}