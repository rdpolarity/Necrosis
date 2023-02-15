// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelBlockStructure.h"
#include "VoxelBlockRegistry.h"
#include "VoxelRuntime/VoxelRuntime.h"

VOXEL_RUN_ON_STARTUP_GAME(FixupBlockStructureConfigs)
{
	for (const UScriptStruct* Struct : GetDerivedStructs<FVoxelBlockStructureConfig>())
	{
		for (FProperty& Property : GetStructProperties(Struct))
		{
			ensure(!Property.HasAnyPropertyFlags(CPF_Edit));
			ensure(!Property.HasAnyPropertyFlags(CPF_BlueprintVisible));

			if (!Property.HasAnyPropertyFlags(CPF_Transient))
			{
				Property.SetPropertyFlags(CPF_BlueprintVisible);
				Property.SetPropertyFlags(CPF_Edit);
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelBlockReference::Resolve(const FVoxelRuntime& Runtime)
{
	if (!Asset)
	{
		return;
	}

	Data = Runtime.GetSubsystem<FVoxelBlockRegistry>().GetBlockData(Asset);
}

void UVoxelBlockStructureAsset::PostLoad()
{
	Super::PostLoad();
	
	FixupConfig();
}

#if WITH_EDITOR
void UVoxelBlockStructureAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FixupConfig();
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelBlockStructureAsset::FixupConfig()
{
	if (!Structure.IsValid())
	{
		Config = {};
		return;
	}

	UScriptStruct* ConfigStruct = Structure.Get<FVoxelBlockStructure>().GetConfigStruct();
	if (Config.GetScriptStruct() != ConfigStruct)
	{
		Config = FVoxelInstancedStruct(ConfigStruct);
	}
}