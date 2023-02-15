// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "VoxelMinimal/VoxelVirtualStruct.h"
#include "VoxelInstancedStruct.generated.h"

USTRUCT(BlueprintType)
struct VOXELCORE_API FVoxelInstancedStruct
{
	GENERATED_BODY()

public:
	FVoxelInstancedStruct() = default;
	explicit FVoxelInstancedStruct(UScriptStruct* ScriptStruct)
	{
		InitializeAs(ScriptStruct);
	}

	FVoxelInstancedStruct(const FVoxelInstancedStruct& Other)
	{
		InitializeAs(Other.ScriptStruct, Other.StructMemory);
	}

	FORCEINLINE FVoxelInstancedStruct(FVoxelInstancedStruct&& Other)
		: ScriptStruct(Other.ScriptStruct)
		, StructMemory(Other.StructMemory)
	{
		Other.ScriptStruct = nullptr;
		Other.StructMemory = nullptr;
	}
	~FVoxelInstancedStruct()
	{
		Reset();
	}

public:
	FVoxelInstancedStruct& operator=(const FVoxelInstancedStruct& Other)
	{
		if (!ensure(this != &Other))
		{
			return *this;
		}
		
		InitializeAs(Other.ScriptStruct, Other.StructMemory);
		return *this;
	}

	FVoxelInstancedStruct& operator=(FVoxelInstancedStruct&& Other)
	{
		if (!ensure(this != &Other))
		{
			return *this;
		}

		Reset();

		ScriptStruct = Other.ScriptStruct;
		StructMemory = Other.StructMemory;

		Other.ScriptStruct = nullptr;
		Other.StructMemory = nullptr;

		return *this;
	}

	void Reset();
	void* Release();
	// Should work even if ScriptStruct has a virtual destructor because it'll be called by DestroyStruct
	void MoveRawPtr(UScriptStruct* NewScriptStruct, void* NewStructMemory);
	void InitializeAs(UScriptStruct* NewScriptStruct, const void* NewStructMemory = nullptr);

	template<typename T>
	T* Release()
	{
		checkVoxelSlow(IsA<T>());
		return static_cast<T*>(Release());
	}
	template<typename T>
	TUniquePtr<T> ReleaseUnique()
	{
		return TUniquePtr<T>(Release<T>());
	}
	template<typename T>
	TSharedPtr<T> ReleaseShared()
	{
		return ::MakeShareable(Release<T>());
	}

	template<typename T>
	static FVoxelInstancedStruct Make()
	{
		FVoxelInstancedStruct InstancedStruct;
		InstancedStruct.InitializeAs(TBaseStructure<T>::Get());
		return InstancedStruct;
	}

	template<typename T>
	static FVoxelInstancedStruct Make(const T& Struct)
	{
		FVoxelInstancedStruct InstancedStruct;
		if constexpr (TIsDerivedFrom<T, FVoxelVirtualStruct>::Value)
		{
			InstancedStruct.InitializeAs(Struct.GetStruct(), &Struct);
		}
		else
		{
			InstancedStruct.InitializeAs(TBaseStructure<T>::Get(), &Struct);
		}
		return InstancedStruct;
	}
	template<typename T>
	static FVoxelInstancedStruct Make(TUniquePtr<T> Struct)
	{
		checkVoxelSlow(Struct.IsValid());

		FVoxelInstancedStruct InstancedStruct;
		if constexpr (TIsDerivedFrom<T, FVoxelVirtualStruct>::Value)
		{
			InstancedStruct.MoveRawPtr(Struct->GetStruct(), Struct.Get());
		}
		else
		{
			InstancedStruct.MoveRawPtr(TBaseStructure<T>::Get(), Struct.Get());
		}
		Struct.Release();
		return InstancedStruct;
	}

public:
	//~ Begin TStructOpsTypeTraits Interface
	bool Serialize(FArchive& Ar);
	bool Identical(const FVoxelInstancedStruct* Other, uint32 PortFlags) const;
	bool ExportTextItem(FString& ValueStr, const FVoxelInstancedStruct& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const;
	bool ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText, FArchive* InSerializingArchive = nullptr);
	void AddStructReferencedObjects(FReferenceCollector& Collector);
	void GetPreloadDependencies(TArray<UObject*>& OutDependencies) const
	{
		if (ScriptStruct)
		{
			OutDependencies.Add(ScriptStruct);
		}
	}
	//~ End TStructOpsTypeTraits Interface
	
public:
	FORCEINLINE UScriptStruct* GetScriptStruct() const
	{
		return ScriptStruct;
	}
	
