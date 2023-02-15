// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"
#include "VoxelEngineVersionHelpers.h"

#ifndef INTELLISENSE_PARSER
#if defined(__INTELLISENSE__) || defined(__RSCPP_VERSION)
#define INTELLISENSE_PARSER 1
#else
#define INTELLISENSE_PARSER 0
#endif
#endif

#if INTELLISENSE_PARSER
#undef VOXEL_DEBUG
#define VOXEL_DEBUG 1
#undef RHI_RAYTRACING
#define RHI_RAYTRACING 1
#define INTELLISENSE_ONLY(...) __VA_ARGS__
#define INTELLISENSE_SKIP(...)

// Needed for Resharper to detect the printf hidden in the lambda
#undef UE_LOG
#define UE_LOG(CategoryName, Verbosity, Format, ...) \
	{ \
		(void)ELogVerbosity::Verbosity; \
		(void)(FLogCategoryBase*)&CategoryName; \
		FString::Printf(Format, ##__VA_ARGS__); \
	}

#error "Compiler defined as parser"
#else
#define INTELLISENSE_ONLY(...)
#define INTELLISENSE_SKIP(...) __VA_ARGS__
#endif

// This is defined in the generated.h. It lets you use GetOuterASomeOuter. Resharper/intellisense are confused when it's used, so define it for them
#define INTELLISENSE_DECLARE_WITHIN(Name) INTELLISENSE_ONLY(DECLARE_WITHIN(Name))

#define INTELLISENSE_PRINTF(Format, ...) INTELLISENSE_ONLY((void)FString::Printf(TEXT(Format), __VA_ARGS__);)

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if VOXEL_DEBUG
#define checkVoxelSlow(x) check(x)
#define checkfVoxelSlow(x, ...) checkf(x, ##__VA_ARGS__)
#define ensureVoxelSlow(x) ensure(x)
#define ensureMsgfVoxelSlow(x, ...) ensureMsgf(x, ##__VA_ARGS__)
#define ensureVoxelSlowNoSideEffects(x) ensure(x)
#define ensureMsgfVoxelSlowNoSideEffects(x, ...) ensureMsgf(x, ##__VA_ARGS__)
#define VOXEL_ASSUME(...) check(__VA_ARGS__)
#undef FORCEINLINE
#define FORCEINLINE FORCEINLINE_DEBUGGABLE_ACTUAL
#define VOXEL_DEBUG_ONLY(...) __VA_ARGS__
#else
#define checkVoxelSlow(x)
#define checkfVoxelSlow(x, ...)
#define ensureVoxelSlow(x) (!!(x))
#define ensureMsgfVoxelSlow(x, ...) (!!(x))
#define ensureVoxelSlowNoSideEffects(x)
#define ensureMsgfVoxelSlowNoSideEffects(...)
#define VOXEL_ASSUME(...) UE_ASSUME(__VA_ARGS__)
#define VOXEL_DEBUG_ONLY(...)
#endif

#if VOXEL_DEBUG
#define ensureThreadSafe(...) ensure(__VA_ARGS__)
#else
#define ensureThreadSafe(...)
#endif

#define checkStatic(...) static_assert(__VA_ARGS__, "Static assert failed")
#define checkfStatic(Expr, Error) static_assert(Expr, Error)

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define VOXEL_LOCTEXT(Text) INVTEXT(Text)

// Inline static helper to avoid rehashing FNames
#ifndef STATIC_FNAME
#define STATIC_FNAME(Name) ([]() -> const FName& { static const FName StaticName = Name; return StaticName; }())
#endif

// Static string helper
#ifndef STATIC_FSTRING
#define STATIC_FSTRING(String) ([]() -> const FString& { static const FString StaticString = String; return StaticString; }())
#endif

#ifndef GET_MEMBER_NAME_STATIC
#define GET_MEMBER_NAME_STATIC(ClassName, MemberName) STATIC_FNAME(GET_MEMBER_NAME_STRING_CHECKED(ClassName, MemberName))
#endif

#ifndef GET_OWN_MEMBER_NAME
#define GET_OWN_MEMBER_NAME(MemberName) GET_MEMBER_NAME_CHECKED(TDecay<decltype(*this)>::Type, MemberName)
#endif

#ifndef FUNCTION_FNAME
#define FUNCTION_FNAME FName(__FUNCTION__)
#endif

#define VOXEL_FUNCTION_FNAME STATIC_FNAME(*VoxelStats_CleanupFunctionName(__FUNCTION__))

VOXELCORE_API FString Voxel_RemoveFunctionNameScope(const FString& FunctionName);

#ifndef FUNCTION_ERROR_IMPL
#define FUNCTION_ERROR_IMPL(FunctionName, Error) (FString(FunctionName) + TEXT(": ") + Error)
#endif

#ifndef FUNCTION_ERROR
#define FUNCTION_ERROR(Error) FUNCTION_ERROR_IMPL(__FUNCTION__, Error)
#endif

#define VOXEL_DEPRECATED(Version, Message) UE_DEPRECATED(0, Message " If this is a C++ voxel graph, you should compile it to C++ again.")

#define VOXEL_GET_TYPE(Value) typename TDecay<decltype(Value)>::Type
#define VOXEL_THIS_TYPE VOXEL_GET_TYPE(*this)
// This is needed in classes, where just doing class Name would fwd declare it in the class scope
#define VOXEL_FWD_DECLARE_CLASS(Name) void PREPROCESSOR_JOIN(__VoxelDeclareDummy_, __LINE__)(class Name*);

// This makes the macro parameter show up as a class in Resharper
#if INTELLISENSE_PARSER
#define VOXEL_FWD_DECLARE_CLASS_INTELLISENSE(Name) VOXEL_FWD_DECLARE_CLASS(Name)
#else
#define VOXEL_FWD_DECLARE_CLASS_INTELLISENSE(Name)
#endif

#define VOXEL_CONSOLE_VARIABLE(Api, Type, Name, Default, Command, Description) \
	Api Type Name = Default; \
	static FAutoConsoleVariableRef CVar_ ## Name( \
		TEXT(Command),  \
		Name,  \
		TEXT(Description))

