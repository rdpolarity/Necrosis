// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "Stats/StatsMisc.h"
#include "HAL/LowLevelMemStats.h"
#include "VoxelMacros.h"

DECLARE_STATS_GROUP(TEXT("Voxel"), STATGROUP_Voxel, STATCAT_Advanced);
DECLARE_STATS_GROUP(TEXT("Voxel Counters"), STATGROUP_VoxelCounters, STATCAT_Advanced);
DECLARE_STATS_GROUP(TEXT("Voxel Memory"), STATGROUP_VoxelMemory, STATCAT_Advanced);
DECLARE_STATS_GROUP(TEXT("Voxel GPU Memory"), STATGROUP_VoxelGpuMemory, STATCAT_Advanced);

#if ENABLE_LOW_LEVEL_MEM_TRACKER
DECLARE_LLM_MEMORY_STAT(TEXT("Voxel"), STAT_VoxelLLM, STATGROUP_LLMFULL);
#endif

#define VOXEL_LLM_TAG ELLMTag(int32(ELLMTag::ProjectTagEnd) - 1)
#define VOXEL_LLM_SCOPE() \
	LLM(if (!GVoxelLLMRegistered) { Voxel_RegisterLLM(); }) \
	LLM_SCOPE(VOXEL_LLM_TAG)

extern VOXELCORE_API bool GVoxelLLMRegistered;
VOXELCORE_API void Voxel_RegisterLLM();
VOXELCORE_API void Voxel_CheckLLMScope();

#if STATS
struct FStat_Voxel_Base
{
	static FORCEINLINE EStatDataType::Type GetStatType()
	{
		return EStatDataType::ST_int64;
	}
	static FORCEINLINE bool IsClearEveryFrame()
	{
		return true;
	}
	static FORCEINLINE bool IsCycleStat()
	{
		return true;
	}
	static FORCEINLINE FPlatformMemory::EMemoryCounterRegion GetMemoryRegion()
	{
		return FPlatformMemory::MCR_Invalid;
	}
};
template<typename T>
struct TStat_Voxel_Initializer
{
	TStat_Voxel_Initializer(const FString& Description)
	{
		T::GetDescriptionRef() = Description;
	}
};

#define IMPL_VOXEL_SCOPE_COUNTER_STAT_CLASS_NAME(Suffix) PREPROCESSOR_JOIN(PREPROCESSOR_JOIN(FStat_Voxel_, __LINE__), Suffix)

// We want to be able to use __FUNCTION__ as description, so it's a bit tricky
#define VOXEL_SCOPE_COUNTER_IMPL(Description) \
	struct IMPL_VOXEL_SCOPE_COUNTER_STAT_CLASS_NAME(PREPROCESSOR_NOTHING) : FStat_Voxel_Base \
	{ \
		using TGroup = FStatGroup_STATGROUP_Voxel; \
		\
		static FORCEINLINE FString& GetDescriptionRef() \
		{ \
			static FString StaticDescription = "VoxelStaticDescription"; \
			return StaticDescription; \
		} \
		static FORCEINLINE const char* GetStatName() \
		{ \
			return PREPROCESSOR_TO_STRING(IMPL_VOXEL_SCOPE_COUNTER_STAT_CLASS_NAME(_Name)); \
		} \
		static FORCEINLINE const TCHAR* GetDescription() \
		{ \
			return *GetDescriptionRef(); \
		} \
	}; \
	static TStat_Voxel_Initializer<IMPL_VOXEL_SCOPE_COUNTER_STAT_CLASS_NAME(PREPROCESSOR_NOTHING)> IMPL_VOXEL_SCOPE_COUNTER_STAT_CLASS_NAME(_Initializer){ Description }; \
	static FThreadSafeStaticStat<IMPL_VOXEL_SCOPE_COUNTER_STAT_CLASS_NAME(PREPROCESSOR_NOTHING)> IMPL_VOXEL_SCOPE_COUNTER_STAT_CLASS_NAME(_Ptr); \
	FScopeCycleCounter IMPL_VOXEL_SCOPE_COUNTER_STAT_CLASS_NAME(_CycleCount)(IMPL_VOXEL_SCOPE_COUNTER_STAT_CLASS_NAME(_Ptr.GetStatId())); \
	VOXEL_DEBUG_ONLY(Voxel_CheckLLMScope());

