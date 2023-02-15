// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelPinType.h"
#include "VoxelPinValue.generated.h"

VOXEL_FWD_NAMESPACE_CLASS(FVoxelMetaGraphVariableCollectionCustomization, MetaGraph, FVariableCollectionCustomization);
VOXEL_FWD_NAMESPACE_CLASS(FVoxelMetaGraphPinValueCustomization, MetaGraph, FPinValueCustomization);

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelPinValue
{
	GENERATED_BODY()
		
private:
	UPROPERTY(EditAnywhere, Category = "Config")
	FVoxelPinType Type;

	UPROPERTY(EditAnywhere, Category = "Config")
	bool bBool = false;

	UPROPERTY(EditAnywhere, Category = "Config")
	uint8 Byte = 0;

	UPROPERTY(EditAnywhere, Category = "Config")
	float Float = 0.f;

	UPROPERTY(EditAnywhere, Category = "Config")
	int32 Int32 = 0;

	UPROPERTY(EditAnywhere, Category = "Config")
	FName Name;
	
	UPROPERTY(EditAnywhere, Category = "Config")
	int64 Enum = 0;

	UPROPERTY(EditAnywhere, Category = "Config")
	FVoxelInstancedStruct Struct;
	
	UPROPERTY(EditAnywhere, Category = "Config")
	TSoftObjectPtr<UObject> Object;

public:
	FVoxelPinValue() = default;
	explicit FVoxelPinValue(const FVoxelPinType& Type);

	static FVoxelPinValue MakeFromPinDefaultValue(const UEdGraphPin& Pin);
	void ApplyToPinDefaultValue(UEdGraphPin& Pin) const;

	FString ExportToString() const;
	bool ImportFromString(const FString& Value);
	bool ImportFromUnrelated(FVoxelPinValue Other);

	FVoxelPinValue WithoutTag() const;

public:
	FORCEINLINE bool IsValid() const
	{
		return Type.IsValid();
	}

public:
	FORCEINLINE const FVoxelPinType& GetType() const
	{
		return Type;
	}
	template<typename T>
	FORCEINLINE bool Is() const
	{
		return Type.Is<T>();
	}
	template<typename T>
	FORCEINLINE bool IsDerivedFrom() const
	{
		return Type.IsDerivedFrom<T>();
	}
	FORCEINLINE bool IsDerivedFrom(const FVoxelPinType& Other) const
	{
		return Type.IsDerivedFrom(Other);
	}
	FORCEINLINE bool IsBuffer() const
	{
		return Type.IsBuffer();
	}
	
public:
	template<typename T>
	FORCEINLINE T& Get()
	{
		checkVoxelSlow(Type.WithoutTag().IsDerivedFrom<T>());
		checkStatic(!TIsSame<T, FVoxelPinType>::Value);
		checkStatic(!TIsSame<T, FVoxelPinValue>::Value);

		using PropertyType = FVoxelObjectUtilities::TPropertyType<T>;

		if constexpr (TIsSame<PropertyType, FBoolProperty>::Value)
		{
			return bBool;
		}
		else if constexpr (TIsSame<PropertyType, FByteProperty>::Value)
		{
			return Byte;
		}
		else if constexpr (TIsSame<PropertyType, FFloatProperty>::Value)
		{
			return Float;
		}
		else if constexpr (TIsSame<PropertyType, FIntProperty>::Value)
		{
			return Int32;
		}
		else if constexpr (TIsSame<PropertyType, FNameProperty>::Value)
		{
			return Name;
		}
		else if constexpr (TIsSame<PropertyType, FEnumProperty>::Value)
		{
			return reinterpret_cast<T&>(Enum);
		}
		else if constexpr (TIsSame<PropertyType, FStructProperty>::Value)
		{
			return Struct.Get<T>();
		}
		else if constexpr (TIsSame<PropertyType, FSoftObjectProperty>::Value)
		{
			return ReinterpretCastRef<T&>(Object);
		}
		else
		{
			checkStatic(TIsSame<PropertyType, void>::Value);
			check(false);
			return *this;
		}
	}
	FORCEINLINE int64& GetEnum()
	{
		checkVoxelSlow(Type.IsEnum());
		return Enum;
	}
	FORCEINLINE FVoxelInstancedStruct& GetStruct()
	{
		checkVoxelSlow(Type.IsStruct());
		checkVoxelSlow(Struct.GetScriptStruct()->IsChildOf(Type.GetStruct()));
		return Struct;
	}
	FORCEINLINE TSoftObjectPtr<UObject>& GetObject()
	{
		checkVoxelSlow(Type.IsObject());
		return Object;
	}

public:
	template<typename T>
	FORCEINLINE const T& Get() const
	{
		return VOXEL_CONST_CAST(this)->Get<T>();
	}
	FORCEINLINE const int64& GetEnum() const
	{
		return VOXEL_CONST_CAST(this)->GetEnum();
	}
	FORCEINLINE const FVoxelInstancedStruct& GetStruct() const
	{
		return VOXEL_CONST_CAST(this)->GetStruct();
	}
	FORCEINLINE const TSoftObjectPtr<UObject>& GetObject() const
	{
		return VOXEL_CONST_CAST(this)->GetObject();
	}

public:
	template<typename T>
	static FVoxelPinValue Make(const T& Value = FVoxelUtilities::MakeSafe<T>())
	{
		checkStatic(!TIsSame<T, FVoxelPinType>::Value);
		checkStatic(!TIsSame<T, FVoxelPinValue>::Value);

		FVoxelPinValue Result(FVoxelPinType::Make<T>());
		Result.Get<T>() = Value;
		return Result;
	}

	static FVoxelPinValue MakeStruct(FVoxelInstancedStruct&& Struct)
	{
		FVoxelPinValue Result;
		Result.Type = FVoxelPinType::MakeStruct(Struct.GetScriptStruct());
		Result.Struct = MoveTemp(Struct);
		return Result;
	}

public:
	bool operator==(const FVoxelPinValue& Other) const;
	bool operator!=(const FVoxelPinValue& Other) const
	{
		return !(*this == Other);
	}

	friend FVoxelMetaGraphPinValueCustomization;
	friend FVoxelMetaGraphVariableCollectionCustomization;
	friend class FVoxelMetaGraphLocalVariableDeclarationNodeCustomization;
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelCustomHash : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	virtual uint64 GetHash() const VOXEL_PURE_VIRTUAL({});
};