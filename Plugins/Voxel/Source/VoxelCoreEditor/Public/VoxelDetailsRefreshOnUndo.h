// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "EditorUndoClient.h"
#include "VoxelEditorDetailsUtilities.h"

class VOXELCOREEDITOR_API FVoxelDetailsRefreshOnUndo : public FEditorUndoClient
{
public:
	FSimpleMulticastDelegate Delegate;

	FVoxelDetailsRefreshOnUndo()
	{
		GEditor->RegisterForUndo(this);
	}
	template<typename T>
	FVoxelDetailsRefreshOnUndo(T& Object)
		: FVoxelDetailsRefreshOnUndo()
	{
		Delegate.Add(FVoxelEditorUtilities::MakeRefreshDelegate(Object));
	}
	virtual ~FVoxelDetailsRefreshOnUndo() override
	{
		GEditor->UnregisterForUndo(this);
	}
	
	virtual void PostUndo(bool bSuccess) override
	{
		Delegate.Broadcast();
	}
	virtual void PostRedo(bool bSuccess) override
	{
		PostUndo(bSuccess);
	}
};