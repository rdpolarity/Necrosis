// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMinimal.h"

#include "UObject/MetaData.h"
#include "Serialization/BulkDataReader.h"
#include "Serialization/BulkDataWriter.h"

#if WITH_EDITOR
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#endif

#if WITH_EDITOR
FString FVoxelObjectUtilities::GetClassDisplayName_EditorOnly(const UClass* Class)
{
	if (!Class)
	{
		return "NULL";
	}

	FString ClassName = Class->GetDisplayNameText().ToString();
	
	const TArray<FString> Acronyms = { "RGB", "LOD" };
	for (const FString& Acronym : Acronyms)
	{
		ClassName.ReplaceInline(*Acronym, *(Acronym + " "), ESearchCase::CaseSensitive);
	}

	return ClassName;
}

FString FVoxelObjectUtilities::GetPropertyTooltip(const UFunction& Function, const FProperty& Property)
{
	ensure(Property.Owner == &Function);

	return GetPropertyTooltip(
		Function.GetToolTipText().ToString(),
		Property.GetName(),
		&Property == Function.GetReturnProperty());
}

FString FVoxelObjectUtilities::GetPropertyTooltip(const FString& FunctionTooltip, const FString& PropertyName, bool bIsReturnPin)
{
	VOXEL_FUNCTION_COUNTER();

	// Same as UK2Node_CallFunction::GeneratePinTooltipFromFunction

	const FString Tag = bIsReturnPin ? "@return" : "@param";

	int32 Position = -1;
	while (Position < FunctionTooltip.Len())
	{
		Position = FunctionTooltip.Find(Tag, ESearchCase::IgnoreCase, ESearchDir::FromStart, Position);
		if (Position == -1) // If the tag wasn't found
		{
			break;
		}

		// Advance past the tag
		Position += Tag.Len();

		// Advance past whitespace
		while (Position < FunctionTooltip.Len() && FChar::IsWhitespace(FunctionTooltip[Position]))
		{
			Position++;
		}

		// If this is a parameter pin
		if (!bIsReturnPin)
		{
			FString TagParamName;

			// Copy the parameter name
			while (Position < FunctionTooltip.Len() && !FChar::IsWhitespace(FunctionTooltip[Position]))
			{
				TagParamName.AppendChar(FunctionTooltip[Position++]);
			}

			// If this @param tag doesn't match the param we're looking for
			if (TagParamName != PropertyName)
			{
				continue;
			}
		}

		// Advance past whitespace
		while (Position < FunctionTooltip.Len() && FChar::IsWhitespace(FunctionTooltip[Position]))
		{
			Position++;
		}

		FString PropertyTooltip;
		while (Position < FunctionTooltip.Len() && FunctionTooltip[Position] != TEXT('@'))
		{
			// advance past newline
			while (Position < FunctionTooltip.Len() && FChar::IsLinebreak(FunctionTooltip[Position]))
			{
				Position++;

				// advance past whitespace at the start of a new line
				while (Position < FunctionTooltip.Len() && FChar::IsWhitespace(FunctionTooltip[Position]))
				{
					Position++;
				}

				// replace the newline with a single space
				if (Position < FunctionTooltip.Len() && !FChar::IsLinebreak(FunctionTooltip[Position]))
				{
					PropertyTooltip.AppendChar(TEXT(' '));
				}
			}

			if (Position < FunctionTooltip.Len() && FunctionTooltip[Position] != TEXT('@'))
			{
				PropertyTooltip.AppendChar(FunctionTooltip[Position++]);
			}
		}

		// Trim any trailing whitespace from the descriptive text
		PropertyTooltip.TrimEndInline();

		// If we came up with a valid description for the param/return-val
		if (PropertyTooltip.IsEmpty())
		{
			continue;
		}

		return PropertyTooltip;
	}

	return FunctionTooltip;
}

TMap<FName, FString> FVoxelObjectUtilities::GetMetadata(const UObject* Object)
{
	if (!ensure(Object))
	{
		return {};
	}

	return Object->GetOutermost()->GetMetaData()->ObjectMetaDataMap.FindRef(Object);
}

