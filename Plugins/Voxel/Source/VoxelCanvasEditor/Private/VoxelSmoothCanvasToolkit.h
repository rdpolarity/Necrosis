// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Toolkits/BaseToolkit.h"

class FVoxelSmoothCanvasToolkit : public FModeToolkit
{
public:
	FVoxelSmoothCanvasToolkit() = default;

	//~ Begin FModeToolkit Interface
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode) override;
	
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;

	virtual FText GetActiveToolDisplayName() const override;
	virtual FText GetActiveToolMessage() const override;
	//~ End FModeToolkit Interface
};