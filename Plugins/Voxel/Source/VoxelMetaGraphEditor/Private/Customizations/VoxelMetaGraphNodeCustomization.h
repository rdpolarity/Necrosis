// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelMetaGraphPinValueCustomization.h"

class IVoxelNodeDefinition;
class UVoxelMetaGraphStructNode;
class FVoxelMetaGraphEditorToolkit;

BEGIN_VOXEL_NAMESPACE(MetaGraph)

class FVoxelNodeCustomization
	: public IDetailCustomization
	, public FPinValueCustomization
{
public:
	FVoxelNodeCustomization(TWeakPtr<FVoxelMetaGraphEditorToolkit> Toolkit) : WeakToolkit(Toolkit)
	{
	}

	//~ Begin IDetailCustomization Interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	//~ End IDetailCustomization Interface

	//~ Begin FPinValueCustomization Interface
	virtual TSharedPtr<FPinValueCustomization> GetSelfSharedPtr() override;
	//~ End FPinValueCustomization Interface

	TSharedRef<SWidget> CreateExposePinButton(const TWeakObjectPtr<UVoxelMetaGraphStructNode>& WeakNode, FName PinName) const;

private:
	TMap<FName, TSharedPtr<IPropertyHandle>> InitializeStructChildren(const TSharedRef<IPropertyHandle>& StructHandle);

private:
	TWeakPtr<FVoxelMetaGraphEditorToolkit> WeakToolkit;
	FSimpleDelegate RefreshDelegate;
	TSharedPtr<IVoxelNodeDefinition> NodeDefinition;
};

END_VOXEL_NAMESPACE(MetaGraph)