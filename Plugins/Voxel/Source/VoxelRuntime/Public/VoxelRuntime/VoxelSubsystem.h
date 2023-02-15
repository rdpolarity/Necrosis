// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelRuntime.h"
#include "VoxelSubsystem.generated.h"

DECLARE_VOXEL_COUNTER(VOXELRUNTIME_API, STAT_VoxelNumSubsystems, "Num Subsystems");
DECLARE_STATS_GROUP(TEXT("Voxel Subsystems"), STATGROUP_VoxelSubsystems, STATCAT_Advanced);

class IVoxelSubsystem;

UCLASS(Abstract)
class VOXELRUNTIME_API UVoxelSubsystemProxy : public UObject
{
	GENERATED_BODY()

public:
	using SubsystemClass = IVoxelSubsystem;

	virtual TSharedRef<IVoxelSubsystem> GetSubsystem() const;
	virtual FName GetSubsystemCppName() const { return {}; }

public:
	//~ Begin UObject Interface
	virtual void PostCDOContruct() override;
	//~ End UObject Interface

protected:
	template<typename T>
	static TSharedRef<T> MakeSubsystem()
	{
		return TSharedPtr<T>(new T(), [](T* Object)
		{
			if (Object)
			{
				FVoxelUtilities::RunOnGameThread([=]
				{
					UVoxelSubsystemProxy::DeleteSubsystem(Object);
				});
			}
		}).ToSharedRef();
	}

