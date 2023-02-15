// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelGraphMessages.h"
#if WITH_EDITOR
#include "Subsystems/AssetEditorSubsystem.h"
#endif

VOXELCORE_API FVoxelGraphMessages* GVoxelGraphMessages = nullptr;

void FVoxelGraphMessages::FMessageConsumer::LogMessage(const TSharedRef<FTokenizedMessage>& Message)
{
	if (Message->GetSeverity() == EMessageSeverity::Error)
	{
		*HasErrorPtr = true;
	}

#if WITH_EDITOR
	for (UObject* Object = VOXEL_CONST_CAST(Graph.Get()); Object; Object = Object->GetOuter())
	{
		if (GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(Object, false))
		{
			// Don't show a popup if the asset is opened
			return;
		}
	}
#endif

	FVoxelMessages::LogMessage(Message);
}