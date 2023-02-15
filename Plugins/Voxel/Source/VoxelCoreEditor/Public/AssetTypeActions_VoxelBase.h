// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "AssetTypeActions_Base.h"

class FVoxelBaseEditorToolkit;

class VOXELCOREEDITOR_API FAssetTypeActions_VoxelBase : public FAssetTypeActions_Base
{
public:
	FAssetTypeActions_VoxelBase() = default;

	//~ Begin FAssetTypeActions_Base Interface
	virtual uint32 GetCategories() override;

	virtual bool HasActions(const TArray<UObject*>& InObjects) const override { return true; }
	virtual void GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor) override;
	//~ End FAssetTypeActions_Base Interface

	virtual bool SupportReimport() const { return false; }
	virtual TSharedPtr<FVoxelBaseEditorToolkit> MakeToolkit() const { return nullptr; }
};

class VOXELCOREEDITOR_API FAssetTypeActions_Voxel : public FAssetTypeActions_VoxelBase
{
public:
	const FText Name;
	const FColor Color;
	UClass* const SupportedClass;
	const TArray<FText> SubMenus;
	const TFunction<TSharedPtr<FVoxelBaseEditorToolkit>()> MakeToolkitLambda;

	FAssetTypeActions_Voxel(
		const FText& Name,
		FColor Color,
		UClass* SupportedClass,
		const TArray<FText> SubMenus,
		const TFunction<TSharedPtr<FVoxelBaseEditorToolkit>()>& MakeToolkitLambda)
		: Name(Name)
		, Color(Color)
		, SupportedClass(SupportedClass)
		, SubMenus(SubMenus)
		, MakeToolkitLambda(MakeToolkitLambda)
	{
	}

	//~ Begin FAssetTypeActions_VoxelBase Interface
	virtual FText GetName() const override { return Name; }
	virtual FColor GetTypeColor() const override { return Color; }
	virtual UClass* GetSupportedClass() const override { return SupportedClass; }
	virtual const TArray<FText>& GetSubMenus() const override { return SubMenus; }

	virtual TSharedPtr<FVoxelBaseEditorToolkit> MakeToolkit() const override { return MakeToolkitLambda(); }
	//~ End FAssetTypeActions_VoxelBase Interface
};