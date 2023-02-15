// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "EdGraph/EdGraph.h"
#include "VoxelEdGraph.generated.h"

class SGraphEditor;
class FVoxelGraphEditorToolkit;
struct FVoxelGraphDelayOnGraphChangedScope;

UCLASS()
class VOXELGRAPHEDITOR_API UVoxelEdGraph : public UEdGraph
{
	GENERATED_BODY()

public:
	TWeakPtr<FVoxelGraphEditorToolkit> WeakToolkit;

	TSharedPtr<FVoxelGraphEditorToolkit> GetGraphToolkit() const;

	//~ Begin UObject interface
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject interface

private:
	TArray<TSharedPtr<FVoxelGraphDelayOnGraphChangedScope>> DelayOnGraphChangedScopeStack;
};