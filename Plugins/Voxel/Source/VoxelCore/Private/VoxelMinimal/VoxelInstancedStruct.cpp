// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMinimal.h"
#include "UObject/UObjectThreadContext.h"

void FVoxelInstancedStruct::Reset()
{
	if (!ScriptStruct ||
		!StructMemory)
	{
		ensure(!ScriptStruct && !StructMemory);
		return;
	}

	if (UObjectInitialized())
	{
		ScriptStruct->DestroyStruct(StructMemory);
	}

	FMemory::Free(StructMemory);

	ScriptStruct = nullptr;
	StructMemory = nullptr;
}

void* FVoxelInstancedStruct::Release()
{
	void* OutStructMemory = StructMemory;

	ScriptStruct = nullptr;
	StructMemory = nullptr;

	return OutStructMemory;
}

void FVoxelInstancedStruct::MoveRawPtr(UScriptStruct* NewScriptStruct, void* NewStructMemory)
{
	Reset();

	check(NewScriptStruct);
	check(NewStructMemory);

	ScriptStruct = NewScriptStruct;
	StructMemory = NewStructMemory;
}

void FVoxelInstancedStruct::InitializeAs(UScriptStruct* NewScriptStruct, const void* NewStructMemory)
{
	Reset();

	if (!NewScriptStruct)
	{
		// Null
		ensure(!NewStructMemory);
		return;
	}

	ScriptStruct = NewScriptStruct;
	StructMemory = FMemory::Malloc(FMath::Max(1, ScriptStruct->GetStructureSize()));

	ScriptStruct->InitializeStruct(StructMemory);

	if (NewStructMemory)
	{
		ScriptStruct->CopyScriptStruct(StructMemory, NewStructMemory);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelInstancedStruct::Serialize(FArchive& Ar)
{
	VOXEL_FUNCTION_COUNTER_LLM();

	using FVersion = DECLARE_VOXEL_VERSION
	(
		InitialVersion,
		StoreName
	);

	uint8 Version = FVersion::LatestVersion;
	Ar << Version;
	ensure(Version <= FVersion::LatestVersion);

	if (Ar.IsLoading())
	{
		FString StructName;
		if (Version >= FVersion::StoreName)
		{
			Ar << StructName;
		}

		{
			UScriptStruct* NewScriptStruct = nullptr;
			Ar << NewScriptStruct;
			if (NewScriptStruct)
			{
				Ar.Preload(NewScriptStruct);
			}

			if (NewScriptStruct != ScriptStruct)
			{
				InitializeAs(NewScriptStruct);
			}
			ensure(ScriptStruct == NewScriptStruct);
		}

		int32 SerializedSize = 0; 
		Ar << SerializedSize;

		if (!ScriptStruct && SerializedSize > 0)
		{
			// Struct not found

			TArray<FProperty*> Properties;
			Ar.GetSerializedPropertyChain(Properties);

			FString Name;
			if (const FUObjectSerializeContext* SerializeContext = FUObjectThreadContext::Get().GetSerializeContext())
			{
				if (SerializeContext->SerializedObject)
				{
					Name += SerializeContext->SerializedObject->GetPathName();
				}
			}

			for (int32 Index = 0; Index < Properties.Num(); Index++)
			{
				if (!Name.IsEmpty())
				{
					Name += ".";
				}
				Name += Properties.Last(Index)->GetNameCPP();
			}
			LOG_VOXEL(Warning, "Struct %s not found: %s", *StructName, *Name);

			ensureVoxelSlow(false);

			Ar.Seek(Ar.Tell() + SerializedSize);
		}
		else if (ScriptStruct)
		{
			const int64 Start = Ar.Tell();
			ScriptStruct->SerializeItem(Ar, StructMemory, nullptr);
			ensure(Ar.Tell() - Start == SerializedSize);

			if (ScriptStruct->IsChildOf(FVoxelVirtualStruct::StaticStruct()))
			{
				static_cast<FVoxelVirtualStruct*>(StructMemory)->PostSerialize();
			}

			if (ScriptStruct == FBodyInstance::StaticStruct())
			{
				static_cast<FBodyInstance*>(StructMemory)->LoadProfileData(false);
			}
		}
	}
	else if (Ar.IsSaving())
	{
		FString StructName;
		if (ScriptStruct)
		{
			StructName = ScriptStruct->GetName();
		}
		else
		{
			StructName = "<null>";
		}

		Ar << StructName;
		Ar << ScriptStruct;
	
		const int64 SerializedSizePosition = Ar.Tell();
		{
			int32 SerializedSize = 0;
			Ar << SerializedSize;
		}
		
		const int64 Start = Ar.Tell();
		if (ScriptStruct)
		{
			check(StructMemory);

			if (ScriptStruct->IsChildOf(FVoxelVirtualStruct::StaticStruct()))
			{
				static_cast<FVoxelVirtualStruct*>(StructMemory)->PreSerialize();
			}

			ScriptStruct->SerializeItem(Ar, StructMemory, nullptr);
		}
		const int64 End = Ar.Tell();

		Ar.Seek(SerializedSizePosition);
		{
			int32 SerializedSize = End - Start;
			Ar << SerializedSize;
		}
		Ar.Seek(End);
	}

	return true;
}

bool FVoxelInstancedStruct::Identical(const FVoxelInstancedStruct* Other, uint32 PortFlags) const
{
	if (!ensure(Other))
	{
		return false;
	}

	if (ScriptStruct != Other->ScriptStruct)
	{
		return false;
	}

	if (!ScriptStruct)
	{
		return true;
	}

	VOXEL_SCOPE_COUNTER("CompareScriptStruct");
	return ScriptStruct->CompareScriptStruct(StructMemory, Other->StructMemory, PortFlags);
}

bool FVoxelInstancedStruct::ExportTextItem(FString& ValueStr, const FVoxelInstancedStruct& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const
{
	VOXEL_FUNCTION_COUNTER_LLM();

	if (!ScriptStruct)
	{
		ValueStr += "None";
		return true;
	}

	const FVoxelInstancedStruct* DefaultValuePtr = &DefaultValue;

	ValueStr += ScriptStruct->GetPathName();
	ScriptStruct->ExportText(
		ValueStr,
		StructMemory,
		DefaultValuePtr && ScriptStruct == DefaultValuePtr->GetScriptStruct() ? DefaultValuePtr->GetStructMemory() : nullptr,
		Parent,
		PortFlags,
		ExportRootScope);

	return true;
}

bool FVoxelInstancedStruct::ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText, FArchive* InSerializingArchive)
{
	VOXEL_FUNCTION_COUNTER_LLM();

	FString StructPathName;
	{
		const TCHAR* Result = FPropertyHelpers::ReadToken(Buffer, StructPathName, true);
		if (!Result)
		{
			return false;
		}
		Buffer = Result;
	}

	if (StructPathName.Len() == 0 ||
		StructPathName == "None")
	{
		Reset();
		return true;
	}

	UScriptStruct* NewScriptStruct = LoadObject<UScriptStruct>(nullptr, *StructPathName);
	if (!ensure(NewScriptStruct))
	{
		return false;
	}

	if (NewScriptStruct != ScriptStruct)
	{
		InitializeAs(NewScriptStruct);
	}

	{
		const TCHAR* Result = NewScriptStruct->ImportText(
			Buffer,
			StructMemory,
			Parent,
			PortFlags,
			ErrorText,
			[&] { return NewScriptStruct->GetName(); });

		if (!Result)
		{
			return false;
		}
		Buffer = Result;
	}

	return true;
}

void FVoxelInstancedStruct::AddStructReferencedObjects(FReferenceCollector& Collector)
{
	VOXEL_FUNCTION_COUNTER_LLM();

	if (!ScriptStruct)
	{
		return;
	}

	Collector.AddReferencedObject(ScriptStruct);
	check(ScriptStruct);

	FVoxelObjectUtilities::AddStructReferencedObjects(Collector, ScriptStruct, StructMemory);
}