#define VOXEL_CONSOLE_COMMAND(Name, Command, Description) \
	INTELLISENSE_ONLY(void Name()); \
	static void Cmd ## Name(); \
	static FAutoConsoleCommand AutoCmd ## Name( \
	    TEXT(Command), \
	    TEXT(Description), \
		MakeLambdaDelegate([] \
		{ \
			VOXEL_LLM_SCOPE(); \
			Cmd ## Name(); \
		})); \
	\
	static void Cmd ## Name()

#define VOXEL_EXPAND(X) X

#define VOXEL_APPEND_LINE(X) PREPROCESSOR_JOIN(X, __LINE__)

#define VOXEL_STACK_BUFFER_UNINITIALIZED(Name, Size) \
	void* RESTRICT const Name = FMemory_Alloca(Size);

#define VOXEL_STACK_BUFFER_ZEROED(Name, Size) \
	const int32 VOXEL_APPEND_LINE(AllocaSize) = Size; \
	void* RESTRICT const Name = FMemory_Alloca(VOXEL_APPEND_LINE(AllocaSize)); \
	FMemory::Memzero(Name, VOXEL_APPEND_LINE(AllocaSize));

// Unlike GENERATE_MEMBER_FUNCTION_CHECK, this supports inheritance
// However, it doesn't do any signature check
#define VOXEL_GENERATE_MEMBER_FUNCTION_CHECK(MemberName)		            \
template <typename T>														\
class THasMemberFunction_##MemberName										\
{																			\
	template <typename U> static char MemberTest(decltype(&U::MemberName));	\
	template <typename U> static int32 MemberTest(...);						\
public:																		\
	enum { Value = sizeof(MemberTest<T>(nullptr)) == sizeof(char) };		\
};

#define VOXEL_FOLD_EXPRESSION(...) \
	{ \
		int32 Temp[] = { 0, ((__VA_ARGS__), 0)... }; \
		(void)Temp; \
	}

#if VOXEL_DEBUG && !IS_MONOLITHIC
#define VOXEL_ISPC_ASSERT() \
	extern "C" void VoxelISPC_Assert(const int32 Line) \
	{ \
		ensureAlwaysMsgf(false, TEXT("ISPC LINE: %d"), Line); \
	}
#else
#define VOXEL_ISPC_ASSERT() 
#endif

#define VOXEL_DEFAULT_MODULE(Name) IMPLEMENT_MODULE(FDefaultModuleImpl, Name) VOXEL_ISPC_ASSERT()

#define VOXEL_PURE_VIRTUAL(...) { ensureMsgf(false, TEXT("Pure virtual %s called"), *FString(__FUNCTION__)); return __VA_ARGS__; }

#define VOXEL_USE_VARIABLE(Name) ensure(&Name != reinterpret_cast<void*>(0x1234))

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern VOXELCORE_API FSimpleMulticastDelegate GOnVoxelModuleUnloaded;

enum class EVoxelRunOnStartupPhase
{
	Game,
	Editor,
	FirstTick
};
struct VOXELCORE_API FVoxelRunOnStartupPhaseHelper
{
	FVoxelRunOnStartupPhaseHelper(EVoxelRunOnStartupPhase Phase, int32 Priority, TFunction<void()> Lambda);
};

#define VOXEL_RUN_ON_STARTUP(UniqueId, Phase, Priority) \
	INTELLISENSE_ONLY(void UniqueId();) \
	void VOXEL_APPEND_LINE(PREPROCESSOR_JOIN(VoxelStartupFunction, UniqueId))(); \
	static const FVoxelRunOnStartupPhaseHelper PREPROCESSOR_JOIN(VoxelRunOnStartupPhaseHelper, UniqueId)(EVoxelRunOnStartupPhase::Phase, Priority, [] \
	{ \
		VOXEL_APPEND_LINE(PREPROCESSOR_JOIN(VoxelStartupFunction, UniqueId))(); \
	}); \
	void VOXEL_APPEND_LINE(PREPROCESSOR_JOIN(VoxelStartupFunction, UniqueId))()

#define VOXEL_RUN_ON_STARTUP_GAME(UniqueId) VOXEL_RUN_ON_STARTUP(UniqueId, Game, 0)
#define VOXEL_RUN_ON_STARTUP_EDITOR(UniqueId) VOXEL_RUN_ON_STARTUP(UniqueId, Editor, 0)

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace VoxelInternal
{
	struct FVoxelOnConstruct
	{
		template<typename FuncType>
		FVoxelOnConstruct operator+(FuncType&& InFunc)
		{
			InFunc();
			return {};
		}
	};
}

#define VOXEL_ON_CONSTRUCT() VoxelInternal::FVoxelOnConstruct VOXEL_APPEND_LINE(__OnConstruct) = VoxelInternal::FVoxelOnConstruct() + [this]

#define VOXEL_SLATE_ARGS() \
	struct FArguments; \
	using WidgetArgsType = FArguments; \
	struct FDummyArguments \
	{ \
		using FArguments = FArguments; \
	}; \
	\
	struct FArguments : public TSlateBaseNamedArgs<FDummyArguments>