void FVoxelObjectUtilities::CreateNewAsset_Deferred(UClass* Class, const FString& BaseName, const FString& Suffix, TFunction<void(UObject*)> SetupObject)
{
	// Create an appropriate and unique name 
	FString AssetName;
	FString PackageName;

	const FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().CreateUniqueAssetName(BaseName, Suffix, PackageName, AssetName);

	IVoxelFactory* Factory = IVoxelAutoFactoryInterface::GetInterface().MakeFactory(Class);
	if (!ensure(Factory))
	{
		return;
	}

	Factory->OnSetupObject.AddLambda(SetupObject);

	IContentBrowserSingleton& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get();
	ContentBrowser.FocusPrimaryContentBrowser(false);
	ContentBrowser.CreateNewAsset(AssetName, FPackageName::GetLongPackagePath(PackageName), Class, Factory->GetUFactory());
}

UObject* FVoxelObjectUtilities::CreateNewAsset_Direct(UClass* Class, const FString& BaseName, const FString& Suffix)
{
	FString NewPackageName;
	FString NewAssetName;

	const FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().CreateUniqueAssetName(BaseName, Suffix, NewPackageName, NewAssetName);

	UPackage* Package = CreatePackage(*NewPackageName);
	if (!ensure(Package))
	{
		return nullptr;
	}

	UObject* Object = NewObject<UObject>(Package, Class, *NewAssetName, RF_Public | RF_Standalone);
	if (!ensure(Object))
	{
		return nullptr;
	}

	Object->MarkPackageDirty();
	return Object;
}
#endif

void FVoxelObjectUtilities::InvokeFunctionWithNoParameters(UObject* Object, UFunction* Function)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(Object) ||
		!ensure(Function) ||
		!ensure(!Function->Children))
	{
		return;
	}

	TGuardValue<bool> Guard(GAllowActorScriptExecutionInEditor, true);

	void* Params = nullptr;
	if (Function->ParmsSize > 0)
	{
		// Return value
		Params = FMemory::Malloc(Function->ParmsSize);
	}
	Object->ProcessEvent(Function, Params);
	FMemory::Free(Params);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelObjectUtilities::PropertyFromText_Direct(const FProperty& Property, const FString& Text, void* Data, UObject* Owner)
{
#if VOXEL_ENGINE_VERSION < 501
	return Property.ImportText(*Text, Data, PPF_None, Owner) != nullptr;
#else
	return Property.ImportText_Direct(*Text, Data, Owner, PPF_None) != nullptr;
#endif
}

bool FVoxelObjectUtilities::PropertyFromText_InContainer(const FProperty& Property, const FString& Text, void* ContainerData, UObject* Owner)
{
#if VOXEL_ENGINE_VERSION < 501
	return Property.ImportText(*Text, Property.ContainerPtrToValuePtr<void>(ContainerData), PPF_None, Owner) != nullptr;
#else
	return Property.ImportText_InContainer(*Text, ContainerData, Owner, PPF_None) != nullptr;
#endif
}

bool FVoxelObjectUtilities::PropertyFromText_InContainer(const FProperty& Property, const FString& Text, UObject* Owner)
{
	check(Owner);
	check(Owner->GetClass()->IsChildOf(CastChecked<UClass>(Property.Owner.ToUObject())));

	return PropertyFromText_InContainer(Property, Text, static_cast<void*>(Owner), Owner);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString FVoxelObjectUtilities::PropertyToText_Direct(const FProperty& Property, const void* Data, const UObject* Owner)
{
	FString Value;
	ensure(Property.ExportText_Direct(Value, Data, Data, VOXEL_CONST_CAST(Owner), PPF_None));
	return Value;
}

FString FVoxelObjectUtilities::PropertyToText_InContainer(const FProperty& Property, const void* ContainerData, const UObject* Owner)
{
	FString Value;
	ensure(Property.ExportText_InContainer(0, Value, ContainerData, ContainerData, VOXEL_CONST_CAST(Owner), PPF_None));
	return Value;
}

FString FVoxelObjectUtilities::PropertyToText_InContainer(const FProperty& Property, const UObject* Owner)
{
	check(Owner);
	check(Owner->GetClass()->IsChildOf(CastChecked<UClass>(Property.Owner.ToUObject())));

	return PropertyToText_InContainer(Property, static_cast<const void*>(Owner), Owner);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelObjectUtilities::ShouldSerializeBulkData(FArchive& Ar)
{
	return
		Ar.IsPersistent() &&
		!Ar.IsObjectReferenceCollector() &&
		!Ar.ShouldSkipBulkData() &&
		ensure(!Ar.IsTransacting());
}

void FVoxelObjectUtilities::SerializeBulkData(
	UObject* Object,
	FByteBulkData& BulkData,
	FArchive& Ar,
	TFunctionRef<void()> SaveBulkData,
	TFunctionRef<void()> LoadBulkData)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ShouldSerializeBulkData(Ar))
	{
		return;
	}

	if (Ar.IsSaving())
	{
		// Clear the bulk data before writing it
		BulkData.RemoveBulkData();

		{
			VOXEL_SCOPE_COUNTER("SaveBulkData");
			SaveBulkData();
		}
	}

	BulkData.Serialize(Ar, Object);

	// NOTE: we can't call RemoveBulkData after saving as serialization is queued until the end of the save process

	if (Ar.IsLoading())
	{
		{
			VOXEL_SCOPE_COUNTER("LoadBulkData");
			LoadBulkData();
		}

		// Clear bulk data after loading to save memory
		BulkData.RemoveBulkData();
	}
}

