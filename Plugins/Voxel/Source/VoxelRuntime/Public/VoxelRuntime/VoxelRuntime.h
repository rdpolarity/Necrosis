// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelRuntimeSettings.h"

class FVoxelRuntime;
class IVoxelSubsystem;
class UVoxelSubsystemProxy;
class FVoxelRuntimeTickable;

DECLARE_UNIQUE_VOXEL_ID(FVoxelRuntimeId);

class VOXELRUNTIME_API IVoxelMetaGraphRuntime : public TSharedFromThis<IVoxelMetaGraphRuntime>
{
public:
	IVoxelMetaGraphRuntime() = default;
	virtual ~IVoxelMetaGraphRuntime() = default;
	
	virtual void Create() VOXEL_PURE_VIRTUAL();
	virtual void Destroy() VOXEL_PURE_VIRTUAL();
	virtual void Tick() VOXEL_PURE_VIRTUAL();
};

class VOXELRUNTIME_API FVoxelRuntime : public TSharedFromThis<FVoxelRuntime>
{
private:
	FVoxelRuntime();
	~FVoxelRuntime();

	bool CreateInternal();

	friend FVoxelUtilities::TGameThreadDeleter<FVoxelRuntime>;

public:
	FSimpleMulticastDelegate OnDestroy;
	FSimpleMulticastDelegate OnPostEditMove;

	static TSharedPtr<FVoxelRuntime> CreateRuntime(const FVoxelRuntimeSettings& Settings);
	void Destroy();
	void AddReferencedObjects(FReferenceCollector& Collector);

	FORCEINLINE FVoxelRuntimeId GetRuntimeId() const
	{
		return RuntimeId;
	}
	FORCEINLINE bool IsDestroyed() const
	{
		return !bIsCreated;
	}
	FORCEINLINE TStatId GetStatId() const
	{
		return StatId;
	}

	FORCEINLINE const FMatrix& LocalToWorld() const
	{
		return PrivateLocalToWorld;
	}
	FORCEINLINE const FMatrix& WorldToLocal() const
	{
		return PrivateWorldToLocal;
	}

	FORCEINLINE const FVoxelRuntimeSettings& GetSettings() const
	{
		return Settings;
	}
	FORCEINLINE UWorld* GetWorld() const
	{
		return GetSettings().GetWorld();
	}

	FORCEINLINE FVector3d GetPriorityPosition() const
	{
		return PriorityPosition;
	}
	
	template<typename T>
	FORCEINLINE T& GetSubsystem() const
	{
		return static_cast<T&>(this->GetSubsystem(T::StaticClass()));
	}

	IVoxelSubsystem& GetSubsystem(TSubclassOf<UVoxelSubsystemProxy> Class) const;
	TArray<IVoxelSubsystem*> GetAllSubsystems() const;

public:
	template<typename... TArgs>
	TFunction<void(TArgs...)> MakeWeakLambda(TFunction<void(TArgs...)> Lambda) const
	{
		return
			[Lambda = ::MoveTemp(Lambda), WeakRuntime = MakeWeakPtr(AsShared())](TArgs... Args)
			{
				VOXEL_LLM_SCOPE();

				const TSharedPtr<const FVoxelRuntime> Runtime = WeakRuntime.Pin();
				if (!Runtime)
				{
					return;
				}

				if (Runtime->IsDestroyed())
				{
					return;
				}

				Lambda(::Forward<TArgs>(Args)...);
			};
	}
	template<typename LambdaType>
	auto MakeWeakLambda(LambdaType Lambda) const -> decltype(auto)
	{
		return MakeWeakLambda(TFunction<void()>(::MoveTemp(Lambda)));
	}

	void AsyncTask(ENamedThreads::Type Thread, TFunction<void()> Function);

private:
	const FVoxelRuntimeId RuntimeId = FVoxelRuntimeId::New();

	TStatId StatId;
	FThreadSafeBool bIsCreated = false;
	FVoxelRuntimeSettings Settings;
	TSharedPtr<IVoxelMetaGraphRuntime> MetaGraphRuntime;
	TUniquePtr<FVoxelRuntimeTickable> Tickable;

	TArray<TSharedPtr<IVoxelSubsystem>> SubsystemsArray;
	TMap<TSubclassOf<UVoxelSubsystemProxy>, TSharedPtr<IVoxelSubsystem>> SubsystemsMap;

	FMatrix PrivateLocalToWorld;
	FMatrix PrivateWorldToLocal;

	FVector3d PriorityPosition = FVector3d::ZeroVector;
	
	TQueue<TFunction<void()>, EQueueMode::Mpsc> QueuedGameThreadTasks;

	void Tick();

	friend class FVoxelRuntimeTickable;
};