	FORCEINLINE void* GetStructMemory()
	{
		return StructMemory;
	}
	FORCEINLINE const void* GetStructMemory() const
	{
		return StructMemory;
	}
	
	FORCEINLINE bool IsValid() const
	{
		checkVoxelSlow((ScriptStruct != nullptr) == (StructMemory != nullptr));
		return ScriptStruct != nullptr;
	}
	
public:
	template<typename T>
	FORCEINLINE bool IsA() const
	{
		checkVoxelSlow((ScriptStruct != nullptr) == (StructMemory != nullptr));
		return ScriptStruct && ScriptStruct->IsChildOf(TBaseStructure<T>::Get());
	}

	template<typename T>
	FORCEINLINE T* GetPtr()
	{
		if (!IsA<T>())
		{
			return nullptr;
		}

		return static_cast<T*>(StructMemory);
	}
	template<typename T>
	FORCEINLINE const T* GetPtr() const
	{
		return VOXEL_CONST_CAST(this)->GetPtr<T>();
	}

	template<typename T>
	FORCEINLINE T& Get()
	{
		checkVoxelSlow(IsA<T>());
		return *static_cast<T*>(StructMemory);
	}
	template<typename T>
	FORCEINLINE const T& Get() const
	{
		return VOXEL_CONST_CAST(this)->Get<T>();
	}

	FORCEINLINE operator bool() const
	{
		return IsValid();
	}

	template<typename T>
	TSharedRef<T> MakeSharedCopy() const
	{
		check(IsA<T>());
		
		void* NewStructMemory = FMemory::Malloc(FMath::Max(1, ScriptStruct->GetStructureSize()));
		ScriptStruct->InitializeStruct(NewStructMemory);
		ScriptStruct->CopyScriptStruct(NewStructMemory, StructMemory);
		return ::MakeShareable(static_cast<T*>(NewStructMemory));
	}
	
	FORCEINLINE void CopyFrom(const void* Data)
	{
		checkVoxelSlow(IsValid());
		ScriptStruct->CopyScriptStruct(GetStructMemory(), Data);
	}
	FORCEINLINE void CopyTo(void* Data) const
	{
		checkVoxelSlow(IsValid());
		ScriptStruct->CopyScriptStruct(Data, GetStructMemory());
	}
	
public:
	bool operator==(const FVoxelInstancedStruct& Other) const
	{
		return Identical(&Other, PPF_None);
	}
	bool operator!=(const FVoxelInstancedStruct& Other) const
	{
		return !Identical(&Other, PPF_None);
	}

private:
	UScriptStruct* ScriptStruct = nullptr;
	void* StructMemory = nullptr;
};

template<>
struct TStructOpsTypeTraits<FVoxelInstancedStruct> : TStructOpsTypeTraitsBase2<FVoxelInstancedStruct>
{
	enum
	{
		WithSerializer = true,
		WithIdentical = true,
		WithExportTextItem = true,
		WithImportTextItem = true,
		WithAddStructReferencedObjects = true,
		WithGetPreloadDependencies = true,
	};
};

template<typename T>
struct TVoxelInstancedStruct : private FVoxelInstancedStruct
{
public:
	TVoxelInstancedStruct() = default;

	template<typename OtherType, typename = typename TEnableIf<TIsDerivedFrom<OtherType, T>::Value>::Type>
	FORCEINLINE TVoxelInstancedStruct(const TVoxelInstancedStruct<OtherType>& Other)
		: FVoxelInstancedStruct(Other)
	{
	}
	template<typename OtherType, typename = typename TEnableIf<TIsDerivedFrom<OtherType, T>::Value>::Type>
	FORCEINLINE TVoxelInstancedStruct(TVoxelInstancedStruct<OtherType>&& Other)
		: FVoxelInstancedStruct(::MoveTemp(Other))
	{
	}

	template<typename OtherType, typename = typename TEnableIf<TIsDerivedFrom<OtherType, T>::Value>::Type>
	FORCEINLINE TVoxelInstancedStruct(const OtherType& Other)
		: FVoxelInstancedStruct(FVoxelInstancedStruct::Make(Other))
	{
	}

