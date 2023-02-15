// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "VoxelMinimal/VoxelDuplicateTransient.h"
#include "VoxelVirtualStruct.generated.h"

USTRUCT()
struct VOXELCORE_API FVoxelVirtualStruct
{
	GENERATED_BODY()

public:
	FVoxelVirtualStruct() = default;
	virtual ~FVoxelVirtualStruct() = default;

	virtual FString Internal_GetMacroName() const;
	virtual UScriptStruct* Internal_GetStruct() const
	{
		return StaticStruct();
	}

	// Only called if this is stored in an instanced struct
	virtual void PreSerialize() {}
	virtual void PostSerialize() {}

	virtual int64 GetAllocatedSize() const
	{
		return GetStruct()->GetStructureSize();
	}

	FORCEINLINE UScriptStruct* GetStruct() const
	{
		if (!PrivateStruct.Get())
		{
			PrivateStruct = Internal_GetStruct();
		}
		checkVoxelSlow(PrivateStruct.Get() == Internal_GetStruct());
		return PrivateStruct.Get();
	}

	template<typename T, typename = typename TEnableIf<TIsDerivedFrom<T, FVoxelVirtualStruct>::Value>::Type>
	FORCEINLINE bool IsA() const
	{
		return GetStruct()->IsChildOf(T::StaticStruct());
	}
	template<typename T, typename = typename TEnableIf<TIsDerivedFrom<T, FVoxelVirtualStruct>::Value>::Type>
	T* As()
	{
		if (!IsA<T>())
		{
			return nullptr;
		}

		return static_cast<T*>(this);
	}
	template<typename T, typename = typename TEnableIf<TIsDerivedFrom<T, FVoxelVirtualStruct>::Value>::Type>
	const T* As() const
	{
		if (!IsA<T>())
		{
			return nullptr;
		}

		return static_cast<const T*>(this);
	}

	TSharedRef<FVoxelVirtualStruct> MakeSharedCopy() const;

private:
	mutable TVoxelDuplicateTransient<UScriptStruct*> PrivateStruct;
};

#define GENERATED_VIRTUAL_STRUCT_BODY_ALIASES() \
	auto MakeSharedCopy() const -> decltype(auto) { return StaticCastSharedRef<VOXEL_THIS_TYPE>(Super::MakeSharedCopy()); } \
	template<typename T, typename = typename TEnableIf<!TIsReferenceType<T>::Value, TSharedRef<T>>::Type> \
	static auto MakeSharedCopy(T&& Data) -> decltype(auto) { return ::MakeSharedCopy(MoveTemp(Data)); } \
	template<typename T> \
	static auto MakeSharedCopy(const T& Data) -> decltype(auto) { return ::MakeSharedCopy(Data); }

#define GENERATED_VIRTUAL_STRUCT_BODY_IMPL(Parent) \
	virtual UScriptStruct* PREPROCESSOR_JOIN(Internal_GetStruct, Parent)() const override { return StaticStruct(); } \
	GENERATED_VIRTUAL_STRUCT_BODY_ALIASES()

#define DECLARE_VIRTUAL_STRUCT_PARENT(Parent, Macro) \
	virtual FString Internal_GetMacroName() const override \
	{ \
		return #Macro; \
	} \
	virtual UScriptStruct* Internal_GetStruct() const final override \
	{ \
		return Internal_GetStruct ## Parent(); \
	} \
	virtual UScriptStruct* Internal_GetStruct ## Parent() const \
	{ \
		return StaticStruct(); \
	} \
	GENERATED_VIRTUAL_STRUCT_BODY_ALIASES()

