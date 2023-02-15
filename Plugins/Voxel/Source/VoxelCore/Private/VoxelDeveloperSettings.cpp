// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelDeveloperSettings.h"
#include "Misc/ConfigCacheIni.h"
#if VOXEL_ENGINE_VERSION >= 501
#include "Misc/ConfigUtilities.h"
#endif

UVoxelDeveloperSettings::UVoxelDeveloperSettings()
{
	CategoryName = "Plugins";
}

FName UVoxelDeveloperSettings::GetContainerName() const
{
	return "Project";
}

void UVoxelDeveloperSettings::PostInitProperties()
{
	Super::PostInitProperties();

#if WITH_EDITOR
	if (IsTemplate())
	{
		ImportConsoleVariableValues();
	}
#endif
}

void UVoxelDeveloperSettings::PostCDOContruct()
{
	Super::PostCDOContruct();
	
	UE_501_ONLY(UE::ConfigUtilities::)ApplyCVarSettingsFromIni(*GetClass()->GetPathName(), *GEngineIni, ECVF_SetByProjectSetting);
}

#if WITH_EDITOR
void UVoxelDeveloperSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property)
	{
		ExportValuesToConsoleVariables(PropertyChangedEvent.Property);
	}
}
#endif