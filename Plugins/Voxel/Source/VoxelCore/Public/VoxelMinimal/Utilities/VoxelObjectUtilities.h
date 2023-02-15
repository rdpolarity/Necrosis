// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "Templates/Casts.h"
#include "Templates/SubclassOf.h"
#include "UObject/UObjectHash.h"
#include "Serialization/BulkData.h"
#include "VoxelMinimal/Utilities/VoxelBaseUtilities.h"

template<typename FieldType>
FORCEINLINE FieldType* CastField(FField& Src)
{
	return CastField<FieldType>(&Src);
}
template<typename FieldType>
FORCEINLINE const FieldType* CastField(const FField& Src)
{
	return CastField<FieldType>(&Src);
}

template<typename FieldType>
FORCEINLINE FieldType& CastFieldChecked(FField& Src)
{
	return *CastFieldChecked<FieldType>(&Src);
}
template<typename FieldType>
FORCEINLINE const FieldType& CastFieldChecked(const FField& Src)
{
	return *CastFieldChecked<FieldType>(&Src);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename ToType, typename FromType, typename Allocator, typename = typename TEnableIf<TIsDerivedFrom<
	typename TRemoveConst<ToType>::Type,
	typename TRemoveConst<FromType>::Type
>::Value>::Type>
FORCEINLINE const TArray<ToType*, Allocator>& CastChecked(const TArray<FromType*, Allocator>& Array)
{
#if VOXEL_DEBUG
	for (FromType* Element : Array)
	{
		checkVoxelSlow(Element->template IsA<ToType>());
	}
#endif

	return ReinterpretCastArray<ToType*>(Array);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename T>
struct TIsSoftObjectPtr
{
	static constexpr bool Value = false;
};

template<>
struct TIsSoftObjectPtr<FSoftObjectPtr>
{
	static constexpr bool Value = true;
};

template<typename T>
struct TIsSoftObjectPtr<TSoftObjectPtr<T>>
{
	static constexpr bool Value = true;
};

template<typename T>
struct TSoftObjectPtrClass;

template<typename T>
struct TSoftObjectPtrClass<TSoftObjectPtr<T>>
{
	using Type = T;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct VOXELCORE_API FVoxelObjectUtilities
{
#if WITH_EDITOR
	static FString GetClassDisplayName_EditorOnly(const UClass* Class);
	static FString GetPropertyTooltip(const UFunction& Function, const FProperty& Property);
	static FString GetPropertyTooltip(const FString& FunctionTooltip, const FString& PropertyName, bool bIsReturnPin);

	static TMap<FName, FString> GetMetadata(const UObject* Object);

	// Will let the user rename the asset in the content browser
	static void CreateNewAsset_Deferred(UClass* Class, const FString& BaseName, const FString& Suffix, TFunction<void(UObject*)> SetupObject);

	template<typename T>
	static void CreateNewAsset_Deferred(const FString& BaseName, const FString& Suffix, TFunction<void(T*)> SetupObject)
	{
		FVoxelObjectUtilities::CreateNewAsset_Deferred(T::StaticClass(), BaseName, Suffix, [=](UObject* Object) { SetupObject(CastChecked<T>(Object)); });
	}
	
	static UObject* CreateNewAsset_Direct(UClass* Class, const FString& BaseName, const FString& Suffix);

	template<typename T>
	static T* CreateNewAsset_Direct(const FString& BaseName, const FString& Suffix)
	{
		return CastChecked<T>(FVoxelObjectUtilities::CreateNewAsset_Direct(T::StaticClass(), BaseName, Suffix), ECastCheckedType::NullAllowed);
	}
#endif

	static void InvokeFunctionWithNoParameters(UObject* Object, UFunction* Function);

public:
	static bool PropertyFromText_Direct(const FProperty& Property, const FString& Text, void* Data, UObject* Owner);
	template<typename T>
	static bool PropertyFromText_Direct(const FProperty& Property, const FString& Text, T* Data, UObject* Owner) = delete;

	static bool PropertyFromText_InContainer(const FProperty& Property, const FString& Text, void* ContainerData, UObject* Owner);
	template<typename T>
	static bool PropertyFromText_InContainer(const FProperty& Property, const FString& Text, T* ContainerData, UObject* Owner) = delete;

	static bool PropertyFromText_InContainer(const FProperty& Property, const FString& Text, UObject* Owner);

public:
	static FString PropertyToText_Direct(const FProperty& Property, const void* Data, const UObject* Owner);
	template<typename T>
	static FString PropertyToText_Direct(const FProperty& Property, const T* Data, const UObject* Owner) = delete;

	static FString PropertyToText_InContainer(const FProperty& Property, const void* ContainerData, const UObject* Owner);
	template<typename T>
	static FString PropertyToText_InContainer(const FProperty& Property, const T* ContainerData, const UObject* Owner) = delete;

	static FString PropertyToText_InContainer(const FProperty& Property, const UObject* Owner);

public:
	static bool ShouldSerializeBulkData(FArchive& Ar);
	
	VOXEL_GENERATE_MEMBER_FUNCTION_CHECK(Serialize);

	static void SerializeBulkData(
		UObject* Object,
		FByteBulkData& BulkData,
		FArchive& Ar,
		TFunctionRef<void()> SaveBulkData,
		TFunctionRef<void()> LoadBulkData);

	static void SerializeBulkData(
		UObject* Object, 
		FByteBulkData& BulkData, 
		FArchive& Ar, 
		TFunctionRef<void(FArchive&)> Serialize);

	template<typename T>
	static void SerializeBulkData(
		UObject* Object, 
		FByteBulkData& BulkData, 
		FArchive& Ar, 
		T& Data)
	{
		FVoxelObjectUtilities::SerializeBulkData(Object, BulkData, Ar, [&](FArchive& BulkDataAr)
		{
			if constexpr (THasMemberFunction_Serialize<T>::Value)
			{
				Data.Serialize(BulkDataAr);
			}
			else
			{
				BulkDataAr << Data;
			};
		});
	}

public:
	static uint64 HashProperty(const FProperty& Property, const void* ContainerPtr);
	static void AddStructReferencedObjects(FReferenceCollector& Collector, const UScriptStruct* Struct, void* StructMemory);

	static TUniquePtr<FBoolProperty> MakeBoolProperty();
	static TUniquePtr<FFloatProperty> MakeFloatProperty();
	static TUniquePtr<FIntProperty> MakeIntProperty();
	static TUniquePtr<FNameProperty> MakeNameProperty();

	static TUniquePtr<FEnumProperty> MakeEnumProperty(const UEnum* Enum);
	static TUniquePtr<FStructProperty> MakeStructProperty(const UScriptStruct* Struct);
	static TUniquePtr<FObjectProperty> MakeObjectProperty(const UClass* Class);
	
	template<typename T>
	static TUniquePtr<FStructProperty> MakeEnumProperty()
	{
		return FVoxelObjectUtilities::MakeEnumProperty(StaticEnum<T>());
	}
	template<typename T>
	static TUniquePtr<FStructProperty> MakeStructProperty()
	{
		return FVoxelObjectUtilities::MakeStructProperty(TBaseStructure<T>::Get());
	}
	template<typename T>
	static TUniquePtr<FStructProperty> MakeObjectProperty()
	{
		return FVoxelObjectUtilities::MakeObjectProperty(T::StaticClass());
	}

public:
	template<typename T>
	using TPropertyType = typename TMultiChooseClass<
		TIsSame<T, bool>          , FBoolProperty  ,
		TIsSame<T, uint8>         , FByteProperty  ,
		TIsSame<T, float>         , FFloatProperty ,
		TIsSame<T, double>        , FDoubleProperty,
		TIsSame<T, int32>         , FIntProperty   ,
		TIsSame<T, int64>         , FInt64Property ,
		TIsSame<T, FName>         , FNameProperty  ,
		TIsEnum<T>                , FEnumProperty  ,
		TIsDerivedFrom<T, UObject>, FObjectProperty,
		TIsSoftObjectPtr<T>       , FSoftObjectProperty,
		TIsTSubclassOf<T>         , FClassProperty ,
									FStructProperty
	>::Type;

	template<typename T>
	static bool MatchesProperty(const FProperty& Property)
	{
		return FVoxelObjectUtilities::MatchesPropertyImpl(Property, *reinterpret_cast<T*>(-1));
	}

private:
	template<typename T>
	static bool MatchesPropertyImpl(const FProperty& Property, T&)
	{
		using PropertyType = TPropertyType<T>;

		if (!Property.IsA<PropertyType>())
		{
			return false;
		}
		
		if constexpr (TIsSame<PropertyType, FEnumProperty>::Value)
		{
			return CastFieldChecked<FEnumProperty>(Property).GetEnum() == StaticEnum<T>();
		}
		else if constexpr (TIsSame<PropertyType, FStructProperty>::Value)
		{
			return CastFieldChecked<FStructProperty>(Property).Struct == TBaseStructure<T>::Get();
		}
		else if constexpr (TIsSame<PropertyType, FObjectProperty>::Value)
		{
			return CastFieldChecked<FObjectProperty>(Property).PropertyClass == T::StaticClass();
		}
		else if constexpr (TIsSame<PropertyType, FSoftObjectProperty>::Value)
		{
			return CastFieldChecked<FSoftObjectProperty>(Property).PropertyClass == T::StaticClass();
		}
		else
		{
			return true;
		}
	}
	template<typename T>
	static bool MatchesPropertyImpl(const FProperty& Property, T*&)
	{
		checkStatic(TIsDerivedFrom<T, UObject>::Value);
		return MatchesProperty<T>(Property);
	}
	template<typename T>
	static bool MatchesPropertyImpl(const FProperty& Property, TArray<T>&)
	{
		return
			Property.IsA<FArrayProperty>() &&
			MatchesProperty<T>(*CastFieldChecked<FArrayProperty>(Property).Inner);
	}
	template<typename T>
	static bool MatchesPropertyImpl(const FProperty& Property, TSet<T>&)
	{
		return
			Property.IsA<FSetProperty>() &&
			MatchesProperty<T>(*CastFieldChecked<FSetProperty>(Property).ElementProp);
	}
	template<typename Key, typename Value>
	static bool MatchesPropertyImpl(const FProperty& Property, TMap<Key, Value>&)
	{
		return
			Property.IsA<FMapProperty>() &&
			MatchesProperty<Key>(*CastFieldChecked<FMapProperty>(Property).KeyProp) &&
			MatchesProperty<Value>(*CastFieldChecked<FMapProperty>(Property).ValueProp);
	}
};

template<typename T, typename LambdaType>
void ForEachObjectOfClass(LambdaType&& Operation, bool bIncludeDerivedClasses = true, EObjectFlags ExcludeFlags = RF_ClassDefaultObject, EInternalObjectFlags ExclusionInternalFlags = EInternalObjectFlags::None)
{
	::ForEachObjectOfClass(T::StaticClass(), [&](UObject* Object)
	{
		checkVoxelSlow(Object && Object->IsA<T>());
		Operation(static_cast<T*>(Object));
	}, bIncludeDerivedClasses, ExcludeFlags, ExclusionInternalFlags);
}

template<typename T = void, typename ArrayType = typename TChooseClass<TIsSame<T, void>::Value, UClass*, TSubclassOf<T>>::Result>
TArray<ArrayType> GetDerivedClasses(const UClass* Class = T::StaticClass(), bool bRecursive = true, bool bRemoveDeprecated = true)
{
	VOXEL_FUNCTION_COUNTER_LLM();

	TArray<UClass*> Result;
	GetDerivedClasses(Class, Result, bRecursive);

	if (bRemoveDeprecated)
	{
		Result.RemoveAllSwap([](const UClass* Class)
		{
			return Class->HasAnyClassFlags(CLASS_Deprecated);
		});
	}

	return ReinterpretCastArray<ArrayType>(MoveTemp(Result));
}

VOXELCORE_API TArray<UScriptStruct*> GetDerivedStructs(const UScriptStruct* StructType, bool bIncludeBase = false);

template<typename T, bool bIncludeBase = false>
TArray<UScriptStruct*> GetDerivedStructs()
{
	return ::GetDerivedStructs(T::StaticStruct(), bIncludeBase);
}

VOXELCORE_API bool IsFunctionInput(const FProperty& Property);
VOXELCORE_API TArray<UFunction*> GetClassFunctions(const UClass* Class, bool bIncludeSuper = false);

#if WITH_EDITOR
VOXELCORE_API FString GetStringMetaDataHierarchical(const UStruct* Struct, FName Name);
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename T>
struct TVoxelPropertiesIterator
{
public:
	TVoxelPropertiesIterator() = default;
	FORCEINLINE explicit TVoxelPropertiesIterator(const UStruct& Struct)
		: Iterator(&Struct)
	{
		FindNextProperty();
	}

	FORCEINLINE FProperty& operator*() const
	{
		FProperty& Property = VOXEL_CONST_CAST(**Iterator);

		if constexpr (!TIsSame<T, void>::Value)
		{
			checkVoxelSlow(FVoxelObjectUtilities::MatchesProperty<T>(Property));
		}

		return Property;
	}
	FORCEINLINE TVoxelPropertiesIterator& operator++()
	{
		++Iterator;
		FindNextProperty();
		return *this;
	}

	FORCEINLINE bool operator!=(decltype(nullptr)) const
	{
		return bool(Iterator);
	}

private:
	TFieldIterator<FProperty> Iterator;

	FORCEINLINE void FindNextProperty()
	{
		if constexpr (!TIsSame<T, void>::Value)
		{
			while (bool(Iterator) && !FVoxelObjectUtilities::MatchesProperty<T>(**Iterator))
			{
				++Iterator;
			}
		}
	}
};

template<typename T>
struct TVoxelStructRangedFor
{
	const UStruct& Struct;

	FORCEINLINE TVoxelStructRangedFor(const UStruct& Struct)
		: Struct(Struct)
	{
	}

	FORCEINLINE FProperty* First() const
	{
		return Struct.PropertyLink;
	}

	FORCEINLINE TVoxelPropertiesIterator<T> begin() const
	{
		return TVoxelPropertiesIterator<T>(Struct);
	}
	FORCEINLINE decltype(nullptr) end() const
	{
		return nullptr;
	}

	int32 Num() const
	{
		int32 Result = 0;
		for (FProperty& Property : *this)
		{
			Result++;
		}
		return Result;
	}
	TArray<FProperty*> Array() const
	{
		TArray<FProperty*> Result;
		for (FProperty& Property : *this)
		{
			Result.Add(&Property);
		}
		return Result;
	}
};

template<typename T = void>
FORCEINLINE TVoxelStructRangedFor<T> GetStructProperties(const UStruct& Struct)
{
	return TVoxelStructRangedFor<T>(Struct);
}
template<typename T = void>
FORCEINLINE TVoxelStructRangedFor<T> GetStructProperties(const UStruct* Struct)
{
	check(Struct);
	return TVoxelStructRangedFor<T>(*Struct);
}

template<typename T = void>
FORCEINLINE TVoxelStructRangedFor<T> GetClassProperties(const UClass& Class)
{
	return TVoxelStructRangedFor<T>(Class);
}
template<typename T = void>
FORCEINLINE TVoxelStructRangedFor<T> GetClassProperties(const UClass* Class)
{
	check(Class);
	return TVoxelStructRangedFor<T>(*Class);
}

template<typename T = void>
FORCEINLINE TVoxelStructRangedFor<T> GetFunctionProperties(const UFunction& Function)
{
	return TVoxelStructRangedFor<T>(Function);
}
template<typename T = void>
FORCEINLINE TVoxelStructRangedFor<T> GetFunctionProperties(const UFunction* Function)
{
	check(Function);
	return TVoxelStructRangedFor<T>(*Function);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename T>
struct TVoxelPropertyValueIterator
{
public:
	TVoxelPropertyValueIterator() = default;
	FORCEINLINE TVoxelPropertyValueIterator(const UStruct& Struct, void* StructMemory)
		: Iterator(Struct)
		, StructMemory(StructMemory)
	{
	}
	
	FORCEINLINE FProperty& Key() const
	{
		return *Iterator;
	}
	FORCEINLINE T& Value() const
	{
		const FProperty& Property = *Iterator;
		checkVoxelSlow(FVoxelObjectUtilities::MatchesProperty<T>(Property));
		return *Property.ContainerPtrToValuePtr<T>(StructMemory);
	}

	FORCEINLINE const TVoxelPropertyValueIterator& operator*() const
	{
		checkVoxelSlow(FVoxelObjectUtilities::MatchesProperty<T>(*Iterator));
		return *this;
	}
	FORCEINLINE TVoxelPropertyValueIterator& operator++()
	{
		++Iterator;
		return *this;
	}

	FORCEINLINE bool operator!=(decltype(nullptr)) const
	{
		return Iterator != nullptr;
	}

private:
	TVoxelPropertiesIterator<T> Iterator;
	void* StructMemory = nullptr;
};

template<typename T>
struct TVoxelPropertyValueIteratorRangedFor
{
	const UStruct& Struct;
	void* const StructMemory;

	FORCEINLINE TVoxelPropertyValueIteratorRangedFor(const UStruct& Struct, void* StructMemory)
		: Struct(Struct)
		, StructMemory(StructMemory)
	{
	}

	FORCEINLINE const FProperty* First() const
	{
		return Struct.PropertyLink;
	}

	FORCEINLINE TVoxelPropertyValueIterator<T> begin() const
	{
		return TVoxelPropertyValueIterator<T>(Struct, StructMemory);
	}
	FORCEINLINE decltype(nullptr) end() const
	{
		return nullptr;
	}
};

template<typename T>
FORCEINLINE TVoxelPropertyValueIteratorRangedFor<T> CreatePropertyValueIterator(const UStruct* Struct, void* StructMemory)
{
	check(Struct);
	check(StructMemory);
	return TVoxelPropertyValueIteratorRangedFor<T>(*Struct, StructMemory);
}
template<typename T>
FORCEINLINE TVoxelPropertyValueIteratorRangedFor<const T> CreatePropertyValueIterator(const UStruct* Struct, const void* StructMemory)
{
	return CreatePropertyValueIterator<const T>(Struct, VOXEL_CONST_CAST(StructMemory));
}

template<typename T>
FORCEINLINE TVoxelPropertyValueIteratorRangedFor<T> CreatePropertyValueIterator(UObject* Object)
{
	return CreatePropertyValueIterator<T>(Object->GetClass(), Object);
}
template<typename T>
FORCEINLINE TVoxelPropertyValueIteratorRangedFor<const T> CreatePropertyValueIterator(const UObject* Object)
{
	return CreatePropertyValueIterator<T>(Object->GetClass(), Object);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename EnumType>
const TArray<EnumType>& GetEnumValues()
{
	static TArray<EnumType> Values;
	if (Values.Num() == 0)
	{
		const UEnum* Enum = StaticEnum<EnumType>();
		// -1 for MAX
		for (int32 Index = 0; Index < Enum->NumEnums() - 1; Index++)
		{
			Values.Add(EnumType(Enum->GetValueByIndex(Index)));
		}

	}
	return Values;
}

template<typename Class>
FName GetClassFName()
{
	return Class::StaticClass()->GetFName();
}
template<typename Class>
FString GetClassName()
{
	return Class::StaticClass()->GetName();
}

template<typename T>
FProperty& FindFPropertyChecked_Impl(const FName Name)
{
	UStruct* Struct;
	if constexpr (TIsDerivedFrom<T, UObject>::Value)
	{
		Struct = UObject::StaticClass();
	}
	else
	{
		Struct = TBaseStructure<T>::Get();
	}

	FProperty* Property = FindFProperty<FProperty>(Struct, Name);
	check(Property);
	return *Property;
}

#define FindFPropertyChecked(Class, Name) FindFPropertyChecked_Impl<Class>(GET_MEMBER_NAME_CHECKED(Class, Name))

#define FindUFunctionChecked(Class, Name) \
	[] \
	{ \
		static UFunction* Function = Class::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(Class, Name)); \
		check(Function); \
		return Function; \
	}()

class FVoxelFunctionParameterData
{
public:
	explicit FVoxelFunctionParameterData(const UFunction& Function)
		: Function(Function)
		, Data(FMemory::Malloc(FMath::Max(1, Function.GetStructureSize())))
	{
		Function.InitializeStruct(Data);
	}
	~FVoxelFunctionParameterData()
	{
		Function.DestroyStruct(Data);
		FMemory::Free(Data);
	}

	FORCEINLINE void* GetData()
	{
		return Data;
	}

private:
	const UFunction& Function;
	void* const Data;
};