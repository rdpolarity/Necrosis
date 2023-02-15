// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "EdGraph/EdGraphPin.h"
#include "VoxelPinType.generated.h"

struct FVoxelPinValue;
struct FVoxelExposedPinType;

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelPinType
{
	GENERATED_BODY()

private:
	UPROPERTY()
	FName PropertyClass;

	UPROPERTY()
	TObjectPtr<UObject> PropertyObject = nullptr;
	
	UPROPERTY()
	FName Tag;

public:
	FVoxelPinType() = default;
	FVoxelPinType(const FEdGraphPinType& PinType);
	explicit FVoxelPinType(const FProperty& Property);
	
public:
	template<typename T>
	static FVoxelPinType Make()
	{
		checkStatic(!TIsSame<T, FVoxelPinType>::Value);

		using PropertyType = FVoxelObjectUtilities::TPropertyType<T>;

		if constexpr (TIsSame<PropertyType, FEnumProperty>::Value)
		{
			return FVoxelPinType::MakeImpl<PropertyType>(StaticEnum<T>());
		}
		else if constexpr (TIsSame<PropertyType, FStructProperty>::Value)
		{
			return FVoxelPinType::MakeImpl<PropertyType>(TBaseStructure<T>::Get());
		}
		else if constexpr (TIsSame<PropertyType, FSoftObjectProperty>::Value)
		{
			return FVoxelPinType::MakeImpl<PropertyType>(TSoftObjectPtrClass<T>::Type::StaticClass());
		}
		else
		{
			return MakeImpl<PropertyType>();
		}
	}

	static FVoxelPinType MakeWildcard()
	{
		FVoxelPinType PinType;
		PinType.PropertyClass = STATIC_FNAME("wildcard");
		return PinType;
	}
	static FVoxelPinType MakeEnum(UEnum* Enum)
	{
		check(Enum);
		return MakeImpl<FEnumProperty>(Enum);
	}
	static FVoxelPinType MakeObject(UClass* Class)
	{
		check(Class);
		return MakeImpl<FSoftObjectProperty>(Class);
	}
	static FVoxelPinType MakeStruct(UScriptStruct* Struct)
	{
		check(Struct);
		return MakeImpl<FStructProperty>(Struct);
	}

private:
	template<typename T>
	static FVoxelPinType MakeImpl(UObject* InPropertyObject = nullptr)
	{
		checkStatic(TIsDerivedFrom<T, FProperty>::Value);

		FVoxelPinType PinType;
		PinType.PropertyClass = GetClassFName<T>();
		PinType.PropertyObject = InPropertyObject;
		check(PinType.IsValid());
		return PinType;
	}
	
public:
	bool IsValid() const;
	FString ToString() const;
	FString ToCppName() const;
	int32 GetTypeSize() const;
	FEdGraphPinType GetEdGraphPinType() const;
	bool IsDerivedFrom(const FVoxelPinType& Other) const;

public:
	bool IsBuffer() const;
	EPixelFormat GetPixelFormat() const;
	FVoxelPinType GetBufferType() const;
	FVoxelPinType GetInnerType() const;
	FVoxelPinType GetViewType() const;
	FVoxelPinType GetExposedType() const;
	const TVoxelInstancedStruct<FVoxelExposedPinType>& GetExposedTypeInfo() const;
	FVoxelPinValue MakeSafeDefault() const;

	static const TArray<FVoxelPinType>& GetAllBufferTypes();

public:
	FORCEINLINE bool HasTag() const
	{
		return !Tag.IsNone();
	}
	FORCEINLINE void SetTag(FName NewTag)
	{
		Tag = NewTag;
	}
	FORCEINLINE FName GetTag() const
	{
		return Tag;
	}

	FORCEINLINE FVoxelPinType WithTag(FName NewTag) const
	{
		FVoxelPinType Result = *this;
		Result.SetTag(NewTag);
		return Result;
	}
	FORCEINLINE FVoxelPinType WithoutTag() const
	{
		return WithTag({});
	}

public:
	template<typename T>
	FORCEINLINE bool Is() const
	{
		checkStatic(!TIsSame<T, FVoxelPinType>::Value);

		using PropertyType = FVoxelObjectUtilities::TPropertyType<T>;

		if (PropertyClass != GetClassFName<PropertyType>())
		{
			return false;
		}

		if constexpr (TIsSame<PropertyType, FEnumProperty>::Value)
		{
			return GetEnum() == StaticEnum<T>();
		}
		else if constexpr (TIsSame<PropertyType, FStructProperty>::Value)
		{
			return GetStruct() == TBaseStructure<T>::Get();
		}
		else if constexpr (TIsSame<PropertyType, FSoftObjectProperty>::Value)
		{
			return GetClass() == TSoftObjectPtrClass<T>::Type::StaticClass();
		}
		else
		{
			return true;
		}
	}

	template<typename T>
	FORCEINLINE bool IsDerivedFrom() const
	{
		return this->IsDerivedFrom(Make<T>());
	}

	FORCEINLINE bool IsWildcard() const
	{
		return PropertyClass == STATIC_FNAME("wildcard");
	}
	FORCEINLINE bool IsEnum() const
	{
		return PropertyClass == GetClassFName<FEnumProperty>();
	}
	FORCEINLINE bool IsStruct() const
	{
		return PropertyClass == GetClassFName<FStructProperty>();
	}
	FORCEINLINE bool IsObject() const
	{
		return PropertyClass == GetClassFName<FSoftObjectProperty>();
	}

	FORCEINLINE UEnum* GetEnum() const
	{
		check(IsEnum());
		return CastChecked<UEnum>(PropertyObject, ECastCheckedType::NullAllowed);
	}
	FORCEINLINE UScriptStruct* GetStruct() const
	{
		check(IsStruct());
		return CastChecked<UScriptStruct>(PropertyObject, ECastCheckedType::NullAllowed);
	}
	FORCEINLINE UClass* GetClass() const
	{
		check(IsObject());
		return CastChecked<UClass>(PropertyObject, ECastCheckedType::NullAllowed);
	}

public:
	FORCEINLINE bool operator==(const FVoxelPinType& Other) const
	{
		return
			PropertyClass == Other.PropertyClass &&
			PropertyObject == Other.PropertyObject &&
			Tag == Other.Tag;
	}
	FORCEINLINE bool operator!=(const FVoxelPinType& Other) const
	{
		return
			PropertyClass != Other.PropertyClass ||
			PropertyObject != Other.PropertyObject ||
			Tag != Other.Tag;
	}
	
	FORCEINLINE friend bool operator==(const FEdGraphPinType& PinTypeA, const FVoxelPinType& PinTypeB)
	{
		return PinTypeB == PinTypeA;
	}
	FORCEINLINE friend bool operator!=(const FEdGraphPinType& PinTypeA, const FVoxelPinType& PinTypeB)
	{
		return PinTypeB != PinTypeA;
	}

	FORCEINLINE friend uint32 GetTypeHash(const FVoxelPinType& Type)
	{
		return FVoxelUtilities::MurmurHashMulti(
			GetTypeHash(Type.PropertyClass),
			GetTypeHash(Type.PropertyObject),
			GetTypeHash(Type.Tag));
	}
};

class FVoxelPinTypeSet
{
public:
	FVoxelPinTypeSet() = default;

	static FVoxelPinTypeSet All()
	{
		FVoxelPinTypeSet Set;
		Set.bIsAll = true;
		return Set;
	}

	bool IsAll() const
	{
		return bIsAll;
	}
	const TSet<FVoxelPinType>& GetTypes() const
	{
		ensure(!IsAll());
		return Types;
	}

	void Add(const FVoxelPinType& Type)
	{
		Types.Add(Type);
	}
	void Add(const TConstArrayView<FVoxelPinType> InTypes)
	{
		for (const FVoxelPinType& Type : InTypes)
		{
			Types.Add(Type);
		}
	}
	template<typename T>
	void Add()
	{
		Types.Add(FVoxelPinType::Make<T>());
	}
	bool Contains(const FVoxelPinType& Type) const
	{
		return bIsAll || Types.Contains(Type);
	}

private:
	bool bIsAll = false;
	TSet<FVoxelPinType> Types;
};