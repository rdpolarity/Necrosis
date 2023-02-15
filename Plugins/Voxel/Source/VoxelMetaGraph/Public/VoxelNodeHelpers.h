// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBuffer.h"
#include "VoxelNodeHelpers.generated.h"

class IVoxelNodeOuter;
class FVoxelNodeRuntime;

struct FVoxelNodeDefaultValueHelper
{
	template<typename T>
	static FVoxelPinValue Get(T*, decltype(nullptr))
	{
		return {};
	}
	template<typename T>
	static FVoxelPinValue Get(T*, T Value)
	{
		return FVoxelPinValue::Make(Value);
	}
	template<typename BufferType, typename = typename TEnableIf<TIsDerivedFrom<BufferType, FVoxelBuffer>::Value>::Type>
	static FVoxelPinValue Get(BufferType*, typename BufferType::UniformType Value)
	{
		return FVoxelPinValue::Make(Value);
	}
	static FVoxelPinValue Get(FName*, FName Value)
	{
		return FVoxelPinValue::Make(Value);
	}
};

struct FVoxelNodeAliases
{
	template<typename T>
	using TValue = ::TVoxelFutureValue<T>;

	template<typename T>
	using TBufferView = ::TVoxelBufferView<T>;

	template<typename T>
	using TVoxelFutureValue [[deprecated]] = TValue<T>;

	template<typename T>
	using TVoxelBufferView [[deprecated]] = TBufferView<T>;
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelNodeInterface : public FVoxelVirtualStruct, public FVoxelNodeAliases
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	virtual const FVoxelNode& GetNode() const;
	virtual TSharedRef<IVoxelNodeOuter> GetOuter() const VOXEL_PURE_VIRTUAL({});
};

struct VOXELMETAGRAPH_API FVoxelNodeHelpers
{
	static void RaiseQueryError(const FVoxelNodeInterface& Node, const FVoxelQuery& Query, const UScriptStruct* QueryType);
	static void RaiseBufferError(const FVoxelNodeInterface& Node);
};

