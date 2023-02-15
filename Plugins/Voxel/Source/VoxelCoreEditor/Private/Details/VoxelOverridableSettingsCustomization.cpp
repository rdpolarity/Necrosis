// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelEditorMinimal.h"

class FVoxelOverridableSettingsCustomization : public IPropertyTypeCustomization
{
public:
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override
	{
	}

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override
	{
		OverridableParametersHandle = PropertyHandle->GetChildHandleStatic(FVoxelOverridableSettings, OverridableParameters);
		FSimpleDelegate OnSetUpdated = FSimpleDelegate::CreateSP(this, &FVoxelOverridableSettingsCustomization::OnSetUpdated);
		OverridableParametersHandle->AsSet()->SetOnNumElementsChanged(OnSetUpdated);

		const bool bIsOverridableStruct = PropertyHandle->HasMetaData("Override");

		uint32 ChildHandlesCount = 0;
		ensure(PropertyHandle->GetNumChildren(ChildHandlesCount) == FPropertyAccess::Result::Success);

		for (uint32 Index = 0; Index < ChildHandlesCount; Index++)
		{
			TSharedPtr<IPropertyHandle> ChildHandle = PropertyHandle->GetChildHandle(Index);
			if (!ChildHandle)
			{
				continue;
			}

			if (ChildHandle->GetProperty()->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FVoxelOverridableSettings, OverridableParameters))
			{
				continue;
			}

			if (ChildHandle->HasMetaData("ShowOverride"))
			{
				FString ShowOverrideValue = ChildHandle->GetMetaData("ShowOverride");
				if (!bIsOverridableStruct &&
					ShowOverrideValue == "IfOverriden")
				{
					continue;
				}

				if (bIsOverridableStruct &&
					ShowOverrideValue == "IfGlobal")
				{
					continue;
				}

				ensureMsgf(ShowOverrideValue == "IfOverriden" || ShowOverrideValue == "IfGlobal", TEXT("Invalid ShowOverride value: %s"), *ShowOverrideValue);
			}

			IDetailPropertyRow& PropertyRow = ChildBuilder.AddProperty(ChildHandle.ToSharedRef());

			const bool bOverridableHandle = ChildHandle->HasMetaData("Overridable");
			if (!bOverridableHandle ||
				!bIsOverridableStruct)
			{
				continue;
			}

			PropertyRow.EditCondition(MakeAttributeLambda([this, ChildHandle]
			{
				if (!ChildHandle)
				{
					return false;
				}

				TSet<FName>* OverridableParameters = nullptr;
				if (!FVoxelEditorUtilities::GetPropertyValue(OverridableParametersHandle, OverridableParameters))
				{
					return false;
				}

				return OverridableParameters->Contains(ChildHandle->GetProperty()->GetFName());
			}),
			MakeLambdaDelegate([this, ChildHandle](bool bOverride)
			{
				if (!ChildHandle)
				{
					return;
				}

				const FName TargetPropertyName = ChildHandle->GetProperty()->GetFName();
				const TSharedPtr<IPropertyHandleSet> OverridableParametersSet = OverridableParametersHandle->AsSet();

				if (bOverride)
				{
					const int32 TargetElementIndex = GetValueIndex(TargetPropertyName);
					if (TargetElementIndex != -1)
					{
						return;
					}

					NewOverridablePropertyName = TargetPropertyName;
					bAddOverride = true;

					ensure(OverridableParametersSet->AddItem() == FPropertyAccess::Result::Success);
				}
				else
				{
					const int32 TargetElementIndex = GetValueIndex(TargetPropertyName);
					if (TargetElementIndex == -1)
					{
						return;
					}

					bAddOverride = false;

					OverridableParametersSet->DeleteItem(TargetElementIndex);
				}
			}));
		}
	}

	void OnSetUpdated()
	{
		if (!bAddOverride)
		{
			return;
		}

		ON_SCOPE_EXIT
		{
			bAddOverride = false;
		};

		const int32 ElementIndex = GetValueIndex(FName(""));
		if (!ensure(ElementIndex != -1))
		{
			return;
		}

		const TSharedPtr<IPropertyHandle> NewElementHandle = OverridableParametersHandle->GetChildHandle(ElementIndex);
		if (!ensure(NewElementHandle))
		{
			return;
		}

		NewElementHandle->SetValue(NewOverridablePropertyName);
	}

	int32 GetValueIndex(const FName LookupElement) const
	{
		if (!ensure(OverridableParametersHandle))
		{
			return -1;
		}

		uint32 ElementsCount = 0;
		ensure(OverridableParametersHandle->AsSet()->GetNumElements(ElementsCount) == FPropertyAccess::Result::Success);

		for (uint32 ElementIndex = 0; ElementIndex < ElementsCount; ElementIndex++)
		{
			const TSharedPtr<IPropertyHandle> ElementHandle = OverridableParametersHandle->GetChildHandle(ElementIndex);

			FName Value;
			if (ElementHandle->GetValue(Value) != FPropertyAccess::Result::Success)
			{
				continue;
			}

			if (Value == LookupElement)
			{
				return ElementIndex;
			}
		}

		return -1;
	}

private:
	bool bAddOverride = false;
	FName NewOverridablePropertyName;
	TSharedPtr<IPropertyHandle> OverridableParametersHandle;
};

class FVoxelOverridableSettingsIdentifier : public IPropertyTypeIdentifier
{
	virtual bool IsPropertyTypeCustomized(const IPropertyHandle& PropertyHandle) const override
	{
		return !PropertyHandle.HasMetaData("NoOverrideCustomization");
	}
};

DEFINE_VOXEL_STRUCT_LAYOUT_RECURSIVE_IDENTIFIER(FVoxelOverridableSettings, FVoxelOverridableSettingsCustomization, FVoxelOverridableSettingsIdentifier);