// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelMetaGraphPinValueCustomization.h"

class FVoxelMetaGraphEditorToolkit;

BEGIN_VOXEL_NAMESPACE(MetaGraph)

class FParameterObjectCustomization
	: public IDetailCustomization
	, public FPinValueCustomization
{
public:
	FParameterObjectCustomization(TWeakPtr<FVoxelMetaGraphEditorToolkit> Toolkit, FGuid TargetParameterId)
		: WeakToolkit(Toolkit)
		, TargetParameterId(TargetParameterId)
	{
	}

	//~ Begin IDetailCustomization Interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	//~ End IDetailCustomization Interface

	//~ Begin FPinValueCustomization Interface
	virtual TSharedPtr<FPinValueCustomization> GetSelfSharedPtr() override;
	//~ End FPinValueCustomization Interface

private:
	TWeakPtr<FVoxelMetaGraphEditorToolkit> WeakToolkit;
	FGuid TargetParameterId;
};

END_VOXEL_NAMESPACE(MetaGraph)