// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphPinValueCustomization.h"

#include "VoxelPinType.h"
#include "VoxelPinValue.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

void FPinValueCustomization::Tick()
{
	// Tricky: can tick once after the property is gone due to SListPanel being delayed

	const double Time = FPlatformTime::Seconds();
	if (Time < LastSyncEditableInstanceFromSourceSeconds + 0.1)
	{
		return;
	}
	LastSyncEditableInstanceFromSourceSeconds = Time;
	
	for (auto& It : StructOnScopes)
	{
		SyncEditableInstanceFromSource(It.Key);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FPinValueCustomization::AddPinValueCustomization(
	IDetailCategoryBuilder& CategoryBuilder,
	const TSharedRef<IPropertyHandle>& ValueHandle,
	const FVoxelPinValue& Value,
	const FString& RowName,
	const FString& Tooltip,
	const TFunction<void(FDetailWidgetRow& Row, const TSharedRef<IPropertyHandle>& Handle, const TSharedRef<SWidget>& NameWidget, const TSharedRef<SWidget>& ValueWidget)>& SetupRowLambda)
{
	AddPinValueCustomization_Impl(
		[&](const FText& FilterString) -> FDetailWidgetRow&
		{
			return CategoryBuilder.AddCustomRow(FilterString);
		},
		[&](const TSharedRef<FStructOnScope>& StructOnScope)
		{
			return CategoryBuilder.AddExternalStructure(StructOnScope);
		},
		FVoxelEditorUtilities::MakeRefreshDelegate(CategoryBuilder),
		ValueHandle,
		Value,
		RowName,
		Tooltip,
		SetupRowLambda
	);
}

void FPinValueCustomization::AddPinValueCustomization(
	IDetailChildrenBuilder& ChildrenBuilder,
	const TSharedRef<IPropertyHandle>& ValueHandle,
	const FVoxelPinValue& Value,
	const FString& RowName,
	const FString& Tooltip,
	const TFunction<void(FDetailWidgetRow& Row, const TSharedRef<IPropertyHandle>& Handle, const TSharedRef<SWidget>& NameWidget, const TSharedRef<SWidget>& ValueWidget)>& SetupRowLambda)
{
	AddPinValueCustomization_Impl(
		[&](const FText& FilterString) -> FDetailWidgetRow&
		{
			return ChildrenBuilder.AddCustomRow(FilterString);
		},
		[&](const TSharedRef<FStructOnScope>& StructOnScope)
		{
			return ChildrenBuilder.AddExternalStructure(StructOnScope);
		},
		FVoxelEditorUtilities::MakeRefreshDelegate(ChildrenBuilder),
		ValueHandle,
		Value,
		RowName,
		Tooltip,
		SetupRowLambda
	);
}

void FPinValueCustomization::AddPinValueCustomization_Impl(
	const TFunction<FDetailWidgetRow&(const FText& FilterString)>& CreateCustomRow,
	const TFunction<IDetailPropertyRow*(const TSharedRef<FStructOnScope>& StructOnScope)>& AddExternalStructure,
	const FSimpleDelegate& RefreshDelegate,
	const TSharedRef<IPropertyHandle>& ValueHandle,
	const FVoxelPinValue& Value,
	const FString& RowName,
	const FString& Tooltip,
	const TFunction<void(FDetailWidgetRow& Row, const TSharedRef<IPropertyHandle>& Handle, const TSharedRef<SWidget>& NameWidget, const TSharedRef<SWidget>& ValueWidget)>& SetupRowLambda)
{
	
	const auto SetupTooltip = [&](const TSharedRef<SWidget>& NameWidget)
	{
		NameWidget->SetToolTipText(FText::FromString(Tooltip));
		return NameWidget;
	};

	const FVoxelPinType Type = Value.GetType();
	if (Type.Is<bool>())
	{
		const TSharedRef<IPropertyHandle> Handle = ValueHandle->GetChildHandleStatic(FVoxelPinValue, bBool);
		Handle->SetPropertyDisplayName(FText::FromString(RowName));
		SetupRowLambda(CreateCustomRow(FText::FromString(RowName)), Handle, SetupTooltip(Handle->CreatePropertyNameWidget()), Handle->CreatePropertyValueWidget());
	}
	else if (Type.Is<uint8>())
	{
		const TSharedRef<IPropertyHandle> Handle = ValueHandle->GetChildHandleStatic(FVoxelPinValue, Byte);
		Handle->SetPropertyDisplayName(FText::FromString(RowName));
		SetupRowLambda(CreateCustomRow(FText::FromString(RowName)), Handle, SetupTooltip(Handle->CreatePropertyNameWidget()), Handle->CreatePropertyValueWidget());
	}
	else if (Type.Is<float>())
	{
		const TSharedRef<IPropertyHandle> Handle = ValueHandle->GetChildHandleStatic(FVoxelPinValue, Float);
		Handle->SetPropertyDisplayName(FText::FromString(RowName));
		SetupRowLambda(CreateCustomRow(FText::FromString(RowName)), Handle, SetupTooltip(Handle->CreatePropertyNameWidget()), Handle->CreatePropertyValueWidget());
	}
	else if (Type.Is<int32>())
	{
		const TSharedRef<IPropertyHandle> Handle = ValueHandle->GetChildHandleStatic(FVoxelPinValue, Int32);
		Handle->SetPropertyDisplayName(FText::FromString(RowName));
		SetupRowLambda(CreateCustomRow(FText::FromString(RowName)), Handle, SetupTooltip(Handle->CreatePropertyNameWidget()), Handle->CreatePropertyValueWidget());
	}
	else if (Type.Is<FName>())
	{
		const TSharedRef<IPropertyHandle> Handle = ValueHandle->GetChildHandleStatic(FVoxelPinValue, Name);
		Handle->SetPropertyDisplayName(FText::FromString(RowName));
		SetupRowLambda(CreateCustomRow(FText::FromString(RowName)), Handle, SetupTooltip(Handle->CreatePropertyNameWidget()), Handle->CreatePropertyValueWidget());
	}
	else if (Type.IsEnum())
	{
		const TSharedRef<IPropertyHandle> Handle = ValueHandle->GetChildHandleStatic(FVoxelPinValue, Enum);
		Handle->SetPropertyDisplayName(FText::FromString(RowName));

		const UEnum* Enum = Type.GetEnum();

		const TSharedRef<SWidget> ValueWidget = SNew(SBox)
		.MinDesiredWidth(125.f)
		[
			SNew(SVoxelDetailComboBox<int32>)
			.RefreshOptionsOnChange()
			.RefreshDelegate(RefreshDelegate)
			.Options_Lambda([=]
			{
				TArray<int32> Enumerators;
				for (int32 Index = 0; Index < Enum->NumEnums() - 1; Index++)
				{
					if (Enum->HasMetaData(TEXT("Hidden"), Index) ||
						Enum->HasMetaData(TEXT("Spacer"), Index))
					{
						continue;
					}
					Enumerators.Add(Index);
				}

				return Enumerators;
			})
			.CurrentOption(Enum->GetIndexByValue(Value.GetEnum()))
			.OptionText(MakeLambdaDelegate([=](int32 Index)
			{
				FString EnumDisplayName = Enum->GetDisplayNameTextByIndex(Index).ToString();
				if (EnumDisplayName.Len() == 0)
				{
					return Enum->GetNameStringByIndex(Index);
				}

				return EnumDisplayName;
			}))
			.OnSelection_Lambda([Handle](int32 NewValue)
			{
				Handle->SetValue(NewValue);
			})
		];

		SetupRowLambda(CreateCustomRow(FText::FromString(RowName)), Handle, SetupTooltip(Handle->CreatePropertyNameWidget()), ValueWidget);
	}
	else if (Type.IsStruct())
	{
		const TSharedRef<IPropertyHandle> StructHandle = ValueHandle->GetChildHandleStatic(FVoxelPinValue, Struct);
		const TSharedRef<FStructOnScope> StructOnScope = MakeShared<FStructOnScope>(Value.GetStruct().GetScriptStruct());
		AddStructOnScope(StructHandle, StructOnScope);

		IDetailPropertyRow* Row = AddExternalStructure(StructOnScope);
		if (!ensure(Row))
		{
			return;
		}

		Row->GetPropertyHandle()->SetPropertyDisplayName(FText::FromString(RowName));

		TSharedPtr<SWidget> NameWidget;
		TSharedPtr<SWidget> ValueWidget;
		Row->GetDefaultWidgets(NameWidget, ValueWidget);

		SetupRowLambda(Row->CustomWidget(true), StructHandle, SetupTooltip(NameWidget.ToSharedRef()), ValueWidget.ToSharedRef());

		BindStructPropertyValueChanges(StructHandle, FVoxelEditorUtilities::GetChildHandlesRecursive(Row->GetPropertyHandle()));
	}
	else if (Type.IsObject())
	{
		const TSharedRef<IPropertyHandle> Handle = ValueHandle->GetChildHandleStatic(FVoxelPinValue, Object);

		Handle->GetProperty()->SetMetaData("AllowedClasses", Type.GetClass()->GetName());

		SetupRowLambda(CreateCustomRow(FText::FromString(RowName)), Handle, SetupTooltip(Handle->CreatePropertyNameWidget()), Handle->CreatePropertyValueWidget());

		Handle->GetProperty()->RemoveMetaData("AllowedClasses");
	}
	else
	{
		ensure(false);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FPinValueCustomization::AddStructOnScope(const TSharedRef<IPropertyHandle>& StructHandle, const TSharedRef<FStructOnScope>& StructOnScope)
{
	StructOnScopes.Add(StructHandle, StructOnScope);

	SyncEditableInstanceFromSource(StructHandle);
}

void FPinValueCustomization::BindStructPropertyValueChanges(const TSharedRef<IPropertyHandle>& StructHandle, const TArray<TSharedPtr<IPropertyHandle>>& ChildHandles)
{
	const TSharedPtr<FPinValueCustomization> SelfSharedPtr = GetSelfSharedPtr();
	if (!ensure(SelfSharedPtr))
	{
		return;
	}

	const FSimpleDelegate OnStructValuePreChangeDelegate = FSimpleDelegate::CreateSP(SelfSharedPtr.ToSharedRef(), &FPinValueCustomization::OnStructValuePreChange, MakeWeakPtr(StructHandle));
	const FSimpleDelegate OnStructValuePostChangeDelegate = FSimpleDelegate::CreateSP(SelfSharedPtr.ToSharedRef(), &FPinValueCustomization::OnStructValuePostChange, MakeWeakPtr(StructHandle));

	for (const TSharedPtr<IPropertyHandle>& ChildHandle : ChildHandles)
	{
		ChildHandle->SetOnPropertyValuePreChange(OnStructValuePreChangeDelegate);
		ChildHandle->SetOnChildPropertyValuePreChange(OnStructValuePreChangeDelegate);
		ChildHandle->SetOnPropertyValueChanged(OnStructValuePostChangeDelegate);
		ChildHandle->SetOnChildPropertyValueChanged(OnStructValuePostChangeDelegate);
	}
}

void FPinValueCustomization::SyncAllEditableInstancesFromSource()
{
	for (auto& It : StructOnScopes)
	{
		SyncEditableInstanceFromSource(It.Key);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FPinValueCustomization::OnStructValuePreChange(TWeakPtr<IPropertyHandle> WeakStructProperty)
{
	const TSharedPtr<IPropertyHandle> StructProperty = WeakStructProperty.Pin();
	if (!StructProperty ||
		!StructProperty->IsValidHandle())
	{
		return;
	}

	StructProperty->NotifyPreChange();
}

void FPinValueCustomization::OnStructValuePostChange(TWeakPtr<IPropertyHandle> WeakStructProperty) const
{
	const TSharedPtr<IPropertyHandle> StructProperty = WeakStructProperty.Pin();
	if (!StructProperty ||
		!StructProperty->IsValidHandle())
	{
		return;
	}

	SyncEditableInstanceToSource(StructProperty);

	StructProperty->NotifyPostChange(EPropertyChangeType::ValueSet);
	StructProperty->NotifyFinishedChangingProperties();
}

void FPinValueCustomization::SyncEditableInstanceToSource(const TSharedPtr<IPropertyHandle>& StructProperty) const
{
	if (!StructProperty ||
		!StructProperty->IsValidHandle())
	{
		return;
	}

	const TSharedPtr<FStructOnScope>& StructOnScope = StructOnScopes[StructProperty];

	FVoxelEditorUtilities::ForeachData<FVoxelInstancedStruct>(StructProperty, [this, StructOnScope](FVoxelInstancedStruct& InstancedStruct)
	{
		if (InstancedStruct.GetScriptStruct() != StructOnScope->GetStruct())
		{
			return;
		}

		InstancedStruct.GetScriptStruct()->CopyScriptStruct(InstancedStruct.GetStructMemory(), StructOnScope->GetStructMemory());
	});
}

void FPinValueCustomization::SyncEditableInstanceFromSource(const TSharedPtr<IPropertyHandle>& StructProperty) const
{
	if (!StructProperty ||
		!StructProperty->IsValidHandle())
	{
		return;
	}

	const TSharedPtr<FStructOnScope>& StructOnScope = StructOnScopes[StructProperty];

	FVoxelEditorUtilities::ForeachData<FVoxelInstancedStruct>(StructProperty, [this, StructOnScope](FVoxelInstancedStruct& InstancedStruct)
	{
		if (InstancedStruct.GetScriptStruct() != StructOnScope->GetStruct())
		{
			return;
		}

		InstancedStruct.GetScriptStruct()->CopyScriptStruct(StructOnScope->GetStructMemory(), InstancedStruct.GetStructMemory());
	});
}

END_VOXEL_NAMESPACE(MetaGraph)