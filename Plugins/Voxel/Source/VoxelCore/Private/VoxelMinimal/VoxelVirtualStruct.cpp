// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMinimal.h"

#if DO_CHECK
VOXEL_RUN_ON_STARTUP_GAME(CheckVoxelVirtualStruct)
{
	for (UScriptStruct* Struct : GetDerivedStructs<FVoxelVirtualStruct>())
	{
		TVoxelInstancedStruct<FVoxelVirtualStruct> Instance(Struct);
		ensureAlwaysMsgf(Instance->GetStruct() == Struct, TEXT("Missing %s() in %s"), *Instance->Internal_GetMacroName(), *Struct->GetStructCPPName());
	}
}
#endif

FString FVoxelVirtualStruct::Internal_GetMacroName() const
{
	return "GENERATED_VIRTUAL_STRUCT_BODY";
}

TSharedRef<FVoxelVirtualStruct> FVoxelVirtualStruct::MakeSharedCopy() const
{
	const UScriptStruct* Struct = GetStruct();

	FVoxelVirtualStruct* StructMemory = static_cast<FVoxelVirtualStruct*>(FMemory::Malloc(Struct->GetStructureSize()));
	Struct->InitializeStruct(StructMemory);
	Struct->CopyScriptStruct(StructMemory, this);
	return MakeShareable(StructMemory);
}