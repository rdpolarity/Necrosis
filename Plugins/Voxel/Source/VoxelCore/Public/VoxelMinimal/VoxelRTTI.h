// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"

#define GENERATED_VOXEL_RTTI_BODY(Class) \
	public: \
		static FName StaticClass() { (void)sizeof(Class); static FName Name = #Class; return Name; } \
	private: \
		virtual FName GetPrivateClass() const override { return StaticClass(); } \
		void __Dummy() { static_assert(TIsSame<VOXEL_THIS_TYPE, Class>::Value, "Invalid class!"); } \
	public: \
		TSharedRef<Class> AsShared() { return StaticCastSharedRef<Class>(IVoxelRTTI::AsShared()); } \
		TSharedRef<const Class> AsShared() const { return StaticCastSharedRef<const Class>(IVoxelRTTI::AsShared()); }

class IVoxelRTTI : public TSharedFromThis<IVoxelRTTI>
{
public:
	IVoxelRTTI() = default;
	virtual ~IVoxelRTTI() = default;

	FORCEINLINE FName GetClass() const
	{
		if (PrivateClass.IsNone())
		{
			PrivateClass = GetPrivateClass();
		}
		return PrivateClass;
	}

	template<typename T>
	bool IsA() const
	{
		return GetClass() == T::StaticClass();
	}

	template<typename T>
	T& As()
	{
		check(IsA<T>());
		return static_cast<T&>(*this);
	}
	template<typename T>
	const T& As() const
	{
		check(IsA<T>());
		return static_cast<const T&>(*this);
	}

protected:
	virtual FName GetPrivateClass() const = 0;

private:
	mutable FName PrivateClass;
};

template<typename BaseType>
class TVoxelRTTIStorage
{
public:
	TVoxelRTTIStorage() = default;
	
	template<typename T>
	T& FindOrAdd()
	{
		TSharedPtr<BaseType>& Object = Map.FindOrAdd(T::StaticClass());
		if (!Object)
		{
			Object = MakeShared<T>();
		}
		check(Object->GetClass() == T::StaticClass());
		return static_cast<T&>(*Object);
	}
	template<typename T>
	TSharedPtr<T> Remove()
	{
		TSharedPtr<BaseType> Object;
		Map.RemoveAndCopyValue(T::StaticClass(), Object);
		check(!Object || Object->GetClass() == T::StaticClass());
		return StaticCastSharedPtr<T>(Object);
	}
	
	template<typename T>
	T* Find()
	{
		const TSharedPtr<BaseType>* Metadata = Map.Find(T::StaticClass());
		if (!Metadata)
		{
			return nullptr;
		}
		check((*Metadata)->GetClass() == T::StaticClass());
		return static_cast<T*>(Metadata->Get());
	}
	template<typename T>
	const T* Find() const
	{
		return VOXEL_CONST_CAST(this)->template Find<T>();
	}
	
	template<typename T>
	T FindRef() const
	{
		if (const T* Value = Find<T>())
		{
			return *Value;
		}

		return T{};
	}
	
	template<typename T>
	T& FindChecked()
	{
		T* Result = Find<T>();
		check(Result);
		return *Result;
	}
	template<typename T>
	const T& FindChecked() const
	{
		return VOXEL_CONST_CAST(this)->template FindChecked<T>();
	}

	template<typename T>
	bool Contains() const
	{
		return Find<T>() != nullptr;
	}

	void Empty()
	{
		Map.Empty();
	}
	int32 Num() const
	{
		return Map.Num();
	}

	const TMap<FName, TSharedPtr<BaseType>>& GetMap() const
	{
		return Map;
	}

private:
	TMap<FName, TSharedPtr<BaseType>> Map;
};