// Useful for templates
#define VOXEL_WRAP(...) __VA_ARGS__

#define UE_NONCOPYABLE_MOVEABLE(TypeName) \
	TypeName(TypeName&&) = default; \
	TypeName& operator=(TypeName&&) = default; \
	TypeName(const TypeName&) = delete; \
	TypeName& operator=(const TypeName&) = delete;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct FVoxelLambdaCaller
{
	template<typename T>
	FORCEINLINE auto operator+(T&& Lambda) -> decltype(auto)
	{
		return Lambda();
	}
};

#define INLINE_LAMBDA FVoxelLambdaCaller() + [&]()

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define VOXEL_UNIQUE_ID() []() { ensureVoxelSlowNoSideEffects(IsInGameThread()); static uint64 __UniqueId = 0; return ++__UniqueId; }()

// UniqueClass: to forbid copying ids from different classes
template<typename UniqueClass>
class TVoxelUniqueId
{
public:
	TVoxelUniqueId() = default;

	FORCEINLINE bool IsValid() const { return Id != 0; }

	FORCEINLINE bool operator==(const TVoxelUniqueId& Other) const { return Id == Other.Id; }
	FORCEINLINE bool operator!=(const TVoxelUniqueId& Other) const { return Id != Other.Id; }

	FORCEINLINE bool operator<(const TVoxelUniqueId& Other) const { return Id < Other.Id; }
	FORCEINLINE bool operator>(const TVoxelUniqueId& Other) const { return Id > Other.Id; }

	FORCEINLINE bool operator<=(const TVoxelUniqueId& Other) const { return Id <= Other.Id; }
	FORCEINLINE bool operator>=(const TVoxelUniqueId& Other) const { return Id >= Other.Id; }

	FORCEINLINE friend uint32 GetTypeHash(TVoxelUniqueId UniqueId)
	{
	    return uint32(UniqueId.Id);
	}

	FORCEINLINE uint64 GetId() const
	{
		return Id;
	}

	FORCEINLINE static TVoxelUniqueId New()
	{
		return TVoxelUniqueId(TVoxelUniqueId_MakeNew(static_cast<UniqueClass*>(nullptr)));
	}

private:
	FORCEINLINE TVoxelUniqueId(uint64 Id)
		: Id(Id)
	{
		ensureVoxelSlow(IsValid());
	}
	
	uint64 Id = 0;
};

