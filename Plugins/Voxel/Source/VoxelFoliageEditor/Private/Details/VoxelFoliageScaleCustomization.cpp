// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "VoxelFoliageInstanceTemplate.h"

class FVoxelFoliageScaleSettingsCustomization : public IPropertyTypeCustomization
{
public:
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override
	{
		if (PropertyHandle->HasMetaData("Override"))
		{
			HeaderRow
			.NameContent()
			[
				PropertyHandle->CreatePropertyNameWidget()
			];
		}
	}

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override
	{
		const TSharedRef<IPropertyHandle> TypeHandle = PropertyHandle->GetChildHandleStatic(FVoxelFoliageScaleSettings, Scaling);
		const TSharedRef<IPropertyHandle> ScaleXHandle = PropertyHandle->GetChildHandleStatic(FVoxelFoliageScaleSettings, ScaleX);
		const TSharedRef<IPropertyHandle> ScaleYHandle = PropertyHandle->GetChildHandleStatic(FVoxelFoliageScaleSettings, ScaleY);
		const TSharedRef<IPropertyHandle> ScaleZHandle = PropertyHandle->GetChildHandleStatic(FVoxelFoliageScaleSettings, ScaleZ);

		TypeHandle->SetOnPropertyValueChanged(FVoxelEditorUtilities::MakeRefreshDelegate(CustomizationUtils));
	
		ChildBuilder.AddProperty(TypeHandle);

		FVoxelFoliageScaleSettings* FoliageScaleSettings = nullptr;
		if (!FVoxelEditorUtilities::GetPropertyValue(PropertyHandle, FoliageScaleSettings))
		{
			return;
		}

		if (FoliageScaleSettings->Scaling == EVoxelFoliageScaling::Uniform)
		{
			ChildBuilder.AddProperty(ScaleXHandle).DisplayName(VOXEL_LOCTEXT("Scale"));
		}
		else if (FoliageScaleSettings->Scaling == EVoxelFoliageScaling::Free)
		{
			ChildBuilder.AddProperty(ScaleXHandle);
			ChildBuilder.AddProperty(ScaleYHandle);
			ChildBuilder.AddProperty(ScaleZHandle);
		}
		else
		{
			ensure(FoliageScaleSettings->Scaling == EVoxelFoliageScaling::LockXY);

			ChildBuilder.AddProperty(ScaleXHandle).DisplayName(VOXEL_LOCTEXT("Scale XY"));
			ChildBuilder.AddProperty(ScaleZHandle).DisplayName(VOXEL_LOCTEXT("Scale Z"));
		}
	}
};

DEFINE_VOXEL_STRUCT_LAYOUT(FVoxelFoliageScaleSettings, FVoxelFoliageScaleSettingsCustomization);