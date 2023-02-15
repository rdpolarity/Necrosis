// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelEdGraph.h"
#include "VoxelGraphSchema.h"
#include "VoxelGraphEditorToolkit.h"

TSharedPtr<FVoxelGraphEditorToolkit> UVoxelEdGraph::GetGraphToolkit() const
{
	return WeakToolkit.Pin();
}

void UVoxelEdGraph::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
	
	DelayOnGraphChangedScopeStack.Add(MakeShared<FVoxelGraphDelayOnGraphChangedScope>());
}

void UVoxelEdGraph::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive)
	{
		return;
	}

	UVoxelGraphSchema::OnGraphChanged(this);

	if (ensure(DelayOnGraphChangedScopeStack.Num() > 0))
	{
		DelayOnGraphChangedScopeStack.Pop();
	}
}