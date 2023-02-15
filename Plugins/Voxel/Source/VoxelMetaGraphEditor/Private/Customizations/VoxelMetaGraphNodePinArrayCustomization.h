// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"

class IVoxelNodeDefinition;
class UVoxelMetaGraphStructNode;
class FVoxelMetaGraphEditorToolkit;

BEGIN_VOXEL_NAMESPACE(MetaGraph)

class FVoxelNodeCustomization;

class FVoxelNodePinArrayCustomization
	: public IDetailCustomNodeBuilder
	, public TSharedFromThis<FVoxelNodePinArrayCustomization>
{
public:
	FString PinName;
	FString Tooltip;
	TArray<TSharedPtr<IPropertyHandle>> Properties;
	TWeakPtr<FVoxelNodeCustomization> WeakCustomization;
	TWeakObjectPtr<UVoxelMetaGraphStructNode> WeakStructNode;
	TWeakPtr<IVoxelNodeDefinition> WeakNodeDefinition;

	//~ Begin IDetailCustomNodeBuilder Interface
	virtual void GenerateHeaderRowContent(FDetailWidgetRow& NodeRow) override;
	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override;
	virtual FName GetName() const override { return *PinName; }
	//~ End IDetailCustomNodeBuilder Interface

private:
	void AddNewPin();
	bool CanAddNewPin() const;
	void ClearAllPins();
	bool CanRemovePin() const;

	TSharedRef<SWidget> CreateElementEditButton(const FName EntryPinName);

	void InsertPinBefore(const FName EntryPinName);
	void DeletePin(const FName EntryPinName);
	bool CanDeletePin(const FName EntryPinName);
	void DuplicatePin(const FName EntryPinName);
};

END_VOXEL_NAMESPACE(MetaGraph)