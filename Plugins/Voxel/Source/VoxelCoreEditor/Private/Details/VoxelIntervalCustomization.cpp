// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/NumericUnitTypeInterface.inl"

template <typename NumericType>
class FVoxelIntervalCustomization : public IPropertyTypeCustomization
{
public:
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override
	{
		PrepareSettings(StructPropertyHandle);
		
		HeaderRow.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(251.0f)
		.MaxDesiredWidth(251.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.f, 0.f, 3.f, 0.f))
			.AutoWidth()
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(VOXEL_LOCTEXT("Min"))
			]
			+ SHorizontalBox::Slot()
			.Padding(FMargin(0.f, 0.f, 3.f, 0.f))
			.VAlign(VAlign_Center)
			[
				SNew(SNumericEntryBox<NumericType>)
				.Value(this, &FVoxelIntervalCustomization<NumericType>::OnGetValue, true)
				.MinValue(MinAllowedValue)
				.MinSliderValue(MinAllowedSliderValue)
				.MaxValue_Lambda([this]
				{
					if (bClampToMinMaxLimits)
					{
						return OnGetValue(false);
					}

					return MaxAllowedValue;
				})
				.MaxSliderValue_Lambda([this]
				{
					if (bClampToMinMaxLimits)
					{
						return OnGetValue(false);
					}

					return MaxAllowedSliderValue;
				})
				.OnValueCommitted(this, &FVoxelIntervalCustomization<NumericType>::OnValueCommitted, true)
				.OnValueChanged(this, &FVoxelIntervalCustomization<NumericType>::OnValueChanged, true)
				.OnBeginSliderMovement(this, &FVoxelIntervalCustomization<NumericType>::OnBeginSliderMovement)
				.OnEndSliderMovement(this, &FVoxelIntervalCustomization<NumericType>::OnEndSliderMovement)
				.UndeterminedString(VOXEL_LOCTEXT("Multiple Values"))
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.AllowSpin(true)
				.IsEnabled_Lambda([this]
				{
					return MinValueHandle ? !MinValueHandle->IsEditConst() : false;
				})
				.TypeInterface(TypeInterface)
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.Padding(2.f, 0.f)
			.AutoWidth()
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(VOXEL_LOCTEXT("Max"))
			]
			+ SHorizontalBox::Slot()
			.Padding(FMargin(0.f, 0.f, 3.f, 0.f))
			.VAlign(VAlign_Center)
			[
				SNew(SNumericEntryBox<NumericType>)
				.Value(this, &FVoxelIntervalCustomization<NumericType>::OnGetValue, false)
				.MinValue_Lambda([this]
				{
					if (bClampToMinMaxLimits)
					{
						return OnGetValue(true);
					}

					return MinAllowedValue;
				})
				.MinSliderValue_Lambda([this]
				{
					if (bClampToMinMaxLimits)
					{
						return OnGetValue(true);
					}

					return MinAllowedSliderValue;
				})
				.MaxValue(MaxAllowedValue)
				.MaxSliderValue(MaxAllowedSliderValue)
				.OnValueCommitted(this, &FVoxelIntervalCustomization<NumericType>::OnValueCommitted, false)
				.OnValueChanged(this, &FVoxelIntervalCustomization<NumericType>::OnValueChanged, false)
				.OnBeginSliderMovement(this, &FVoxelIntervalCustomization<NumericType>::OnBeginSliderMovement)
				.OnEndSliderMovement(this, &FVoxelIntervalCustomization<NumericType>::OnEndSliderMovement)
				.UndeterminedString(VOXEL_LOCTEXT("Multiple Values"))
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.AllowSpin(true)
				.IsEnabled_Lambda([this]
				{
					return MaxValueHandle ? !MaxValueHandle->IsEditConst() : false;
				})
				.TypeInterface(TypeInterface)
			]
		];
	}

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override
	{
		
	}