void FVoxelObjectUtilities::SerializeBulkData(
	UObject* Object,
	FByteBulkData& BulkData,
	FArchive& Ar,
	TFunctionRef<void(FArchive&)> Serialize)
{
	SerializeBulkData(
		Object,
		BulkData,
		Ar,
		[&]
		{
			FBulkDataWriter Writer(BulkData, true);
			Serialize(Writer);
		},
		[&]
		{
			FBulkDataReader Reader(BulkData, true);
			Serialize(Reader);
			ensure(!Reader.IsError() && Reader.AtEnd());
		});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint64 FVoxelObjectUtilities::HashProperty(const FProperty& Property, const void* ContainerPtr)
{
#if VOXEL_ENGINE_VERSION < 501
#define CASE(Type) if (Property.IsA<Type>()) { return FVoxelUtilities::MurmurHash(CastFieldChecked<Type>(Property).GetPropertyValue_InContainer(ContainerPtr)); }
#else
	switch (Property.GetCastFlags())
	{
#define CASE(Type) case Type::StaticClassCastFlags(): return FVoxelUtilities::MurmurHash(CastFieldChecked<Type>(Property).GetPropertyValue_InContainer(ContainerPtr));
#endif

	CASE(FBoolProperty);
	CASE(FByteProperty);
	CASE(FIntProperty);
	CASE(FFloatProperty);
	CASE(FDoubleProperty);
	CASE(FUInt16Property);
	CASE(FUInt32Property);
	CASE(FUInt64Property);
	CASE(FInt8Property);
	CASE(FInt16Property);
	CASE(FInt64Property);
	CASE(FClassProperty);
	CASE(FNameProperty);
	CASE(FObjectProperty);
	CASE(FObjectPtrProperty);
	CASE(FWeakObjectProperty);
#undef CASE

#if VOXEL_ENGINE_VERSION >= 501
	default: break;
	}
#endif

	if (Property.IsA<FStructProperty>())
	{
		const UScriptStruct* Struct = CastFieldChecked<FStructProperty>(Property).Struct;

#define CASE(Type) \
		if (Struct == TBaseStructure<Type>::Get()) \
		{ \
			return FVoxelUtilities::MurmurHash(*Property.ContainerPtrToValuePtr<Type>(ContainerPtr)); \
		}

		CASE(FVector2D);
		CASE(FVector);
		CASE(FVector4);
		CASE(FColor);
		CASE(FLinearColor);
		CASE(FMatrix);

#undef CASE
	}

	ensure(false);
	return 0;
}

void FVoxelObjectUtilities::AddStructReferencedObjects(FReferenceCollector& Collector, const UScriptStruct* Struct, void* StructMemory)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(Struct))
	{
		return;
	}

	if (Struct->StructFlags & STRUCT_AddStructReferencedObjects)
	{
		Struct->GetCppStructOps()->AddStructReferencedObjects()(StructMemory, Collector);
	}

	for (TPropertyValueIterator<const FObjectProperty> It(Struct, StructMemory); It; ++It)
	{
		UObject** ObjectPtr = static_cast<UObject**>(VOXEL_CONST_CAST(It.Value()));
		Collector.AddReferencedObject(*ObjectPtr);
	}
}