#define GENERATED_VIRTUAL_STRUCT_BODY() GENERATED_VIRTUAL_STRUCT_BODY_IMPL(PREPROCESSOR_NOTHING)

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename To>
FORCEINLINE To* Cast(FVoxelVirtualStruct* Struct)
{
	if (!Struct ||
		!Struct->IsA<To>())
	{
		return nullptr;
	}

	return static_cast<To*>(Struct);
}
template<typename To>
FORCEINLINE const To* Cast(const FVoxelVirtualStruct* Struct)
{
	return Cast<To>(VOXEL_CONST_CAST(Struct));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename To>
FORCEINLINE To* Cast(FVoxelVirtualStruct& Struct)
{
	return Cast<To>(&Struct);
}
template<typename To>
FORCEINLINE const To* Cast(const FVoxelVirtualStruct& Struct)
{
	return Cast<To>(&Struct);
}

template<typename To>
FORCEINLINE To& CastChecked(FVoxelVirtualStruct& Struct)
{
	checkVoxelSlow(Struct.IsA<To>());
	return static_cast<To&>(Struct);
}
template<typename To>
FORCEINLINE const To& CastChecked(const FVoxelVirtualStruct& Struct)
{
	return CastChecked<To>(VOXEL_CONST_CAST(Struct));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename To, typename From, typename = typename TEnableIf<TIsDerivedFrom<From, FVoxelVirtualStruct>::Value>::Type>
FORCEINLINE TSharedPtr<To> Cast(const TSharedPtr<From>& Struct)
{
	if (!Struct ||
		!Struct->template IsA<To>())
	{
		return nullptr;
	}

	return ::StaticCastSharedPtr<To>(Struct);
}
template<typename To, typename From, typename = typename TEnableIf<TIsDerivedFrom<From, FVoxelVirtualStruct>::Value>::Type>
FORCEINLINE TSharedPtr<const To> Cast(const TSharedPtr<const From>& Struct)
{
	if (!Struct ||
		!Struct->template IsA<To>())
	{
		return nullptr;
	}

	return ::StaticCastSharedPtr<const To>(Struct);
}

template<typename To, typename From, typename = typename TEnableIf<TIsDerivedFrom<From, FVoxelVirtualStruct>::Value>::Type>
FORCEINLINE TSharedPtr<To> Cast(const TSharedRef<From>& Struct)
{
	if (!Struct->template IsA<To>())
	{
		return nullptr;
	}

	return ::StaticCastSharedRef<To>(Struct);
}
template<typename To, typename From, typename = typename TEnableIf<TIsDerivedFrom<From, FVoxelVirtualStruct>::Value>::Type>
FORCEINLINE TSharedPtr<const To> Cast(const TSharedRef<const From>& Struct)
{
	if (!Struct->template IsA<To>())
	{
		return nullptr;
	}

	return ::StaticCastSharedRef<const To>(Struct);
}

template<typename To, typename From, typename = typename TEnableIf<TIsDerivedFrom<From, FVoxelVirtualStruct>::Value>::Type>
FORCEINLINE TSharedRef<To> CastChecked(const TSharedRef<From>& Struct)
{
	checkVoxelSlow(Struct->template IsA<To>());
	return ::StaticCastSharedRef<To>(Struct);
}
template<typename To, typename From, typename = typename TEnableIf<TIsDerivedFrom<From, FVoxelVirtualStruct>::Value>::Type>
FORCEINLINE TSharedRef<const To> CastChecked(const TSharedRef<const From>& Struct)
{
	checkVoxelSlow(Struct->template IsA<To>());
	return ::StaticCastSharedRef<const To>(Struct);
}

template<typename To, typename From, typename = typename TEnableIf<TIsDerivedFrom<From, FVoxelVirtualStruct>::Value>::Type>
FORCEINLINE TUniquePtr<To> CastChecked(TUniquePtr<From>&& Struct)
{
	checkVoxelSlow(Struct->template IsA<To>());
	return TUniquePtr<To>(static_cast<To*>(Struct.Release()));
}
template<typename To, typename From, typename = typename TEnableIf<TIsDerivedFrom<From, FVoxelVirtualStruct>::Value>::Type>
FORCEINLINE TUniquePtr<const To> CastChecked(TUniquePtr<const From>&& Struct)
{
	checkVoxelSlow(Struct->template IsA<To>());
	return TUniquePtr<const To>(static_cast<const To*>(Struct.Release()));
}