// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"

struct FVoxelPinValue;

BEGIN_VOXEL_NAMESPACE(MetaGraph)

class FPinValueCustomization : public FVoxelTicker
{
public:
	//~ Begin FVoxelTicker Interface
	virtual void Tick() override;
	//~ End FVoxelTicker Interface

	virtual TSharedPtr<FPinValueCustomization> GetSelfSharedPtr() VOXEL_PURE_VIRTUAL(nullptr);

public:
	void AddPinValueCustomization(
		IDetailCategoryBuilder& CategoryBuilder,
		const TSharedRef<IPropertyHandle>& ValueHandle,
		const FVoxelPinValue& Value,
		const FString& RowName,
		const FString& Tooltip,
		const TFunction<void(FDetailWidgetRow& Row, const TSharedRef<IPropertyHandle>& Handle, const TSharedRef<SWidget>& NameWidget, const TSharedRef<SWidget>& ValueWidget)>& SetupRowLambda);

	void AddPinValueCustomization(
		IDetailChildrenBuilder& ChildrenBuilder,
		const TSharedRef<IPropertyHandle>& ValueHandle,
		const FVoxelPinValue& Value,
		const FString& RowName,
		const FString& Tooltip,
		const TFunction<void(FDetailWidgetRow& Row, const TSharedRef<IPropertyHandle>& Handle, const TSharedRef<SWidget>& NameWidget, const TSharedRef<SWidget>& ValueWidget)>& SetupRowLambda);

private:
	void AddPinValueCustomization_Impl(
		const TFunction<FDetailWidgetRow&(const FText& FilterString)>& CreateCustomRow,
		const TFunction<IDetailPropertyRow*(const TSharedRef<FStructOnScope>& StructOnScope)>& AddExternalStructure,
		const FSimpleDelegate& RefreshDelegate,
		const TSharedRef<IPropertyHandle>& ValueHandle,
		const FVoxelPinValue& Value,
		const FString& RowName,
		const FString& Tooltip,
		const TFunction<void(FDetailWidgetRow& Row, const TSharedRef<IPropertyHandle>& Handle, const TSharedRef<SWidget>& NameWidget, const TSharedRef<SWidget>& ValueWidget)>& SetupRowLambda);

public:
	void AddStructOnScope(const TSharedRef<IPropertyHandle>& StructHandle, const TSharedRef<FStructOnScope>& StructOnScope);
	void BindStructPropertyValueChanges(const TSharedRef<IPropertyHandle>& StructHandle, const TArray<TSharedPtr<IPropertyHandle>>& ChildHandles);

	void SyncAllEditableInstancesFromSource();

private:
	void OnStructValuePreChange(TWeakPtr<IPropertyHandle> WeakStructProperty);
	void OnStructValuePostChange(TWeakPtr<IPropertyHandle> WeakStructProperty) const;
	void SyncEditableInstanceToSource(const TSharedPtr<IPropertyHandle>& StructProperty) const;
	void SyncEditableInstanceFromSource(const TSharedPtr<IPropertyHandle>& StructProperty) const;

private:
	TMap<TSharedPtr<IPropertyHandle>, TSharedPtr<FStructOnScope>> StructOnScopes;

	double LastSyncEditableInstanceFromSourceSeconds = 0.0;
};

END_VOXEL_NAMESPACE(MetaGraph)