#define DEFINE_VOXEL_NODE_IMPL(NodeName, PinName, Impl) \
	INTELLISENSE_ONLY(void VOXEL_APPEND_LINE(NodeName)(FVoxelPinRef PinName) { (void)((NodeName*)nullptr)->PinName ## Pin; (void)PinName; }); \
	struct NodeName ## _ ## PinName ## _ ## Impl : public NodeName \
	{ \
		using ReturnType = TValue<decltype(DeclVal<NodeName>().PinName ## Pin)::Type>; \
		\
		void RaiseBufferError() const \
		{ \
			FVoxelNodeHelpers::RaiseBufferError(*this); \
		} \
		bool IsDefaultValue(const FVoxelPinRef& Pin) const \
		{ \
			return GetNodeRuntime().IsDefaultValue(Pin); \
		} \
		\
		template<typename T, typename = typename TEnableIf<TIsDerivedFrom<T, FVoxelPinRef>::Value>::Type> \
		auto Get(const T& Pin, const FVoxelQuery& Query) const -> decltype(auto) \
		{ \
			return GetNodeRuntime().Get(Pin, Query); \
		} \
		template<typename T, typename = typename TEnableIf<!TIsDerivedFrom<T, FVoxelPinRef>::Value>::Type> \
		auto Get(const FVoxelPinRef& Pin, const FVoxelQuery& Query) const -> decltype(auto) \
		{ \
			return GetNodeRuntime().Get<T>(Pin, Query); \
		} \
		template<typename T, typename = typename TEnableIf<TIsDerivedFrom<T, FVoxelPinRef>::Value>::Type> \
		auto GetBufferView(const T& Pin, const FVoxelQuery& Query) const -> decltype(auto) \
		{ \
			return GetNodeRuntime().GetBufferView(Pin, Query); \
		} \
		template<typename T, typename = typename TEnableIf<!TIsDerivedFrom<T, FVoxelPinRef>::Value>::Type> \
		auto GetBufferView(const FVoxelPinRef& Pin, const FVoxelQuery& Query) const -> decltype(auto) \
		{ \
			return GetNodeRuntime().GetBufferView<T>(Pin, Query); \
		} \
		\
		template<typename T, typename = typename TEnableIf<TIsDerivedFrom<T, FVoxelPinArrayRef>::Value>::Type, typename = void> \
		auto Get(const T& Pin, const FVoxelQuery& Query) const -> decltype(auto) \
		{ \
			return GetNodeRuntime().Get(Pin, Query); \
		} \
		template<typename T, typename = typename TEnableIf<!TIsDerivedFrom<T, FVoxelPinArrayRef>::Value>::Type> \
		auto Get(const FVoxelPinArrayRef& Pin, const FVoxelQuery& Query) const -> decltype(auto) \
		{ \
			return GetNodeRuntime().Get<T>(Pin, Query); \
		} \
		template<typename T, typename = typename TEnableIf<TIsDerivedFrom<T, FVoxelPinArrayRef>::Value>::Type, typename = void> \
		auto GetBufferView(const T& Pin, const FVoxelQuery& Query) const -> decltype(auto) \
		{ \
			return GetNodeRuntime().GetBufferView(Pin, Query); \
		} \
		template<typename T, typename = typename TEnableIf<!TIsDerivedFrom<T, FVoxelPinArrayRef>::Value>::Type> \
		auto GetBufferView(const FVoxelPinArrayRef& Pin, const FVoxelQuery& Query) const -> decltype(auto) \
		{ \
			return GetNodeRuntime().GetBufferView<T>(Pin, Query); \
		} \
		\
		template<typename T, typename = typename TEnableIf<TIsDerivedFrom<T, IVoxelSubsystem>::Value>::Type> \
		T& GetSubsystem() const \
		{ \
			return GetNodeRuntime().GetSubsystem<T>(); \
		} \
		\
		static FVoxelFutureValue Compute(const FVoxelNode& Node, const FVoxelQuery& Query) \
		{ \
			const NodeName& TypedNode = CastChecked<NodeName>(Node); \
			const FVoxelNodeRuntime::FPinData& PinData = Node.GetNodeRuntime().GetPinData(TypedNode.PinName ## Pin); \
			return static_cast<const NodeName ## _ ## PinName ## _ ## Impl&>(TypedNode).ComputeImpl( \
				PinData.Type, \
				PinData.StatName, \
				Query); \
		} \
		ReturnType ComputeImpl( \
			const FVoxelPinType& ReturnPinType, \
			FName ReturnPinStatName, \
			const FVoxelQuery& Query) const; \
	}; \
	VOXEL_RUN_ON_STARTUP_GAME(Register ## NodeName ## _ ## PinName ## _ ## Impl) \
	{ \
		FVoxelNodeComputePtrs::Initialize ## Impl( \
			NodeName::StaticStruct(), \
			#PinName, \
			&NodeName ## _ ## PinName ## _ ## Impl::Compute, \
			#NodeName "." #PinName " " + FString(__FILE__) + ":" + FString::FromInt(__LINE__)); \
	} \
	NodeName ## _ ## PinName ## _ ## Impl::ReturnType \
	NodeName ## _ ## PinName ## _ ## Impl::ComputeImpl( \
		const FVoxelPinType& ReturnPinType, \
		const FName ReturnPinStatName, \
		const FVoxelQuery& Query) const

#define DEFINE_VOXEL_NODE(NodeName, PinName) DEFINE_VOXEL_NODE_IMPL(NodeName, PinName, Generic)
#define DEFINE_VOXEL_NODE_CPU(NodeName, PinName) DEFINE_VOXEL_NODE_IMPL(NodeName, PinName, Cpu)
#define DEFINE_VOXEL_NODE_GPU(NodeName, PinName) DEFINE_VOXEL_NODE_IMPL(NodeName, PinName, Gpu)

#define FindVoxelQueryData(Type, Name) \
	checkStatic(TIsDerivedFrom<Type, FVoxelQueryData>::Value); \
	if (!Query.Find<Type>()) { FVoxelNodeHelpers::RaiseQueryError(*this, Query, Type::StaticStruct()); return {}; } \
	const TSharedRef<const Type> Name = Query.Find<Type>().ToSharedRef();

#define CheckVoxelBuffersNum(...) \
	if (!FVoxelBufferAccessor(__VA_ARGS__).IsValid()) \
	{ \
		FVoxelNodeHelpers::RaiseBufferError(*this); \
		return {}; \
	}

#define ComputeVoxelBuffersNum(...) FVoxelBufferAccessor(__VA_ARGS__).Num(); CheckVoxelBuffersNum(__VA_ARGS__)

template<typename T, typename = void>
struct TVoxelNodeLambdaArg
{
	static const T& Get(const T& Arg)
	{
		return Arg;
	}
};

template<typename T>
using TVoxelNodeByValue = TOr<
	TIsTriviallyDestructible<T>,
	TIsSame<T, FVoxelSharedPinValue>,
	TAnd<TIsDerivedFrom<T, FVoxelBuffer>, TNot<TIsSame<T, FVoxelBuffer>>>,
	TAnd<TIsDerivedFrom<T, FVoxelBufferView>, TNot<TIsSame<T, FVoxelBufferView>>>>;

template<>
struct TVoxelNodeLambdaArg<FVoxelFutureValue>
{
	static const FVoxelSharedPinValue& Get(const FVoxelFutureValue& Arg)
	{
		return Arg.Get_CheckCompleted();
	}
};

template<typename T>
struct TVoxelNodeLambdaArg<TVoxelFutureValueImpl<T>, typename TEnableIf<TVoxelNodeByValue<T>::Value>::Type>
{
	static const T& Get(const TVoxelFutureValueImpl<T>& Arg)
	{
		return Arg.Get_CheckCompleted();
	}
};
template<typename T>
struct TVoxelNodeLambdaArg<TVoxelFutureValueImpl<T>, typename TEnableIf<!TVoxelNodeByValue<T>::Value>::Type>
{
	static TSharedRef<const T> Get(const TVoxelFutureValueImpl<T>& Arg)
	{
		return Arg.GetShared_CheckCompleted();
	}
};

template<typename T>
struct TVoxelNodeLambdaArg<TArray<TVoxelFutureValueImpl<T>>, typename TEnableIf<TVoxelNodeByValue<T>::Value>::Type>
{
	static TArray<T> Get(const TArray<TVoxelFutureValueImpl<T>>& Arg)
	{
		TArray<T> Result;
		Result.Reserve(Arg.Num());
		for (const TVoxelFutureValueImpl<T>& Element : Arg)
		{
			Result.Add(Element.Get_CheckCompleted());
		}
		return Result;
	}
};
template<typename T>
struct TVoxelNodeLambdaArg<TArray<TVoxelFutureValueImpl<T>>, typename TEnableIf<!TVoxelNodeByValue<T>::Value>::Type>
{
	static TArray<TSharedRef<const T>> Get(const TArray<TVoxelFutureValueImpl<T>>& Arg)
	{
		TArray<TSharedRef<const T>> Result;
		Result.Reserve(Arg.Num());
		for (const TVoxelFutureValueImpl<T>& Element : Arg)
		{
			Result.Add(Element.GetShared_CheckCompleted());
		}
		return Result;
	}
};

template<typename T>
struct TVoxelNodeLambdaArgType
{
	using Type = VOXEL_GET_TYPE(TVoxelNodeLambdaArg<T>::Get(DeclVal<T>()));
};

template<typename T, EVoxelTaskThread Thread, typename... ArgTypes>
struct TVoxelNodeOnComplete
{
	const FVoxelNodeInterface& Node;
	const FVoxelPinType Type;
	const FName StatName;
	TTuple<ArgTypes...> Args;

	using FDependencies = TVoxelArray<FVoxelFutureValue, TInlineAllocator<16>>;
	FDependencies Dependencies;

	TVoxelNodeOnComplete(
		const FVoxelNodeInterface& Node,
		const FVoxelPinType& Type,
		const FName StatName,
		const ArgTypes&... Args)
		: Node(Node)
		, Type(Type.WithoutTag())
		, StatName(StatName)
		, Args(Args...)
	{
		if constexpr (!TIsSame<T, void>::Value)
		{
			ensure(Type.Is<T>());
		}
		VOXEL_FOLD_EXPRESSION(TVoxelNodeOnComplete::AddDependencies(Dependencies, Args));
	}

	static void AddDependencies(FDependencies& InDependencies, const FVoxelFutureValue& Value)
	{
		InDependencies.Add(Value);
	}
	template<typename OtherType, typename = typename TEnableIf<TIsDerivedFrom<OtherType, FVoxelFutureValue>::Value>::Type>
	static void AddDependencies(FDependencies& InDependencies, const TArray<OtherType>& Values)
	{
		for (const FVoxelFutureValue& Value : Values)
		{
			InDependencies.Add(Value);
		}
	}

	template<typename OtherType>
	static void AddDependencies(FDependencies&, const TSharedRef<OtherType>&)
	{
	}
	template<typename OtherType>
	static void AddDependencies(FDependencies&, const TSharedPtr<OtherType>&)
	{
	}
	template<typename OtherType>
	static void AddDependencies(FDependencies&, const TArray<TSharedRef<OtherType>>&)
	{
	}
	template<typename OtherType>
	static void AddDependencies(FDependencies&, const TArray<TSharedPtr<OtherType>>&)
	{
	}
	template<typename OtherType, typename = typename TEnableIf<TIsTriviallyDestructible<OtherType>::Value>::Type>
	static void AddDependencies(FDependencies&, OtherType)
	{
	}
	static void AddDependencies(FDependencies&, const FVoxelQuery&)
	{
	}
	static void AddDependencies(FDependencies&, const FVoxelBuffer&)
	{
	}
	static void AddDependencies(FDependencies&, const FVoxelBufferView&)
	{
	}
	template<typename Body>
	static void AddDependencies(FDependencies&, const TFunction<Body>&)
	{
	}
	template<typename OtherType>
	static void AddDependencies(FDependencies&, OtherType*) = delete;

	template<typename LambdaType>
	FORCEINLINE TVoxelFutureValue<T> operator+(LambdaType&& Lambda)
	{
		return this->Execute(::MoveTemp(Lambda), TMakeIntegerSequence<uint32, sizeof...(ArgTypes)>());
	}

	template<typename LambdaType, uint32... ArgIndices>
	FORCEINLINE TVoxelFutureValue<T> Execute(LambdaType&& Lambda, TIntegerSequence<uint32, ArgIndices...>)
	{
		return TVoxelFutureValue<T>(FVoxelTask::New(
			MakeShared<FVoxelTaskStat>(Node.GetNode()),
			Type,
			StatName,
			Thread,
			Dependencies,
			[Outer = Node.GetOuter(), Args = ::MoveTemp(Args), Lambda = ::MoveTemp(Lambda)]
			{
				(void)Outer;

				if constexpr (Thread == EVoxelTaskThread::RenderThread)
				{
					return Lambda(nullptr, TVoxelNodeLambdaArg<ArgTypes>::Get(Args.template Get<ArgIndices>())..., FVoxelRDGBuilderScope::Get());
				}
				else
				{
					return Lambda(nullptr, TVoxelNodeLambdaArg<ArgTypes>::Get(Args.template Get<ArgIndices>())...);
				}
			}));
	}
};

#define VOXEL_SETUP_ON_COMPLETE(Pin) \
	using ReturnType = decltype(Pin); \
	const FVoxelNodeRuntime::FPinData& VOXEL_APPEND_LINE(PinData) = GetNodeRuntime().GetPinData(Pin); \
	const FVoxelPinType ReturnPinType = VOXEL_APPEND_LINE(PinData).Type; \
	const FName ReturnPinStatName = VOXEL_APPEND_LINE(PinData).StatName;

#define VOXEL_SETUP_ON_COMPLETE_MANUAL(InReturnType, StatName) \
	using ReturnType = TVoxelPinRef<InReturnType>; \
	FVoxelPinType ReturnPinType = FVoxelPinType::Make<InReturnType>(); \
	FName ReturnPinStatName = STATIC_FNAME(StatName);

#define VOXEL_ON_COMPLETE_CAPTURE this, Query, ReturnPinType, ReturnPinStatName

#define VOXEL_ON_COMPLETE_IMPL_CHECK(Data) \
	{ \
		TVoxelNodeOnComplete<void, EVoxelTaskThread::AnyThread>::FDependencies Dependencies; \
		TVoxelNodeOnComplete<void, EVoxelTaskThread::AnyThread>::AddDependencies(Dependencies, Data); \
	}

#define VOXEL_ON_COMPLETE_IMPL_DECLTYPE(Data) , TRemoveConst<VOXEL_GET_TYPE(Data)>::Type
#define VOXEL_ON_COMPLETE_IMPL_LAMBDA_ARGS(Data) , const TVoxelNodeLambdaArgType<TRemoveConst<VOXEL_GET_TYPE(Data)>::Type>::Type& Data

#define VOXEL_ON_COMPLETE_IMPL_AnyThread
#define VOXEL_ON_COMPLETE_IMPL_AsyncThread
#define VOXEL_ON_COMPLETE_IMPL_GameThread
#define VOXEL_ON_COMPLETE_IMPL_RenderThread , FRDGBuilder& GraphBuilder

#define VOXEL_ON_COMPLETE_CUSTOM(Type, StatName, Thread, ...) \
	(INTELLISENSE_ONLY([__VA_ARGS__] { VOXEL_FOREACH(VOXEL_ON_COMPLETE_IMPL_CHECK, DeclVal<FVoxelFutureValue>(), ##__VA_ARGS__); },) \
	TVoxelNodeOnComplete<Type, EVoxelTaskThread::Thread VOXEL_FOREACH(VOXEL_ON_COMPLETE_IMPL_DECLTYPE, ##__VA_ARGS__)>(*this, FVoxelPinType::Make<Type>(), StatName, ##__VA_ARGS__)) + \
	[this, Query](void* VOXEL_FOREACH(VOXEL_ON_COMPLETE_IMPL_LAMBDA_ARGS, ##__VA_ARGS__) VOXEL_ON_COMPLETE_IMPL_ ## Thread) -> TValue<Type>

#define VOXEL_ON_COMPLETE(Thread, ...) \
	(INTELLISENSE_ONLY([__VA_ARGS__] { VOXEL_FOREACH(VOXEL_ON_COMPLETE_IMPL_CHECK, DeclVal<FVoxelFutureValue>(), ##__VA_ARGS__); },) \
	TVoxelNodeOnComplete<decltype(ReturnType())::Type, EVoxelTaskThread::Thread VOXEL_FOREACH(VOXEL_ON_COMPLETE_IMPL_DECLTYPE, ##__VA_ARGS__)>(*this, ReturnPinType, ReturnPinStatName, ##__VA_ARGS__)) + \
	[this, Query = Query, ReturnPinType = ReturnPinType, ReturnPinStatName = ReturnPinStatName](void* VOXEL_FOREACH(VOXEL_ON_COMPLETE_IMPL_LAMBDA_ARGS, ##__VA_ARGS__) VOXEL_ON_COMPLETE_IMPL_ ## Thread) -> TValue<decltype(ReturnType())::Type>