private:
	void OnValueCommitted(NumericType NewValue, ETextCommit::Type CommitType, bool bMin)
	{
		SetValue(NewValue, bMin, EPropertyValueSetFlags::DefaultFlags);
	}

	void OnValueChanged(NumericType NewValue, bool bMin)
	{
		SetValue(NewValue, bMin, EPropertyValueSetFlags::InteractiveChange);
	}

	void OnBeginSliderMovement()
	{
		GEditor->BeginTransaction(VOXEL_LOCTEXT("Set Interval Property"));
	}

	void OnEndSliderMovement(NumericType)
	{
		GEditor->EndTransaction();
	}

	void SetValue(NumericType NewValue, bool bMin, EPropertyValueSetFlags::Type Flags)
	{
		const TSharedPtr<IPropertyHandle> Handle = bMin ? MinValueHandle : MaxValueHandle;
		const TSharedPtr<IPropertyHandle> OtherHandle = bMin ? MaxValueHandle : MinValueHandle;

		const TOptional<NumericType> OtherValue = OnGetValue(!bMin);
		bool bOutOfRange = false;
		if (OtherValue.IsSet())
		{
			if (bMin && NewValue > OtherValue.GetValue())
			{
				bOutOfRange = true;
			}
			else if (!bMin && NewValue < OtherValue.GetValue())
			{
				bOutOfRange = true;
			}
		}

		if (!bOutOfRange || bAllowInvertedInterval)
		{
			if (Flags == EPropertyValueSetFlags::InteractiveChange)
			{
				ensure(Handle->SetValue(NewValue, Flags) == FPropertyAccess::Success);

				if (OtherValue.IsSet())
				{
					ensure(OtherHandle->SetValue(OtherValue.GetValue(), Flags) == FPropertyAccess::Success);
				}
			}
			else
			{
				if (OtherValue.IsSet())
				{
					ensure(OtherHandle->SetValue(OtherValue.GetValue(), Flags) == FPropertyAccess::Success);
				}

				ensure(Handle->SetValue(NewValue, Flags) == FPropertyAccess::Success);
			}
		}
		else if (!bClampToMinMaxLimits)
		{
			if (Flags == EPropertyValueSetFlags::InteractiveChange)
			{
				ensure(Handle->SetValue(NewValue, Flags) == FPropertyAccess::Success);
				ensure(OtherHandle->SetValue(NewValue, Flags) == FPropertyAccess::Success);
			}
			else
			{
				ensure(OtherHandle->SetValue(NewValue, Flags) == FPropertyAccess::Success);
				ensure(Handle->SetValue(NewValue, Flags) == FPropertyAccess::Success);
			}
		}
	}

	void PrepareSettings(TSharedRef<IPropertyHandle> StructPropertyHandle)
	{
		MinValueHandle = StructPropertyHandle->GetChildHandle(TEXT("Min"));
		MaxValueHandle = StructPropertyHandle->GetChildHandle(TEXT("Max"));
		check(MinValueHandle.IsValid());
		check(MaxValueHandle.IsValid());

		const FProperty* Property = StructPropertyHandle->GetProperty();
		check(Property);

		const FString& MetaUIMinString = Property->GetMetaData(TEXT("UIMin"));
		const FString& MetaUIMaxString = Property->GetMetaData(TEXT("UIMax"));
		const FString& MetaClampMinString = Property->GetMetaData(TEXT("ClampMin"));
		const FString& MetaClampMaxString = Property->GetMetaData(TEXT("ClampMax"));
		const FString& UIMinString = MetaUIMinString.Len() ? MetaUIMinString : MetaClampMinString;
		const FString& UIMaxString = MetaUIMaxString.Len() ? MetaUIMaxString : MetaClampMaxString;
		const FString& MetaUnits = Property->GetMetaData(TEXT("Units"));

		NumericType ClampMin = std::numeric_limits<NumericType>::lowest();
		NumericType ClampMax = std::numeric_limits<NumericType>::max();
		TTypeFromString<NumericType>::FromString(ClampMin, *MetaClampMinString);
		TTypeFromString<NumericType>::FromString(ClampMax, *MetaClampMaxString);

		NumericType UIMin = std::numeric_limits<NumericType>::lowest();
		NumericType UIMax = std::numeric_limits<NumericType>::max();
		TTypeFromString<NumericType>::FromString(UIMin, *UIMinString);
		TTypeFromString<NumericType>::FromString(UIMax, *UIMaxString);

		const NumericType ActualUIMin = FMath::Max(UIMin, ClampMin);
		const NumericType ActualUIMax = FMath::Min(UIMax, ClampMax);

		MinAllowedValue = MetaClampMinString.Len() ? ClampMin : TOptional<NumericType>();
		MaxAllowedValue = MetaClampMaxString.Len() ? ClampMax : TOptional<NumericType>();
		MinAllowedSliderValue = (UIMinString.Len()) ? ActualUIMin : TOptional<NumericType>();
		MaxAllowedSliderValue = (UIMaxString.Len()) ? ActualUIMax : TOptional<NumericType>();

		bAllowInvertedInterval = Property->HasMetaData(TEXT("AllowInvertedInterval"));
		bClampToMinMaxLimits = Property->HasMetaData(TEXT("ClampToMinMaxLimits"));

		TOptional<EUnit> PropertyUnits = EUnit::Unspecified;
		if (!MetaUnits.IsEmpty())
		{
			PropertyUnits = FUnitConversion::UnitFromString(*MetaUnits);
		}

		TypeInterface = MakeShared<TNumericUnitTypeInterface<NumericType>>(PropertyUnits.GetValue());
	}

	TOptional<NumericType> OnGetValue(bool bMin) const
	{
		NumericType NumericVal;
		if ((bMin ? MinValueHandle : MaxValueHandle)->GetValue(NumericVal) == FPropertyAccess::Success)
		{
			return NumericVal;
		}

		return TOptional<NumericType>();
	}

private:
	TSharedPtr<TNumericUnitTypeInterface<NumericType>> TypeInterface;

	TSharedPtr<IPropertyHandle> MinValueHandle;
	TSharedPtr<IPropertyHandle> MaxValueHandle;

	TOptional<NumericType> MinAllowedValue;
	TOptional<NumericType> MaxAllowedValue;
	TOptional<NumericType> MinAllowedSliderValue;
	TOptional<NumericType> MaxAllowedSliderValue;

	bool bAllowInvertedInterval = false;
	bool bClampToMinMaxLimits = false;
};

DEFINE_VOXEL_STRUCT_LAYOUT(FVoxelInt32Interval, FVoxelIntervalCustomization<int32>);
DEFINE_VOXEL_STRUCT_LAYOUT(FVoxelFloatInterval, FVoxelIntervalCustomization<float>);