// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelMetaGraphPinValueCustomization.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

class FVariableCollectionCustomization;

class FGeneratorCategoryDetails : public IDetailCustomNodeBuilder
{
public:
	FString Category;
	TMap<int32, TSharedPtr<IPropertyHandle>> Properties;
	TMap<FString, TSharedPtr<FGeneratorCategoryDetails>> InnerGroups;
	TWeakPtr<FVariableCollectionCustomization> WeakCustomization;

	//~ Begin IDetailCustomNodeBuilder Interface
	virtual void GenerateHeaderRowContent(FDetailWidgetRow& NodeRow) override;
	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override;
	virtual FName GetName() const override { return *Category; }
	//~ End IDetailCustomNodeBuilder Interface
};

class FVariableCollectionCustomization
	: public IPropertyTypeCustomization
	, public FSelfRegisteringEditorUndoClient
	, public FPinValueCustomization
{
public:
	//~ Begin IPropertyTypeCustomization Interface
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	//~ End IDetailCustomNodeBuilder Interface

	//~ Begin FSelfRegisteringEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	//~ End FSelfRegisteringEditorUndoClient Interface

	//~ Begin FPinValueCustomization Interface
	virtual TSharedPtr<FPinValueCustomization> GetSelfSharedPtr() override;
	//~ End FPinValueCustomization Interface

	void AddVariable(IDetailCategoryBuilder* CategoryBuilder, IDetailChildrenBuilder* ChildBuilder, const TSharedPtr<IPropertyHandle>& PropertyHandle, bool bUsesCategoryBuilder);

private:
	TArray<FString> GetCategoryChain(const FString& Category) const;

private:
	FSimpleDelegate RefreshDelegate;
	TArray<FString> CategoriesOrder;
	const FString CategoriesPrefix = "";
};

END_VOXEL_NAMESPACE(MetaGraph)