	template<typename T>
	static void DeleteSubsystem(T* Object)
	{
		VOXEL_FUNCTION_COUNTER();
		FScopeCycleCounter CycleCounter(Object->GetStatId());
		delete Object;
	}
	
public:
	static FString GetSubsystemClassDisplayName(TSubclassOf<UVoxelSubsystemProxy> Class);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY_NAMESPACE(InSubsystemClass) \
	public: \
	using SubsystemClass = InSubsystemClass; \
	virtual FName GetSubsystemCppName() const override { return STATIC_FNAME(#InSubsystemClass); } \
	virtual TSharedRef<IVoxelSubsystem> GetSubsystem() const override;

#define GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(InSubsystemClass) \
	VOXEL_FWD_DECLARE_CLASS(InSubsystemClass) \
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY_NAMESPACE(InSubsystemClass)

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define DEFINE_VOXEL_SUBSYSTEM(InSubsystemClass) \
	TSharedRef<IVoxelSubsystem> InSubsystemClass::ProxyClass::GetSubsystem() const \
	{ \
		static_assert(TIsSame<InSubsystemClass, InSubsystemClass::ProxyClass::SubsystemClass>::Value, "Incorrect GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY or GENERATED_VOXEL_SUBSYSTEM_BODY on " #InSubsystemClass); \
		return MakeSubsystem<InSubsystemClass>(); \
	}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define GENERATED_VOXEL_SUBSYSTEM_BODY_IMPL(InProxyClass) \
	using Super = InProxyClass::Super::SubsystemClass; \
	using ProxyClass = InProxyClass; \
	private: \
	virtual TSubclassOf<UVoxelSubsystemProxy> Internal_GetProxyClass() const override { static_assert(TIsSame<VOXEL_THIS_TYPE, ProxyClass::SubsystemClass>::Value, ""); return StaticClass(); } \
	public: \
	FORCEINLINE auto AsShared() { return StaticCastSharedRef<VOXEL_THIS_TYPE>(this->Super::AsShared()); } \
	FORCEINLINE auto AsShared() const { return StaticCastSharedRef<const VOXEL_THIS_TYPE>(this->Super::AsShared()); } \
	FORCEINLINE static TSubclassOf<UVoxelSubsystemProxy> StaticClass() { return ProxyClass::StaticClass(); }

#define GENERATED_VOXEL_SUBSYSTEM_BODY_NO_CONSTRUCTOR(InProxyClass) GENERATED_VOXEL_SUBSYSTEM_BODY_IMPL(InProxyClass)

#define GENERATED_VOXEL_SUBSYSTEM_BODY(InProxyClass) \
	GENERATED_VOXEL_SUBSYSTEM_BODY_NO_CONSTRUCTOR(InProxyClass) \
	using Super::Super;

#define GENERATED_VOXEL_SUBSYSTEM_BODY_TEMPLATE(InProxyClass, ParentTemplate) \
	GENERATED_VOXEL_SUBSYSTEM_BODY_NO_CONSTRUCTOR(InProxyClass) \
	using TemplateSuper = ParentTemplate; \
	using TemplateSuper::TemplateSuper;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DECLARE_UNIQUE_VOXEL_ID_EXPORT(VOXELRUNTIME_API, FVoxelSubsystemId);

// Subsystems will always be deleted on the game thread
class VOXELRUNTIME_API IVoxelSubsystem : public TSharedFromThis<IVoxelSubsystem>
{
public:
	IVoxelSubsystem() = default;
	virtual ~IVoxelSubsystem();
	UE_NONCOPYABLE(IVoxelSubsystem);

	VOXEL_NUM_INSTANCES_TRACKER(STAT_VoxelNumSubsystems);

public:
	FORCEINLINE FVoxelSubsystemId GetSubsystemId() const
	{
		return SubsystemId;
	}
	FORCEINLINE TSubclassOf<UVoxelSubsystemProxy> GetProxyClass() const
	{
		return CachedProxyClass;
	}
	FORCEINLINE FName GetCppName() const
	{
		return CachedCppName;
	}
	FORCEINLINE TStatId GetStatId() const
	{
		return StatId;
	}

	FORCEINLINE UWorld* GetWorld() const
	{
		return GetRuntimeChecked().GetSettings().GetWorld();
	}
	FORCEINLINE AActor* GetActor() const
	{
		return GetRuntimeChecked().GetSettings().GetActor();
	}
	FORCEINLINE USceneComponent* GetRootComponent() const
	{
		return GetRuntimeChecked().GetSettings().GetRootComponent();
	}

	FORCEINLINE const FMatrix& LocalToWorld() const
	{
		return GetRuntimeChecked().LocalToWorld();
	}
	FORCEINLINE const FMatrix& WorldToLocal() const
	{
		return GetRuntimeChecked().WorldToLocal();
	}

	FORCEINLINE TSharedPtr<FVoxelRuntime> GetRuntime() const
	{
		return WeakRuntime.Pin();
	}
	FORCEINLINE FVoxelRuntime& GetRuntimeChecked() const
	{
		checkVoxelSlow(!IsInGameThread() || !IsDestroyed() || FunctionBeingCalled == EFunction::Destroy);
		checkVoxelSlow(WeakRuntime.IsValid());
		checkVoxelSlow(WeakRuntime.Pin().Get() == RawRuntime);
		return *RawRuntime;
	}

public:
	template<typename T>
	FORCEINLINE T& GetSubsystem() const
	{
		return GetRuntimeChecked().GetSubsystem<T>();
	}

	bool IsDestroyed() const
	{
		return bIsDestroyed;
	}

public:
	template<typename LambdaType>
	auto MakeWeakSubsystemLambda(LambdaType Function) -> decltype(auto)
	{
		return MakeWeakPtrLambda(this, [Function = MoveTemp(Function), this](auto&&... Args)
		{
			VOXEL_LLM_SCOPE();

			if (!IsDestroyed())
			{
				Function(Forward<decltype(Args)>(Args)...);
			}
		});
	}
	template<typename LambdaType>
	auto MakeWeakSubsystemLambda(LambdaType Function) const -> decltype(auto)
	{
		return MakeWeakPtrLambda(this, [Function = MoveTemp(Function), this](auto&&... Args)
		{
			if (!IsDestroyed())
			{
				Function(Forward<decltype(Args)>(Args)...);
			}
		});
	}
	template<typename DelegateType = FSimpleDelegate, typename LambdaType>
	DelegateType MakeWeakSubsystemDelegate(LambdaType Function) const
	{
		return MakeWeakPtrTypedDelegate<DelegateType>(this, [Function = MoveTemp(Function), this](auto&&... Args)
		{
			if (!IsDestroyed())
			{
				Function(Forward<decltype(Args)>(Args)...);
			}
		});
	}

	void AsyncTask(ENamedThreads::Type Thread, TFunction<void()> Function)
	{
		GetRuntimeChecked().AsyncTask(Thread, MakeWeakSubsystemLambda(MoveTemp(Function)));
	}

protected:
	// GetSubsystem will initialize the other subsystem, leading to possible recursive dependencies
	virtual void Initialize();
	virtual void Destroy();
	virtual void Tick();
	virtual void AddReferencedObjects(FReferenceCollector& Collector);
	
private:
	void CallInitialize();
	void CallDestroy();
	void CallTick();
	void CallAddReferencedObjects(FReferenceCollector& Collector);

protected:
	virtual TSubclassOf<UVoxelSubsystemProxy> Internal_GetProxyClass() const = 0;
	virtual void Internal_Initialize(FVoxelRuntime& Runtime);

private:
	const FVoxelSubsystemId SubsystemId = FVoxelSubsystemId::New();

	TSubclassOf<UVoxelSubsystemProxy> CachedProxyClass;
	FName CachedCppName;

	TWeakPtr<FVoxelRuntime> WeakRuntime;
	FVoxelRuntime* RawRuntime = nullptr;

	TStatId StatId;

	FThreadSafeBool bIsInitialized = false;
	FThreadSafeBool bIsDestroyed = false;
	
	enum class EFunction
	{
		None,
		Initialize,
		Destroy,
		Tick,
		AddReferencedObjects,
	};
	bool bSuperCalled = false;
	EFunction FunctionBeingCalled = EFunction::None;

	friend class FVoxelRuntime;
};