#else
#define VOXEL_SCOPE_COUNTER_IMPL(Description)
#endif

VOXELCORE_API FString VoxelStats_CleanupFunctionName(const FString& FunctionName);

#define VOXEL_STATS_CLEAN_FUNCTION_NAME VoxelStats_CleanupFunctionName(__FUNCTION__)

#define VOXEL_SCOPE_COUNTER(Description) VOXEL_SCOPE_COUNTER_IMPL(VOXEL_STATS_CLEAN_FUNCTION_NAME + FString(TEXT(".")) + Description)
#define VOXEL_FUNCTION_COUNTER() VOXEL_SCOPE_COUNTER_IMPL(VOXEL_STATS_CLEAN_FUNCTION_NAME)
#define VOXEL_INLINE_COUNTER(Name, ...) ([&]() -> decltype(auto) { VOXEL_SCOPE_COUNTER_IMPL(VOXEL_STATS_CLEAN_FUNCTION_NAME + TEXT(".") + Name); return __VA_ARGS__; }())
#define VOXEL_FUNCTION_COUNTER_LLM() VOXEL_LLM_SCOPE() VOXEL_FUNCTION_COUNTER()

#define VOXEL_LOG_FUNCTION_STATS() FScopeLogTime PREPROCESSOR_JOIN(FScopeLogTime_, __LINE__)(*STATIC_FSTRING(VOXEL_STATS_CLEAN_FUNCTION_NAME));
#define VOXEL_LOG_SCOPE_STATS(Name) FScopeLogTime PREPROCESSOR_JOIN(FScopeLogTime_, __LINE__)(*STATIC_FSTRING(VOXEL_STATS_CLEAN_FUNCTION_NAME + "." Name));
#define VOXEL_TRACE_BOOKMARK() TRACE_BOOKMARK(*STATIC_FSTRING(VOXEL_STATS_CLEAN_FUNCTION_NAME));

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define DECLARE_VOXEL_COUNTER(API, StatName, Name) DECLARE_VOXEL_COUNTER_WITH_CATEGORY(API, STATGROUP_VoxelCounters, StatName, Name)

#define DECLARE_VOXEL_COUNTER_WITH_CATEGORY(API, Category, StatName, Name) \
	VOXEL_DEBUG_ONLY(extern API FThreadSafeCounter64 StatName;) \
	DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT(Name), StatName ## _Stat, Category, API)


#define DECLARE_VOXEL_FRAME_COUNTER(API, StatName, Name) DECLARE_VOXEL_FRAME_COUNTER_WITH_CATEGORY(API, STATGROUP_VoxelCounters, StatName, Name)

#define DECLARE_VOXEL_FRAME_COUNTER_WITH_CATEGORY(API, Category, StatName, Name) \
	VOXEL_DEBUG_ONLY(extern API FThreadSafeCounter64 StatName;) \
	DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT(Name), StatName ## _Stat, Category, API)