#define DECLARE_UNIQUE_VOXEL_ID_EXPORT(Api, Name) \
	Api uint64 TVoxelUniqueId_MakeNew(class __ ## Name ##_Unique*); \
	using Name = TVoxelUniqueId<class __ ## Name ##_Unique>;

#define DECLARE_UNIQUE_VOXEL_ID(Name) DECLARE_UNIQUE_VOXEL_ID_EXPORT(,Name)

#define DEFINE_UNIQUE_VOXEL_ID(Name) \
	INTELLISENSE_ONLY(void VOXEL_APPEND_LINE(Dummy)(Name);) \
	uint64 TVoxelUniqueId_MakeNew(class __ ## Name ##_Unique*) \
	{ \
		static FThreadSafeCounter64 Counter; \
		return Counter.Increment(); \
	}

template<typename T>
class TVoxelIndex
{
public:
	TVoxelIndex() = default;

	FORCEINLINE bool IsValid() const
	{
		return Index != -1;
	}
	FORCEINLINE operator bool() const
	{
		return IsValid();
	}

	FORCEINLINE bool operator==(const TVoxelIndex& Other) const
	{
		return Index == Other.Index;
	}
	FORCEINLINE bool operator!=(const TVoxelIndex& Other) const
	{
		return Index != Other.Index;
	}

	FORCEINLINE friend uint32 GetTypeHash(TVoxelIndex InIndex)
	{
	    return InIndex.Index;
	}

protected:
	int32 Index = -1;
	
	FORCEINLINE TVoxelIndex(int32 Index)
		: Index(Index)
	{
	}

	FORCEINLINE operator int32() const
	{
		return Index;
	}

	friend T;
};
 static_assert(sizeof(TVoxelIndex<class FVoxelIndexDummy>) == sizeof(int32), "");

#define DECLARE_VOXEL_INDEX(Name, FriendClass) using Name = TVoxelIndex<class FriendClass>;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename T>
struct TVoxelRemoveConst
{
	using Type = T&;
};
template<typename T>
struct TVoxelRemoveConst<const T>
{
	using Type = T&;
};
template<typename T>
struct TVoxelRemoveConst<const T*>
{
	using Type = T*;
};
template<typename T>
struct TVoxelRemoveConst<const T&>
{
	using Type = T&;
};

#define VOXEL_CONST_CAST(X) const_cast<typename TVoxelRemoveConst<decltype(X)>::Type>(X)

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename ToType, typename FromType, typename = typename TEnableIf<sizeof(FromType) == sizeof(ToType)>::Type>
ToType* ReinterpretCastPtr(FromType* From)
{
	return reinterpret_cast<ToType*>(From);
}
template<typename ToType, typename FromType, typename = typename TEnableIf<sizeof(FromType) == sizeof(ToType)>::Type>
const ToType* ReinterpretCastPtr(const FromType* From)
{
	return reinterpret_cast<const ToType*>(From);
}

template<typename ToType, typename FromType, typename = typename TEnableIf<sizeof(FromType) == sizeof(ToType)>::Type>
ToType& ReinterpretCastRef(FromType& From)
{
	return reinterpret_cast<ToType&>(From);
}
template<typename ToType, typename FromType, typename = typename TEnableIf<sizeof(FromType) == sizeof(ToType)>::Type>
const ToType& ReinterpretCastRef(const FromType& From)
{
	return reinterpret_cast<const ToType&>(From);
}

template<typename ToType, typename FromType, typename = typename TEnableIf<sizeof(FromType) == sizeof(ToType) && !TIsReferenceType<FromType>::Value>::Type>
ToType&& ReinterpretCastRef(FromType&& From)
{
	return reinterpret_cast<ToType&&>(From);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename ToType, typename FromType, typename Allocator, typename = typename TEnableIf<sizeof(FromType) == sizeof(ToType)>::Type>
TArray<ToType, Allocator>& ReinterpretCastArray(TArray<FromType, Allocator>& Array)
{
	return reinterpret_cast<TArray<ToType, Allocator>&>(Array);
}
template<typename ToType, typename FromType, typename Allocator, typename = typename TEnableIf<sizeof(FromType) == sizeof(ToType)>::Type>
const TArray<ToType, Allocator>& ReinterpretCastArray(const TArray<FromType, Allocator>& Array)
{
	return reinterpret_cast<const TArray<ToType, Allocator>&>(Array);
}

template<typename ToType, typename FromType, typename Allocator, typename = typename TEnableIf<sizeof(FromType) == sizeof(ToType)>::Type>
TArray<ToType, Allocator>&& ReinterpretCastArray(TArray<FromType, Allocator>&& Array)
{
	return reinterpret_cast<TArray<ToType, Allocator>&&>(Array);
}
template<typename ToType, typename FromType, typename Allocator, typename = typename TEnableIf<sizeof(FromType) == sizeof(ToType)>::Type>
const TArray<ToType, Allocator>&& ReinterpretCastArray(const TArray<FromType, Allocator>&& Array)
{
	return reinterpret_cast<const TArray<ToType, Allocator>&&>(Array);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename ToType, typename ToAllocator, typename FromType, typename Allocator, typename = typename TEnableIf<sizeof(FromType) != sizeof(ToType)>::Type>
TArray<ToType, ToAllocator> ReinterpretCastArray_Copy(const TArray<FromType, Allocator>& Array)
{
	const int64 NumBytes = Array.Num() * sizeof(FromType);
	check(NumBytes % sizeof(ToType) == 0);
	return TArray<ToType, Allocator>(reinterpret_cast<const ToType*>(Array.GetData()), NumBytes / sizeof(ToType));
}
template<typename ToType, typename FromType, typename Allocator, typename = typename TEnableIf<sizeof(FromType) != sizeof(ToType)>::Type>
TArray<ToType, Allocator> ReinterpretCastArray_Copy(const TArray<FromType, Allocator>& Array)
{
	return ::ReinterpretCastArray_Copy<ToType, Allocator>(Array);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename ToType, typename FromType, typename Allocator, typename = typename TEnableIf<sizeof(FromType) == sizeof(ToType)>::Type>
TSet<ToType, DefaultKeyFuncs<ToType>, Allocator>& ReinterpretCastSet(TSet<FromType, DefaultKeyFuncs<FromType>, Allocator>& Set)
{
	return reinterpret_cast<TSet<ToType, DefaultKeyFuncs<ToType>, Allocator>&>(Set);
}
template<typename ToType, typename FromType, typename Allocator, typename = typename TEnableIf<sizeof(FromType) == sizeof(ToType)>::Type>
const TSet<ToType, DefaultKeyFuncs<ToType>, Allocator>& ReinterpretCastSet(const TSet<FromType, DefaultKeyFuncs<FromType>, Allocator>& Set)
{
	return reinterpret_cast<const TSet<ToType, DefaultKeyFuncs<ToType>, Allocator>&>(Set);
}

template<typename ToType, typename FromType, typename Allocator, typename = typename TEnableIf<sizeof(FromType) == sizeof(ToType)>::Type>
TSet<ToType, DefaultKeyFuncs<ToType>, Allocator>&& ReinterpretCastSet(TSet<FromType, DefaultKeyFuncs<FromType>, Allocator>&& Set)
{
	return reinterpret_cast<TSet<ToType, DefaultKeyFuncs<ToType>, Allocator>&&>(Set);
}
template<typename ToType, typename FromType, typename Allocator, typename = typename TEnableIf<sizeof(FromType) == sizeof(ToType)>::Type>
const TSet<ToType, DefaultKeyFuncs<ToType>, Allocator>&& ReinterpretCastSet(const TSet<FromType, DefaultKeyFuncs<FromType>, Allocator>&& Set)
{
	return reinterpret_cast<const TSet<ToType, DefaultKeyFuncs<ToType>, Allocator>&&>(Set);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename T>
TSoftObjectPtr<T> MakeSoftObjectPtr(const FString& Path)
{
	return TSoftObjectPtr<T>(FSoftObjectPath(Path));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define __VOXEL_GET_NTH_ARG(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, N, ...) VOXEL_EXPAND(N)

#define __VOXEL_FOREACH_IMPL_00(Prefix, Last, Suffix, X)      Last(X)
#define __VOXEL_FOREACH_IMPL_01(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_00(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_02(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_01(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_03(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_02(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_04(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_03(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_05(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_04(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_06(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_05(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_07(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_06(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_08(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_07(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_09(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_08(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_10(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_09(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_11(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_10(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_12(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_11(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_13(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_12(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_14(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_13(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_15(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_14(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_16(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_15(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_17(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_16(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_18(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_17(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_19(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_18(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_20(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_19(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_21(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_20(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_22(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_21(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_23(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_22(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_24(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_23(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_25(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_24(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_26(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_25(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_27(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_26(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_28(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_27(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_29(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_28(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_30(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_29(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_31(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_30(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)
#define __VOXEL_FOREACH_IMPL_32(Prefix, Last, Suffix, X, ...) Prefix(X) VOXEL_EXPAND(__VOXEL_FOREACH_IMPL_31(Prefix, Last, Suffix, __VA_ARGS__)) Suffix(X)

#define VOXEL_FOREACH_IMPL(Prefix, Last, Suffix, ...) \
	VOXEL_EXPAND(__VOXEL_GET_NTH_ARG(__VA_ARGS__, \
	__VOXEL_FOREACH_IMPL_32, \
	__VOXEL_FOREACH_IMPL_31, \
	__VOXEL_FOREACH_IMPL_30, \
	__VOXEL_FOREACH_IMPL_29, \
	__VOXEL_FOREACH_IMPL_28, \
	__VOXEL_FOREACH_IMPL_27, \
	__VOXEL_FOREACH_IMPL_26, \
	__VOXEL_FOREACH_IMPL_25, \
	__VOXEL_FOREACH_IMPL_24, \
	__VOXEL_FOREACH_IMPL_23, \
	__VOXEL_FOREACH_IMPL_22, \
	__VOXEL_FOREACH_IMPL_21, \
	__VOXEL_FOREACH_IMPL_20, \
	__VOXEL_FOREACH_IMPL_19, \
	__VOXEL_FOREACH_IMPL_18, \
	__VOXEL_FOREACH_IMPL_17, \
	__VOXEL_FOREACH_IMPL_16, \
	__VOXEL_FOREACH_IMPL_15, \
	__VOXEL_FOREACH_IMPL_14, \
	__VOXEL_FOREACH_IMPL_13, \
	__VOXEL_FOREACH_IMPL_12, \
	__VOXEL_FOREACH_IMPL_11, \
	__VOXEL_FOREACH_IMPL_10, \
	__VOXEL_FOREACH_IMPL_09, \
	__VOXEL_FOREACH_IMPL_08, \
	__VOXEL_FOREACH_IMPL_07, \
	__VOXEL_FOREACH_IMPL_06, \
	__VOXEL_FOREACH_IMPL_05, \
	__VOXEL_FOREACH_IMPL_04, \
	__VOXEL_FOREACH_IMPL_03, \
	__VOXEL_FOREACH_IMPL_02, \
	__VOXEL_FOREACH_IMPL_01, \
	__VOXEL_FOREACH_IMPL_00)(Prefix, Last, Suffix, __VA_ARGS__))

#define VOXEL_VOID_MACRO(...)

#define VOXEL_FOREACH(Macro, ...) VOXEL_FOREACH_IMPL(Macro, Macro, VOXEL_VOID_MACRO, __VA_ARGS__)
#define VOXEL_FOREACH_SUFFIX(Macro, ...) VOXEL_FOREACH_IMPL(VOXEL_VOID_MACRO, Macro, Macro, __VA_ARGS__)

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define __VOXEL_FOREACH_ONE_ARG_00(Macro, Arg, X)      Macro(Arg, X)
#define __VOXEL_FOREACH_ONE_ARG_01(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_00(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_02(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_01(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_03(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_02(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_04(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_03(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_05(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_04(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_06(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_05(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_07(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_06(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_08(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_07(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_09(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_08(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_10(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_09(Macro, Arg, __VA_ARGS__))

#define __VOXEL_FOREACH_ONE_ARG_11(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_10(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_12(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_11(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_13(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_12(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_14(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_13(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_15(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_14(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_16(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_15(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_17(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_16(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_18(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_17(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_19(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_18(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_20(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_19(Macro, Arg, __VA_ARGS__))

#define __VOXEL_FOREACH_ONE_ARG_21(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_20(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_22(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_21(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_23(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_22(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_24(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_23(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_25(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_24(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_26(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_25(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_27(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_26(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_28(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_27(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_29(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_28(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_30(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_29(Macro, Arg, __VA_ARGS__))

#define __VOXEL_FOREACH_ONE_ARG_31(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_30(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_32(Macro, Arg, X, ...) Macro(Arg, X) VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_31(Macro, Arg, __VA_ARGS__))

#define VOXEL_FOREACH_ONE_ARG(Macro, Arg, ...) \
	VOXEL_EXPAND(__VOXEL_GET_NTH_ARG(__VA_ARGS__, \
	__VOXEL_FOREACH_ONE_ARG_32, \
	__VOXEL_FOREACH_ONE_ARG_31, \
	__VOXEL_FOREACH_ONE_ARG_30, \
	__VOXEL_FOREACH_ONE_ARG_29, \
	__VOXEL_FOREACH_ONE_ARG_28, \
	__VOXEL_FOREACH_ONE_ARG_27, \
	__VOXEL_FOREACH_ONE_ARG_26, \
	__VOXEL_FOREACH_ONE_ARG_25, \
	__VOXEL_FOREACH_ONE_ARG_24, \
	__VOXEL_FOREACH_ONE_ARG_23, \
	__VOXEL_FOREACH_ONE_ARG_22, \
	__VOXEL_FOREACH_ONE_ARG_21, \
	__VOXEL_FOREACH_ONE_ARG_20, \
	__VOXEL_FOREACH_ONE_ARG_19, \
	__VOXEL_FOREACH_ONE_ARG_18, \
	__VOXEL_FOREACH_ONE_ARG_17, \
	__VOXEL_FOREACH_ONE_ARG_16, \
	__VOXEL_FOREACH_ONE_ARG_15, \
	__VOXEL_FOREACH_ONE_ARG_14, \
	__VOXEL_FOREACH_ONE_ARG_13, \
	__VOXEL_FOREACH_ONE_ARG_12, \
	__VOXEL_FOREACH_ONE_ARG_11, \
	__VOXEL_FOREACH_ONE_ARG_10, \
	__VOXEL_FOREACH_ONE_ARG_09, \
	__VOXEL_FOREACH_ONE_ARG_08, \
	__VOXEL_FOREACH_ONE_ARG_07, \
	__VOXEL_FOREACH_ONE_ARG_06, \
	__VOXEL_FOREACH_ONE_ARG_05, \
	__VOXEL_FOREACH_ONE_ARG_04, \
	__VOXEL_FOREACH_ONE_ARG_03, \
	__VOXEL_FOREACH_ONE_ARG_02, \
	__VOXEL_FOREACH_ONE_ARG_01, \
	__VOXEL_FOREACH_ONE_ARG_00)(Macro, Arg, __VA_ARGS__))

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define __VOXEL_FOREACH_TWO_ARGS_00(Macro, Arg1, Arg2, X)      Macro(Arg1, Arg2, X)
#define __VOXEL_FOREACH_TWO_ARGS_01(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_00(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_02(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_01(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_03(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_02(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_04(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_03(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_05(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_04(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_06(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_05(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_07(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_06(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_08(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_07(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_09(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_08(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_10(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_09(Macro, Arg1, Arg2, __VA_ARGS__))

#define __VOXEL_FOREACH_TWO_ARGS_11(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_10(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_12(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_11(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_13(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_12(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_14(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_13(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_15(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_14(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_16(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_15(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_17(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_16(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_18(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_17(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_19(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_18(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_20(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_19(Macro, Arg1, Arg2, __VA_ARGS__))

#define __VOXEL_FOREACH_TWO_ARGS_21(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_20(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_22(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_21(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_23(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_22(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_24(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_23(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_25(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_24(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_26(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_25(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_27(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_26(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_28(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_27(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_29(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_28(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_30(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_29(Macro, Arg1, Arg2, __VA_ARGS__))

#define __VOXEL_FOREACH_TWO_ARGS_31(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_30(Macro, Arg1, Arg2, __VA_ARGS__))
#define __VOXEL_FOREACH_TWO_ARGS_32(Macro, Arg1, Arg2, X, ...) Macro(Arg1, Arg2, X) VOXEL_EXPAND(__VOXEL_FOREACH_TWO_ARGS_31(Macro, Arg1, Arg2, __VA_ARGS__))

#define VOXEL_FOREACH_TWO_ARGS(Macro, Arg1, Arg2, ...) \
	VOXEL_EXPAND(__VOXEL_GET_NTH_ARG(__VA_ARGS__, \
	__VOXEL_FOREACH_TWO_ARGS_32, \
	__VOXEL_FOREACH_TWO_ARGS_31, \
	__VOXEL_FOREACH_TWO_ARGS_30, \
	__VOXEL_FOREACH_TWO_ARGS_29, \
	__VOXEL_FOREACH_TWO_ARGS_28, \
	__VOXEL_FOREACH_TWO_ARGS_27, \
	__VOXEL_FOREACH_TWO_ARGS_26, \
	__VOXEL_FOREACH_TWO_ARGS_25, \
	__VOXEL_FOREACH_TWO_ARGS_24, \
	__VOXEL_FOREACH_TWO_ARGS_23, \
	__VOXEL_FOREACH_TWO_ARGS_22, \
	__VOXEL_FOREACH_TWO_ARGS_21, \
	__VOXEL_FOREACH_TWO_ARGS_20, \
	__VOXEL_FOREACH_TWO_ARGS_19, \
	__VOXEL_FOREACH_TWO_ARGS_18, \
	__VOXEL_FOREACH_TWO_ARGS_17, \
	__VOXEL_FOREACH_TWO_ARGS_16, \
	__VOXEL_FOREACH_TWO_ARGS_15, \
	__VOXEL_FOREACH_TWO_ARGS_14, \
	__VOXEL_FOREACH_TWO_ARGS_13, \
	__VOXEL_FOREACH_TWO_ARGS_12, \
	__VOXEL_FOREACH_TWO_ARGS_11, \
	__VOXEL_FOREACH_TWO_ARGS_10, \
	__VOXEL_FOREACH_TWO_ARGS_09, \
	__VOXEL_FOREACH_TWO_ARGS_08, \
	__VOXEL_FOREACH_TWO_ARGS_07, \
	__VOXEL_FOREACH_TWO_ARGS_06, \
	__VOXEL_FOREACH_TWO_ARGS_05, \
	__VOXEL_FOREACH_TWO_ARGS_04, \
	__VOXEL_FOREACH_TWO_ARGS_03, \
	__VOXEL_FOREACH_TWO_ARGS_02, \
	__VOXEL_FOREACH_TWO_ARGS_01, \
	__VOXEL_FOREACH_TWO_ARGS_00)(Macro, Arg1, Arg2, __VA_ARGS__))

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define __VOXEL_FOREACH_COMMA_00(Macro, X)      Macro(X)
#define __VOXEL_FOREACH_COMMA_01(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_00(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_02(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_01(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_03(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_02(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_04(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_03(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_05(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_04(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_06(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_05(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_07(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_06(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_08(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_07(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_09(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_08(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_10(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_09(Macro, __VA_ARGS__))

#define __VOXEL_FOREACH_COMMA_11(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_10(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_12(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_11(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_13(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_12(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_14(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_13(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_15(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_14(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_16(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_15(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_17(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_16(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_18(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_17(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_19(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_18(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_20(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_19(Macro, __VA_ARGS__))

#define __VOXEL_FOREACH_COMMA_21(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_20(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_22(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_21(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_23(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_22(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_24(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_23(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_25(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_24(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_26(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_25(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_27(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_26(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_28(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_27(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_29(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_28(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_30(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_29(Macro, __VA_ARGS__))

#define __VOXEL_FOREACH_COMMA_31(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_30(Macro, __VA_ARGS__))
#define __VOXEL_FOREACH_COMMA_32(Macro, X, ...) Macro(X), VOXEL_EXPAND(__VOXEL_FOREACH_COMMA_31(Macro, __VA_ARGS__))

#define VOXEL_FOREACH_COMMA(Macro, ...) \
	VOXEL_EXPAND(__VOXEL_GET_NTH_ARG(__VA_ARGS__, \
	__VOXEL_FOREACH_COMMA_32, \
	__VOXEL_FOREACH_COMMA_31, \
	__VOXEL_FOREACH_COMMA_30, \
	__VOXEL_FOREACH_COMMA_29, \
	__VOXEL_FOREACH_COMMA_28, \
	__VOXEL_FOREACH_COMMA_27, \
	__VOXEL_FOREACH_COMMA_26, \
	__VOXEL_FOREACH_COMMA_25, \
	__VOXEL_FOREACH_COMMA_24, \
	__VOXEL_FOREACH_COMMA_23, \
	__VOXEL_FOREACH_COMMA_22, \
	__VOXEL_FOREACH_COMMA_21, \
	__VOXEL_FOREACH_COMMA_20, \
	__VOXEL_FOREACH_COMMA_19, \
	__VOXEL_FOREACH_COMMA_18, \
	__VOXEL_FOREACH_COMMA_17, \
	__VOXEL_FOREACH_COMMA_16, \
	__VOXEL_FOREACH_COMMA_15, \
	__VOXEL_FOREACH_COMMA_14, \
	__VOXEL_FOREACH_COMMA_13, \
	__VOXEL_FOREACH_COMMA_12, \
	__VOXEL_FOREACH_COMMA_11, \
	__VOXEL_FOREACH_COMMA_10, \
	__VOXEL_FOREACH_COMMA_09, \
	__VOXEL_FOREACH_COMMA_08, \
	__VOXEL_FOREACH_COMMA_07, \
	__VOXEL_FOREACH_COMMA_06, \
	__VOXEL_FOREACH_COMMA_05, \
	__VOXEL_FOREACH_COMMA_04, \
	__VOXEL_FOREACH_COMMA_03, \
	__VOXEL_FOREACH_COMMA_02, \
	__VOXEL_FOREACH_COMMA_01, \
	__VOXEL_FOREACH_COMMA_00)(Macro, __VA_ARGS__))

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define __VOXEL_FOREACH_ONE_ARG_COMMA_00(Macro, Arg, X)      Macro(Arg, X)
#define __VOXEL_FOREACH_ONE_ARG_COMMA_01(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_00(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_02(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_01(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_03(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_02(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_04(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_03(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_05(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_04(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_06(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_05(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_07(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_06(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_08(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_07(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_09(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_08(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_10(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_09(Macro, Arg, __VA_ARGS__))

#define __VOXEL_FOREACH_ONE_ARG_COMMA_11(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_10(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_12(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_11(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_13(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_12(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_14(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_13(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_15(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_14(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_16(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_15(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_17(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_16(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_18(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_17(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_19(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_18(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_20(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_19(Macro, Arg, __VA_ARGS__))

#define __VOXEL_FOREACH_ONE_ARG_COMMA_21(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_20(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_22(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_21(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_23(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_22(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_24(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_23(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_25(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_24(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_26(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_25(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_27(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_26(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_28(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_27(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_29(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_28(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_30(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_29(Macro, Arg, __VA_ARGS__))

#define __VOXEL_FOREACH_ONE_ARG_COMMA_31(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_30(Macro, Arg, __VA_ARGS__))
#define __VOXEL_FOREACH_ONE_ARG_COMMA_32(Macro, Arg, X, ...) Macro(Arg, X), VOXEL_EXPAND(__VOXEL_FOREACH_ONE_ARG_COMMA_31(Macro, Arg, __VA_ARGS__))

#define VOXEL_FOREACH_ONE_ARG_COMMA(Macro, Arg, ...) \
	VOXEL_EXPAND(__VOXEL_GET_NTH_ARG(__VA_ARGS__, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_32, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_31, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_30, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_29, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_28, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_27, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_26, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_25, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_24, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_23, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_22, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_21, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_20, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_19, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_18, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_17, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_16, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_15, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_14, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_13, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_12, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_11, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_10, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_09, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_08, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_07, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_06, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_05, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_04, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_03, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_02, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_01, \
	__VOXEL_FOREACH_ONE_ARG_COMMA_00)(Macro, Arg, __VA_ARGS__))

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define INTERNAL_BEGIN_VOXEL_NAMESPACE(Name) namespace Name {
#define INTERNAL_END_VOXEL_NAMESPACE(Name) }
#define INTERNAL_VOXEL_USE_NAMESPACE(Name) ::Name
#define INTERNAL_VOXEL_USE_NAMESPACE_TYPE(Type) ::Type Type
#define INTERNAL_VOXEL_USE_NAMESPACE_TYPES(Namespace, Type) using Type = Voxel::Namespace::Type;

#define BEGIN_VOXEL_NAMESPACE(...) namespace Voxel { VOXEL_FOREACH(INTERNAL_BEGIN_VOXEL_NAMESPACE, __VA_ARGS__)
#define END_VOXEL_NAMESPACE(...) \
	INTELLISENSE_ONLY(struct { void Dummy() { using namespace Voxel VOXEL_FOREACH(INTERNAL_VOXEL_USE_NAMESPACE, __VA_ARGS__); } };) \
	VOXEL_FOREACH_SUFFIX(INTERNAL_END_VOXEL_NAMESPACE, __VA_ARGS__) }

#define VOXEL_USE_NAMESPACE(...) using namespace Voxel VOXEL_FOREACH(INTERNAL_VOXEL_USE_NAMESPACE, __VA_ARGS__)
#define VOXEL_USE_NAMESPACE_TYPES(Namespace, ...) VOXEL_FOREACH_ONE_ARG(INTERNAL_VOXEL_USE_NAMESPACE_TYPES, Namespace, __VA_ARGS__)


#define INTERNAL_VOXEL_FWD_NAMESPACE_CLASS(Type) class Type;
#define INTERNAL_VOXEL_FWD_NAMESPACE_STRUCT(Type) struct Type;

#define VOXEL_FWD_DECLARE_NAMESPACE_CLASS(...) \
	namespace Voxel \
	{ \
		VOXEL_FOREACH_IMPL(INTERNAL_BEGIN_VOXEL_NAMESPACE, INTERNAL_VOXEL_FWD_NAMESPACE_CLASS, VOXEL_VOID_MACRO, __VA_ARGS__) \
		VOXEL_FOREACH_IMPL(INTERNAL_END_VOXEL_NAMESPACE, VOXEL_VOID_MACRO, VOXEL_VOID_MACRO, __VA_ARGS__) \
	}

#define VOXEL_FWD_DECLARE_NAMESPACE_STRUCT(...) \
	namespace Voxel \
	{ \
		VOXEL_FOREACH_IMPL(INTERNAL_BEGIN_VOXEL_NAMESPACE, INTERNAL_VOXEL_FWD_NAMESPACE_STRUCT, VOXEL_VOID_MACRO, __VA_ARGS__) \
		VOXEL_FOREACH_IMPL(INTERNAL_END_VOXEL_NAMESPACE, VOXEL_VOID_MACRO, VOXEL_VOID_MACRO, __VA_ARGS__) \
	}

#define VOXEL_FWD_NAMESPACE_CLASS(NewName, ...) \
	VOXEL_FWD_DECLARE_NAMESPACE_CLASS(__VA_ARGS__) \
	using NewName = Voxel VOXEL_FOREACH(INTERNAL_VOXEL_USE_NAMESPACE, __VA_ARGS__);

#define VOXEL_FWD_NAMESPACE_STRUCT(NewName, ...) \
	VOXEL_FWD_DECLARE_NAMESPACE_STRUCT(__VA_ARGS__) \
	using NewName = Voxel VOXEL_FOREACH(INTERNAL_VOXEL_USE_NAMESPACE, __VA_ARGS__);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define DECLARE_VOXEL_VERSION(...) \
	struct \
	{ \
		enum Type : int32 \
		{ \
			__VA_ARGS__, \
			__VersionPlusOne, \
			LatestVersion = __VersionPlusOne - 1 \
		}; \
	}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename T>
struct TVoxelUFunctionOverride
{
	struct FFrame : T
	{
		struct FCode
		{
			FORCEINLINE FCode operator+=(FCode) const { return {}; }
			FORCEINLINE FCode operator!() const { return {}; }
		};
		FCode Code;
	};

	using FNativeFuncPtr = void (*)(UObject* Context, FFrame& Stack, void* Result);

	struct FNameNativePtrPair
	{
		const char* NameUTF8;
		FNativeFuncPtr Pointer;
	};

	struct FNativeFunctionRegistrar
	{
		FNativeFunctionRegistrar(UClass* Class, const ANSICHAR* InName, FNativeFuncPtr InPointer)
		{
			RegisterFunction(Class, InName, InPointer);
		}
		static void RegisterFunction(UClass* Class, const ANSICHAR* InName, FNativeFuncPtr InPointer)
		{
			::FNativeFunctionRegistrar::RegisterFunction(Class, InName, reinterpret_cast<::FNativeFuncPtr>(InPointer));
		}
		static void RegisterFunction(UClass* Class, const WIDECHAR* InName, FNativeFuncPtr InPointer)
		{
			::FNativeFunctionRegistrar::RegisterFunction(Class, InName, reinterpret_cast<::FNativeFuncPtr>(InPointer));
		}
		static void RegisterFunctions(UClass* Class, const FNameNativePtrPair* InArray, int32 NumFunctions)
		{
			::FNativeFunctionRegistrar::RegisterFunctions(Class, reinterpret_cast<const ::FNameNativePtrPair*>(InArray), NumFunctions);
		}
	};
};

#define VOXEL_UFUNCTION_OVERRIDE(NewFrameClass) \
	using FFrame = TVoxelUFunctionOverride<NewFrameClass>::FFrame; \
	using FNativeFuncPtr = TVoxelUFunctionOverride<NewFrameClass>::FNativeFuncPtr; \
	using FNameNativePtrPair = TVoxelUFunctionOverride<NewFrameClass>::FNameNativePtrPair; \
	using FNativeFunctionRegistrar = TVoxelUFunctionOverride<NewFrameClass>::FNativeFunctionRegistrar;