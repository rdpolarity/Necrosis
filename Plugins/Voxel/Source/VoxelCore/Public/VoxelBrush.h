// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBrush.generated.h"

class FVoxelRuntime;
class FVoxelBrushRegistry;
struct FVoxelBrushImpl;
struct FVoxelDistanceField;

DECLARE_UNIQUE_VOXEL_ID(FVoxelBrushInternalId);

#define GENERATED_VOXEL_BRUSH_BODY() \
	GENERATED_VIRTUAL_STRUCT_BODY_IMPL(FVoxelBrush) \
	virtual TSharedPtr<const FVoxelBrushImpl> GetImpl(const FMatrix& WorldToLocal) const override;

#define DEFINE_VOXEL_BRUSH(Type) \
	TSharedPtr<const FVoxelBrushImpl> Type::GetImpl(const FMatrix& WorldToLocal) const \
	{ \
		checkStatic(TIsSame<VOXEL_THIS_TYPE, Type ## Impl::BrushType>::Value); \
		if (!IsValid()) \
		{ \
			return nullptr; \
		} \
		return MakeShared<Type ## Impl>(*this, WorldToLocal); \
	}

USTRUCT(BlueprintType)
struct VOXELCORE_API FVoxelBrush : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	DECLARE_VIRTUAL_STRUCT_PARENT(FVoxelBrush, GENERATED_VOXEL_BRUSH_BODY)
		
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config")
	FName LayerName = "Main";

	UPROPERTY()
	FMatrix BrushToWorld = FMatrix::Identity;

	UPROPERTY()
	TWeakObjectPtr<AActor> ActorToSelect;

	virtual uint64 GetHash() const;
	virtual bool IsValid() const VOXEL_PURE_VIRTUAL({});
	virtual void CacheData_GameThread() {}
	virtual TSharedPtr<const FVoxelBrushImpl> GetImpl(const FMatrix& WorldToLocal) const VOXEL_PURE_VIRTUAL({});
};

struct VOXELCORE_API FVoxelBrushImpl : public TSharedFromThis<FVoxelBrushImpl>
{
public:
	using BrushType = FVoxelBrush;

	FVoxelBrushImpl() = default;
	virtual ~FVoxelBrushImpl() = default;

	struct FLess
	{
		FORCEINLINE bool operator()(const TSharedPtr<const FVoxelBrushImpl>& A, const TSharedPtr<const FVoxelBrushImpl>& B) const
		{
#if VOXEL_DEBUG
			if (A->PrivateHash == B->PrivateHash)
			{
				const UScriptStruct* Struct = A->PrivateBrush->GetStruct();
				ensureAlways(Struct->CompareScriptStruct(A->PrivateBrush, B->PrivateBrush, PPF_None));
			}
#endif
			return A->PrivateHash < B->PrivateHash;
		}
	};

	FORCEINLINE const FVoxelBrush& GetBrush() const
	{
		checkVoxelSlow(PrivateBrush);
		return *PrivateBrush;
	}
	FORCEINLINE const FVoxelBox& GetBounds() const
	{
		ensure(!PrivateBounds.IsInfinite());
		return PrivateBounds;
	}

	virtual float GetDistance(const FVector& LocalPosition) const = 0;
	virtual TSharedPtr<FVoxelDistanceField> GetDistanceField() const = 0;

protected:
	void SetBounds(const FVoxelBox& Bounds)
	{
		ensure(PrivateBounds.IsInfinite());
		ensure(!Bounds.IsInfinite());
		PrivateBounds = Bounds;
	}

private:
	const FVoxelBrush* PrivateBrush = nullptr;
	uint64 PrivateHash = 0;
	FVoxelBox PrivateBounds = FVoxelBox::Infinite;

	template<typename>
	friend struct TVoxelBrushImpl;
};

template<typename InBrushType>
struct TVoxelBrushImpl : FVoxelBrushImpl
{
	using BrushType = InBrushType;
	const BrushType Brush;

	explicit TVoxelBrushImpl(const BrushType& Brush, const FMatrix& WorldToLocal)
		: Brush(Brush)
	{
		PrivateBrush = &this->Brush;

		PrivateHash = FVoxelUtilities::MurmurHashMulti(PrivateBrush->GetHash(), WorldToLocal);
		ensure(PrivateHash);
	}
};