#define DEFINE_VOXEL_COUNTER(StatName) \
	VOXEL_DEBUG_ONLY(FThreadSafeCounter64 StatName;) \
	DEFINE_STAT(StatName ## _Stat)

#define INC_VOXEL_COUNTER_BY(StatName, Amount) \
	VOXEL_DEBUG_ONLY(StatName.Add(Amount);) \
	INC_DWORD_STAT_BY(StatName ## _Stat, Amount)

#define DEC_VOXEL_COUNTER_BY(StatName, Amount) \
	VOXEL_DEBUG_ONLY(StatName.Subtract(Amount);) \
	VOXEL_DEBUG_ONLY(ensure(StatName.GetValue() >= 0);) \
	DEC_DWORD_STAT_BY(StatName ## _Stat, Amount);

#define DEC_VOXEL_COUNTER(StatName) DEC_VOXEL_COUNTER_BY(StatName, 1)
#define INC_VOXEL_COUNTER(StatName) INC_VOXEL_COUNTER_BY(StatName, 1)

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define DECLARE_VOXEL_MEMORY_STAT(API, StatName, Name) DECLARE_VOXEL_MEMORY_STAT_WITH_CATEGORY(API, STATGROUP_VoxelMemory, StatName, Name)
#define DECLARE_VOXEL_GPU_MEMORY_STAT(API, StatName, Name) DECLARE_VOXEL_MEMORY_STAT_WITH_CATEGORY(API, STATGROUP_VoxelGpuMemory, StatName, Name)

#define DECLARE_VOXEL_MEMORY_STAT_WITH_CATEGORY(API, Category, StatName, Name) \
	VOXEL_DEBUG_ONLY(extern API FThreadSafeCounter64 StatName;) \
	DECLARE_MEMORY_STAT_EXTERN(TEXT(Name), StatName ## _Stat, Category, API)

#define DEFINE_VOXEL_MEMORY_STAT(StatName) \
	VOXEL_DEBUG_ONLY(FThreadSafeCounter64 StatName;) \
	DEFINE_STAT(StatName ## _Stat)

#define INC_VOXEL_MEMORY_STAT_BY(StatName, Amount) \
	VOXEL_DEBUG_ONLY(StatName.Add(Amount);) \
	INC_MEMORY_STAT_BY(StatName ## _Stat, Amount)

#define DEC_VOXEL_MEMORY_STAT_BY(StatName, Amount) \
	VOXEL_DEBUG_ONLY(ensure(StatName.Subtract(Amount) >= 0);) \
	DEC_MEMORY_STAT_BY(StatName ## _Stat, Amount);

#define FVoxelStatsRefHelper VOXEL_APPEND_LINE(__FVoxelStatsRefHelper)

#if STATS
#define GET_VOXEL_MEMORY_STAT(Name) (INTELLISENSE_ONLY((void)&Name,) StatPtr_ ## Name ## _Stat)
#else
#define GET_VOXEL_MEMORY_STAT(Name)
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UE_TRACE_CHANNEL_EXTERN(VoxelChannel, VOXELCORE_API);

#define VOXEL_SCOPE_COUNTER_STRING(Name) TRACE_CPUPROFILER_EVENT_SCOPE_TEXT_ON_CHANNEL(*(Name), VoxelChannel)
#define VOXEL_SCOPE_COUNTER_FORMAT(Format, ...) VOXEL_SCOPE_COUNTER_STRING(FString::Printf(TEXT(Format), ##__VA_ARGS__))

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if STATS
// UpdateStats will be called by MakeShared after the constructor is called
#define VOXEL_ALLOCATED_SIZE_TRACKER(StatName) \
	struct FVoxelStatsRefHelper \
	{ \
		int64 AllocatedSize = 0; \
		\
		FVoxelStatsRefHelper() = default; \
		FVoxelStatsRefHelper(FVoxelStatsRefHelper&& Other) \
		{ \
			AllocatedSize = Other.AllocatedSize; \
			Other.AllocatedSize = 0; \
		} \
		FVoxelStatsRefHelper& operator=(FVoxelStatsRefHelper&& Other) \
		{ \
			AllocatedSize = Other.AllocatedSize; \
			Other.AllocatedSize = 0; \
			return *this; \
		} \
		FVoxelStatsRefHelper(const FVoxelStatsRefHelper& Other) \
		{ \
			AllocatedSize = Other.AllocatedSize; \
			INC_VOXEL_MEMORY_STAT_BY(StatName, AllocatedSize); \
		} \
		FVoxelStatsRefHelper& operator=(const FVoxelStatsRefHelper& Other) \
		{ \
			AllocatedSize = Other.AllocatedSize; \
			INC_VOXEL_MEMORY_STAT_BY(StatName, AllocatedSize); \
			return *this; \
		} \
		FORCEINLINE ~FVoxelStatsRefHelper() \
		{ \
			DEC_VOXEL_MEMORY_STAT_BY(StatName, AllocatedSize); \
		} \
	}; \
	mutable FVoxelStatsRefHelper VOXEL_APPEND_LINE(__VoxelStatsRefHelper); \
	void UpdateStats() const \
	{ \
		int64& AllocatedSize = VOXEL_APPEND_LINE(__VoxelStatsRefHelper).AllocatedSize; \
		DEC_VOXEL_MEMORY_STAT_BY(StatName, AllocatedSize); \
		AllocatedSize = GetAllocatedSize(); \
		INC_VOXEL_MEMORY_STAT_BY(StatName, AllocatedSize); \
	} \
	void EnsureStatsAreUpToDate() const \
	{ \
		ensure(VOXEL_APPEND_LINE(__VoxelStatsRefHelper).AllocatedSize == GetAllocatedSize()); \
	}

#define VOXEL_TYPE_SIZE_TRACKER(Type, StatName) \
	struct FVoxelStatsRefHelper \
	{ \
		FVoxelStatsRefHelper() \
		{ \
			INC_VOXEL_MEMORY_STAT_BY(StatName, sizeof(Type)); \
		} \
		FORCEINLINE ~FVoxelStatsRefHelper() \
		{ \
			DEC_VOXEL_MEMORY_STAT_BY(StatName, sizeof(Type)); \
		} \
		FVoxelStatsRefHelper(FVoxelStatsRefHelper&& Other) \
			: FVoxelStatsRefHelper() \
		{ \
		} \
		FVoxelStatsRefHelper(const FVoxelStatsRefHelper& Other) \
			: FVoxelStatsRefHelper() \
		{ \
		} \
		FVoxelStatsRefHelper& operator=(FVoxelStatsRefHelper&& Other) \
		{ \
			return *this; \
		} \
		FVoxelStatsRefHelper& operator=(const FVoxelStatsRefHelper& Other) \
		{ \
			return *this; \
		} \
	}; \
	FVoxelStatsRefHelper VOXEL_APPEND_LINE(__VoxelStatsRefHelper);

#define VOXEL_NUM_INSTANCES_TRACKER(StatName) \
	struct FVoxelStatsRefHelper \
	{ \
		FORCEINLINE FVoxelStatsRefHelper() \
		{ \
			INC_VOXEL_COUNTER(StatName); \
		} \
		FORCEINLINE ~FVoxelStatsRefHelper() \
		{ \
			DEC_VOXEL_COUNTER(StatName); \
		} \
	}; \
	FVoxelStatsRefHelper VOXEL_APPEND_LINE(__VoxelStatsRefHelper);

#define VOXEL_ALLOCATED_SIZE_TRACKER_CUSTOM(StatName, Name) \
	class FVoxelStatsRefHelper \
	{ \
	public:\
		FVoxelStatsRefHelper() = default; \
		FVoxelStatsRefHelper(FVoxelStatsRefHelper&& Other) \
		{ \
			AllocatedSize = Other.AllocatedSize; \
			Other.AllocatedSize = 0; \
		} \
		FVoxelStatsRefHelper& operator=(FVoxelStatsRefHelper&& Other) \
		{ \
			AllocatedSize = Other.AllocatedSize; \
			Other.AllocatedSize = 0; \
			return *this; \
		} \
		FVoxelStatsRefHelper(const FVoxelStatsRefHelper& Other) \
		{ \
			AllocatedSize = Other.AllocatedSize; \
			INC_VOXEL_MEMORY_STAT_BY(StatName, AllocatedSize); \
		} \
		FVoxelStatsRefHelper& operator=(const FVoxelStatsRefHelper& Other) \
		{ \
			AllocatedSize = Other.AllocatedSize; \
			INC_VOXEL_MEMORY_STAT_BY(StatName, AllocatedSize); \
			return *this; \
		} \
		FORCEINLINE ~FVoxelStatsRefHelper() \
		{ \
			DEC_VOXEL_MEMORY_STAT_BY(StatName, AllocatedSize); \
		} \
		FVoxelStatsRefHelper& operator=(int64 InAllocatedSize) \
		{ \
			DEC_VOXEL_MEMORY_STAT_BY(StatName, AllocatedSize); \
			AllocatedSize = InAllocatedSize; \
			INC_VOXEL_MEMORY_STAT_BY(StatName, AllocatedSize); \
			return *this; \
		} \
	private: \
		int64 AllocatedSize = 0; \
	}; \
	mutable FVoxelStatsRefHelper Name;

#define VOXEL_COUNTER_HELPER(StatName, Name) \
	class FVoxelStatsRefHelper \
	{ \
	public:\
		FVoxelStatsRefHelper() = default; \
		FVoxelStatsRefHelper(FVoxelStatsRefHelper&& Other) \
		{ \
			Value = Other.Value; \
			Other.Value = 0; \
		} \
		FVoxelStatsRefHelper& operator=(FVoxelStatsRefHelper&& Other) \
		{ \
			Value = Other.Value; \
			Other.Value = 0; \
			return *this; \
		} \
		FVoxelStatsRefHelper(const FVoxelStatsRefHelper& Other) \
		{ \
			Value = Other.Value; \
			INC_VOXEL_COUNTER_BY(StatName, Value); \
		} \
		FVoxelStatsRefHelper& operator=(const FVoxelStatsRefHelper& Other) \
		{ \
			Value = Other.Value; \
			INC_VOXEL_COUNTER_BY(StatName, Value); \
			return *this; \
		} \
		FORCEINLINE ~FVoxelStatsRefHelper() \
		{ \
			DEC_VOXEL_COUNTER_BY(StatName, Value); \
		} \
		FVoxelStatsRefHelper& operator=(int64 NewValue) \
		{ \
			DEC_VOXEL_COUNTER_BY(StatName, Value); \
			Value = NewValue; \
			INC_VOXEL_COUNTER_BY(StatName, Value); \
			return *this; \
		} \
	private: \
		int64 Value = 0; \
	}; \
	mutable FVoxelStatsRefHelper Name;

VOXELCORE_API void Voxel_AddAmountToDynamicStat(FName Name, int64 Amount);

#else
#define VOXEL_ALLOCATED_SIZE_TRACKER(StatName) \
	void UpdateStats() const \
	{ \
	} \
	void EnsureStatsAreUpToDate() const \
	{ \
	}

#define VOXEL_ALLOCATED_SIZE_TRACKER_CUSTOM(StatName, Name) \
	class FVoxelStatsRefHelper \
	{ \
	public:\
		FVoxelStatsRefHelper() = default; \
		void UpdateStats() \
		{ \
		} \
		FVoxelStatsRefHelper& operator+=(int64 Data) \
		{ \
			return *this; \
		} \
		FVoxelStatsRefHelper& operator=(int64 Data) \
		{ \
			return *this; \
		} \
	}; \
	mutable FVoxelStatsRefHelper Name;

#define VOXEL_COUNTER_HELPER(StatName, Name) \
	class FVoxelStatsRefHelper \
	{ \
	public:\
		FVoxelStatsRefHelper() = default; \
		FVoxelStatsRefHelper& operator=(int64 NewValue) \
		{ \
			return *this; \
		} \
	}; \
	mutable FVoxelStatsRefHelper Name;

#define VOXEL_TYPE_SIZE_TRACKER(Type, StatName)

#define VOXEL_NUM_INSTANCES_TRACKER(StatName)

FORCEINLINE void Voxel_AddAmountToDynamicStat(FName Name, int64 Amount)
{
}

#endif