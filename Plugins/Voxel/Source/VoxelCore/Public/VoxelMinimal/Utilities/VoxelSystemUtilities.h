// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "Containers/Ticker.h"
#include "Interfaces/IPluginManager.h"

class VOXELCORE_API FVoxelTicker : public FTSTickerObjectBase
{
public:
	FVoxelTicker() = default;

	//~ Begin FTickerObjectBase Interface
	virtual bool Tick(float DeltaTime) final override
	{
		VOXEL_LLM_SCOPE();

		Tick();
		return true;
	}
	//~ End FTickerObjectBase Interface

	virtual void Tick() = 0;
};

struct VOXELCORE_API FVoxelSystemUtilities
{
	// Delay until next fire; 0 means "next frame"
	static void DelayedCall(TFunction<void()> Call, float Delay = 0);

	static IPlugin& GetPlugin();

public:
	static FString SaveFileDialog(
		const FString& DialogTitle, 
		const FString& DefaultPath, 
		const FString& DefaultFile, 
		const FString& FileTypes);

	static bool OpenFileDialog(
		const FString& DialogTitle, 
		const FString& DefaultPath, 
		const FString& DefaultFile, 
		const FString& FileTypes, 
		bool bAllowMultiple, 
		TArray<FString>& OutFilenames);

private:
	static bool FileDialog(
		bool bSave, 
		const FString& DialogTitle, 
		const FString& DefaultPath, 
		const FString& DefaultFile, 
		const FString& FileTypes, 
		bool bAllowMultiple, 
		TArray<FString>& OutFilenames);
};