TUniquePtr<FBoolProperty> FVoxelObjectUtilities::MakeBoolProperty()
{
	TUniquePtr<FBoolProperty> Property = MakeUnique<FBoolProperty>(FFieldVariant(), FName(), EObjectFlags());
	Property->ElementSize = sizeof(bool);
	return Property;
}

TUniquePtr<FFloatProperty> FVoxelObjectUtilities::MakeFloatProperty()
{
	TUniquePtr<FFloatProperty> Property = MakeUnique<FFloatProperty>(FFieldVariant(), FName(), EObjectFlags());
	Property->ElementSize = sizeof(float);
	return Property;
}

TUniquePtr<FIntProperty> FVoxelObjectUtilities::MakeIntProperty()
{
	TUniquePtr<FIntProperty> Property = MakeUnique<FIntProperty>(FFieldVariant(), FName(), EObjectFlags());
	Property->ElementSize = sizeof(int32);
	return Property;
}

TUniquePtr<FNameProperty> FVoxelObjectUtilities::MakeNameProperty()
{
	TUniquePtr<FNameProperty> Property = MakeUnique<FNameProperty>(FFieldVariant(), FName(), EObjectFlags());
	Property->ElementSize = sizeof(FName);
	return Property;
}

TUniquePtr<FEnumProperty> FVoxelObjectUtilities::MakeEnumProperty(const UEnum* Enum)
{
	TUniquePtr<FEnumProperty> Property = MakeUnique<FEnumProperty>(FFieldVariant(), FName(), EObjectFlags());
	Property->SetEnum(VOXEL_CONST_CAST(Enum));
	Property->ElementSize = sizeof(uint8);
	return Property;
}

TUniquePtr<FStructProperty> FVoxelObjectUtilities::MakeStructProperty(const UScriptStruct* Struct)
{
	check(Struct);

	TUniquePtr<FStructProperty> Property = MakeUnique<FStructProperty>(FFieldVariant(), FName(), EObjectFlags());
	Property->Struct = VOXEL_CONST_CAST(Struct);
	Property->ElementSize = Struct->GetStructureSize();
	return Property;
}

TUniquePtr<FObjectProperty> FVoxelObjectUtilities::MakeObjectProperty(const UClass* Class)
{
	check(Class);

	TUniquePtr<FObjectProperty> Property = MakeUnique<FObjectProperty>(FFieldVariant(), FName(), EObjectFlags());
	Property->PropertyClass = VOXEL_CONST_CAST(Class);
	Property->ElementSize = sizeof(UObject*);
	return Property;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TArray<UScriptStruct*> GetDerivedStructs(const UScriptStruct* StructType, bool bIncludeBase)
{
	VOXEL_FUNCTION_COUNTER_LLM();

	TArray<UScriptStruct*> Result;
	for (TObjectIterator<UScriptStruct> StructIt; StructIt; ++StructIt)
	{
		if (!StructIt->IsChildOf(StructType))
		{
			continue;
		}
		if (!bIncludeBase &&
			StructType == *StructIt)
		{
			continue;
		}

		Result.Add(*StructIt);
	}
	return Result;
}

bool IsFunctionInput(const FProperty& Property)
{
	ensure(Property.Owner.IsA(UFunction::StaticClass()));

	return
		!Property.HasAnyPropertyFlags(CPF_ReturnParm) &&
		(!Property.HasAnyPropertyFlags(CPF_OutParm) || Property.HasAnyPropertyFlags(CPF_ReferenceParm));
}

class FRestoreClassInfo
{
public:
	static const TMap<FName, UFunction*>& GetFunctionMap(const UClass* Class)
	{
		return Class->FuncMap;
	}
};

TArray<UFunction*> GetClassFunctions(const UClass* Class, bool bIncludeSuper)
{
	TArray<UFunction*> Functions;
	FRestoreClassInfo::GetFunctionMap(Class).GenerateValueArray(Functions);

	if (bIncludeSuper)
	{
		const UClass* SuperClass = Class->GetSuperClass();
		while (SuperClass)
		{
			for (auto& It : FRestoreClassInfo::GetFunctionMap(SuperClass))
			{
				Functions.Add(It.Value);
			}
			SuperClass = Class->GetSuperClass();
		}
	}

	return Functions;
}

#if WITH_EDITOR
FString GetStringMetaDataHierarchical(const UStruct* Struct, FName Name)
{
	FString Result;
	Struct->GetStringMetaDataHierarchical(Name, &Result);
	return Result;
}
#endif