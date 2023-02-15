// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelPinValue.h"

struct VOXELMETAGRAPH_API FVoxelSharedPinValue
{
private:
	FVoxelPinType Type;

	bool bBool = false;
	uint8 Byte = 0;
	float Float = 0.f;
	int32 Int32 = 0;
	FName Name;
	int64 Enum = 0;

	// Inherit from FVoxelVirtualStruct for debug
	struct FOpaque : private FVoxelVirtualStruct {};

	TSharedPtr<const FOpaque> SharedStruct;
	UScriptStruct* SharedStructType = nullptr;

	static TSharedRef<FOpaque> MakeSharedStruct(UScriptStruct* Struct, const void* SourceStructMemory);
	void AllocateStruct(UScriptStruct* Struct, const void* SourceStructMemory);

public:
	FVoxelSharedPinValue() = default;

	explicit FVoxelSharedPinValue(const FVoxelPinType& Type);
	explicit FVoxelSharedPinValue(const FVoxelPinValue& Value);
	explicit FVoxelSharedPinValue(const FVoxelPinType& Type, TConstVoxelArrayView<uint8> View);

	int64 GetAllocatedSize() const;

	TConstVoxelArrayView<uint8> MakeByteView() const;
	FVoxelPinValue MakeValue() const;
	uint64 GetHash() const;

public:
	template<typename T>
	static FVoxelSharedPinValue Make(const T& Value = FVoxelUtilities::MakeSafe<T>(), const FVoxelPinType& TypeOverride = FVoxelPinType::Make<T>())
	{
		checkStatic(!TIsSame<T, FVoxelPinType>::Value);
		checkStatic(!TIsSame<T, FVoxelPinValue>::Value);
		checkStatic(!TIsSame<T, FVoxelSharedPinValue>::Value);
		checkVoxelSlow(TypeOverride.IsDerivedFrom<T>());

		FVoxelSharedPinValue Result(TypeOverride);
		VOXEL_CONST_CAST(Result.Get<T>()) = Value;
		return Result;
	}
	template<typename T>
	static FVoxelSharedPinValue Make(const TSharedRef<const T>& Value)
	{
		checkStatic(!TIsSame<T, FVoxelPinType>::Value);
		checkStatic(!TIsSame<T, FVoxelPinValue>::Value);
		checkStatic(!TIsSame<T, FVoxelSharedPinValue>::Value);

		FVoxelSharedPinValue Result;
		Result.Type = FVoxelPinType::Make<T>();
		Result.SharedStruct = ::ReinterpretCastRef<TSharedRef<const FOpaque>>(Value);
		Result.SharedStructType = TBaseStructure<T>::Get();

		if constexpr (TIsDerivedFrom<T, FVoxelVirtualStruct>::Value)
		{
			Result.Type = FVoxelPinType::MakeStruct(Value->GetStruct());
			Result.SharedStructType = Value->GetStruct();
		}

		return Result;
	}
	template<typename T>
	static FVoxelSharedPinValue Make(const TSharedRef<T>& Value)
	{
		return FVoxelSharedPinValue::Make<T>(::StaticCastSharedRef<const T>(Value));
	}

	template<typename T>
	TSharedPtr<const T> GetSharedStruct() const
	{
		checkVoxelSlow(IsDerivedFrom<T>());
		checkVoxelSlow(Type.IsStruct());

		if (!SharedStruct)
		{
			return nullptr;
		}
		checkVoxelSlow(SharedStructType);
		checkVoxelSlow(SharedStructType->IsChildOf(Type.GetStruct()));

		return ReinterpretCastRef<TSharedPtr<const T>>(SharedStruct);
	}
	template<typename T>
	TSharedPtr<T> GetSharedStructCopy() const
	{
		checkVoxelSlow(IsDerivedFrom<T>());
		checkVoxelSlow(Type.IsStruct());

		if (!SharedStruct)
		{
			return nullptr;
		}
		checkVoxelSlow(SharedStructType);
		checkVoxelSlow(SharedStructType->IsChildOf(Type.GetStruct()));

		return ReinterpretCastRef<TSharedRef<T>>(MakeSharedStruct(SharedStructType, SharedStruct.Get()));
	}

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
	
public:
	template<typename T>
	FORCEINLINE const T& Get() const
	{
		checkVoxelSlow(Type.IsDerivedFrom<T>());
		checkStatic(!TIsSame<T, FVoxelPinType>::Value);
		checkStatic(!TIsSame<T, FVoxelPinValue>::Value);
		checkStatic(!TIsSame<T, FVoxelSharedPinValue>::Value);

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
			return reinterpret_cast<const T&>(Enum);
		}
		else if constexpr (TIsSame<PropertyType, FStructProperty>::Value)
		{
			return reinterpret_cast<const T&>(*SharedStruct);
		}
		else
		{
			checkStatic(TIsSame<PropertyType, void>::Value);
			check(false);
			return *this;
		}
	}

public:
	bool operator==(const FVoxelSharedPinValue& Other) const;
	bool operator!=(const FVoxelSharedPinValue& Other) const
	{
		return !(*this == Other);
	}
};