extern VOXELCORE_API FVoxelBrushRegistry* GVoxelBrushRegistry;

class FVoxelBrushId
{
public:
	FVoxelBrushId() = default;

	FORCEINLINE bool IsValid() const
	{
		return Struct != nullptr;
	}

	FORCEINLINE bool operator==(const FVoxelBrushId& Other) const
	{
		return
			World == Other.World &&
			Struct == Other.Struct &&
			Id == Other.Id;
	}
	FORCEINLINE friend uint32 GetTypeHash(const FVoxelBrushId& BrushId)
	{
		return GetTypeHash(BrushId.Id);
	}

private:
	const void* World = nullptr;
	UStruct* Struct = nullptr;
	FVoxelBrushInternalId Id;

	friend FVoxelBrushRegistry;
};

class VOXELCORE_API FVoxelBrushRegistry : public FVoxelTicker
{
public:
	template<typename T, typename = typename TEnableIf<TIsDerivedFrom<T, FVoxelBrush>::Value>::Type>
	TVoxelArray<TSharedPtr<const T>> GetBrushes(const UWorld* World) const
	{
		return ReinterpretCastVoxelArray<TSharedPtr<const T>>(this->GetBrushes(World, T::StaticStruct()));
	}
	template<typename T, typename = typename TEnableIf<TIsDerivedFrom<T, FVoxelBrush>::Value>::Type>
	void AddListener(const UWorld* World, const FSimpleDelegate& OnChanged)
	{
		this->AddListener(World, T::StaticStruct(), OnChanged);
	}

	TVoxelArray<TSharedPtr<const FVoxelBrush>> GetBrushes(const UWorld* World, const UScriptStruct* Struct) const;

	void AddListener(const UWorld* World, const UScriptStruct* Struct, const FSimpleDelegate& OnChanged);
	void UpdateBrush(const UWorld* World, FVoxelBrushId& BrushId, const TSharedPtr<const FVoxelBrush>& Brush);

private:
	struct FKey
	{
		const void* World = nullptr;
		const UStruct* Struct = nullptr;

		FORCEINLINE bool operator==(const FKey& Other) const
		{
			return
				World == Other.World &&
				Struct == Other.Struct;
		}
		FORCEINLINE friend uint32 GetTypeHash(const FKey& Key)
		{
			return HashCombine(GetTypeHash(Key.World), GetTypeHash(Key.Struct));
		}
	};
	struct FBrushes
	{
		mutable FVoxelCriticalSection CriticalSection;
		FSimpleMulticastDelegate OnChanged;
		TMap<FVoxelBrushInternalId, TSharedPtr<const FVoxelBrush>> Brushes;
	};

	mutable FVoxelCriticalSection CriticalSection;
	TSet<FKey> DirtyBrushes;
	TMap<FKey, TSharedPtr<FBrushes>> BrushesMap;

	double LastGC = 0;

	TSharedRef<FBrushes> FindOrAddBrushes(const FKey& Key, bool bMarkDirty = false);
	void UpdateBrushImpl(const FKey& Key, FVoxelBrushInternalId BrushId, const TSharedPtr<const FVoxelBrush>& Brush);

	//~ Begin FVoxelTicker Interface
	virtual void Tick() override;
	//~ End FVoxelTicker Interface
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UCLASS(Abstract)
class VOXELCORE_API AVoxelBrushActor : public AActor
{
	GENERATED_BODY()

public:
	AVoxelBrushActor() = default;
	virtual ~AVoxelBrushActor() override;

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;
	virtual void Destroyed() override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
	virtual void PostEditUndo() override;
#endif
	//~ End AActor Interface
	
	UFUNCTION(BlueprintCallable, Category = "Voxel|Brush")
	void UpdateBrush();

	UFUNCTION(BlueprintCallable, Category = "Voxel|Brush")
	void RemoveBrush();

protected:
	virtual TSharedPtr<const FVoxelBrush> GetBrush() const;

private:
	FVoxelBrushId BrushId;
#if WITH_EDITOR
	TOptional<FTransform> PostEditMove_LastActorToWorld;
#endif
};