	FORCEINLINE TVoxelInstancedStruct(const FVoxelInstancedStruct& Other)
		: FVoxelInstancedStruct(Other)
	{
		CheckType();
	}
	FORCEINLINE TVoxelInstancedStruct(FVoxelInstancedStruct&& Other)
		: FVoxelInstancedStruct(MoveTemp(Other))
	{
		CheckType();
	}

	FORCEINLINE TVoxelInstancedStruct(UScriptStruct* ScriptStruct)
		: FVoxelInstancedStruct(ScriptStruct)
	{
		CheckType();
	}

public:
	template<typename OtherType, typename = typename TEnableIf<TIsDerivedFrom<OtherType, T>::Value>::Type>
	FORCEINLINE TVoxelInstancedStruct& operator=(const TVoxelInstancedStruct<OtherType>& Other)
	{
		FVoxelInstancedStruct::operator=(Other);
		return *this;
	}
	template<typename OtherType, typename = typename TEnableIf<TIsDerivedFrom<OtherType, T>::Value>::Type>
	FORCEINLINE TVoxelInstancedStruct& operator=(TVoxelInstancedStruct<OtherType>&& Other)
	{
		FVoxelInstancedStruct::operator=(MoveTemp(Other));
		return *this;
	}

	FORCEINLINE TVoxelInstancedStruct& operator=(const FVoxelInstancedStruct& Other)
	{
		FVoxelInstancedStruct::operator=(Other);
		CheckType();
		return *this;
	}
	FORCEINLINE TVoxelInstancedStruct& operator=(FVoxelInstancedStruct&& Other)
	{
		FVoxelInstancedStruct::operator=(MoveTemp(Other));
		CheckType();
		return *this;
	}

public:
	using FVoxelInstancedStruct::IsValid;
	using FVoxelInstancedStruct::GetScriptStruct;
	using FVoxelInstancedStruct::AddStructReferencedObjects;

	FORCEINLINE T& Get()
	{
		CheckType();
		checkVoxelSlow(IsValid());
		return *static_cast<T*>(GetStructMemory());
	}
	FORCEINLINE const T& Get() const
	{
		return VOXEL_CONST_CAST(*this).Get();
	}

	template<typename OtherType, typename = typename TEnableIf<TIsDerivedFrom<OtherType, T>::Value && !TIsSame<OtherType, T>::Value>::Type>
	FORCEINLINE OtherType& Get()
	{
		CheckType();
		checkVoxelSlow(IsValid());
		checkVoxelSlow(IsA<OtherType>());
		return *static_cast<OtherType*>(GetStructMemory());
	}
	template<typename OtherType, typename = typename TEnableIf<TIsDerivedFrom<OtherType, T>::Value && !TIsSame<OtherType, T>::Value>::Type>
	FORCEINLINE const OtherType& Get() const
	{
		return VOXEL_CONST_CAST(*this).template Get<OtherType>();
	}

	template<typename OtherType, typename = typename TEnableIf<TIsDerivedFrom<OtherType, T>::Value && !TIsSame<OtherType, T>::Value>::Type>
	FORCEINLINE bool IsA() const
	{
		return FVoxelInstancedStruct::IsA<OtherType>();
	}
	FORCEINLINE bool IsValid() const
	{
		return
			FVoxelInstancedStruct::IsValid() &&
			ensureVoxelSlow(GetScriptStruct()->IsChildOf(TBaseStructure<T>::Get()));
	}

	template<typename OtherType = T, typename = typename TEnableIf<TIsDerivedFrom<OtherType, T>::Value>::Type>
	FORCEINLINE TSharedRef<OtherType> MakeSharedCopy() const
	{
		return FVoxelInstancedStruct::MakeSharedCopy<OtherType>();
	}

	FORCEINLINE T& operator*()
	{
		return Get();
	}
	FORCEINLINE const T& operator*() const
	{
		return Get();
	}

	FORCEINLINE T* operator->()
	{
		return &Get();
	}
	FORCEINLINE const T* operator->() const
	{
		return &Get();
	}

	FORCEINLINE operator bool() const
	{
		return IsValid();
	}
	
private:
	FORCEINLINE void CheckType() const
	{
		checkVoxelSlow(!FVoxelInstancedStruct::IsValid() || FVoxelInstancedStruct::IsA<T>());
	}

	template<typename>
	friend struct TVoxelInstancedStruct;
};

template<typename T>
FORCEINLINE TVoxelInstancedStruct<T> MakeVoxelStructPtr(const T& Struct)
{
	return FVoxelInstancedStruct::Make(Struct);
}