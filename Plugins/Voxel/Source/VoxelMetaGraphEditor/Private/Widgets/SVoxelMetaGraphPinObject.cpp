// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "SVoxelMetaGraphPinObject.h"
#include "SVoxelMetaGraphObjectSelector.h"
#include "VoxelPinType.h"

void SVoxelMetaGraphPinObject::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	VOXEL_FUNCTION_COUNTER();

	SGraphPinObject::Construct(SGraphPinObject::FArguments(), InGraphPinObj);
	GetLabelAndValue()->SetWrapSize(300.f);
}

TSharedRef<SWidget>	SVoxelMetaGraphPinObject::GetDefaultValueWidget()
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(GraphPinObj))
	{
		return SNullWidget::NullWidget;
	}

	const FVoxelPinType Type = FVoxelPinType(GraphPinObj->PinType).GetExposedType();
	if (!ensure(Type.IsObject()))
	{
		return SNullWidget::NullWidget;
	}

	UClass* AllowedClass = Type.GetClass();
	if (!ensure(AllowedClass))
	{
		return SNullWidget::NullWidget;
	}

	return
		SNew(SBox)
		.MaxDesiredWidth(200.f)
		[
			SNew(SVoxelMetaGraphObjectSelector)
			.Visibility(this, &SGraphPin::GetDefaultValueVisibility)
			.AllowedClass(AllowedClass)
			.ThumbnailPool(FVoxelEditorUtilities::GetThumbnailPool())
			.ObjectPath_Lambda([this]
			{
				return GetAssetData(true).UE_501_SWITCH(ObjectPath, GetSoftObjectPath()).ToString();
			})
			.OnObjectChanged(this, &SVoxelMetaGraphPinObject::OnAssetSelectedFromPicker)
		];
}

void SVoxelMetaGraphPinObject::OnAssetSelectedFromPicker(const FAssetData& AssetData)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(!GraphPinObj->IsPendingKill()) ||
		AssetData == GetAssetData(true))
	{
		return;
	}
	
	const FVoxelTransaction Transaction(GraphPinObj, "Change Object Pin Value");

	GraphPinObj->GetSchema()->TrySetDefaultObject(*GraphPinObj, AssetData.GetAsset());
}