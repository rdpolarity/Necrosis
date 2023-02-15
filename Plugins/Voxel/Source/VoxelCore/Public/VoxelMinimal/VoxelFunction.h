// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoxelMacros.h"

namespace UE
{
namespace Core
{
namespace Private
{
namespace Function
{
	struct FVoxelDummyFunctionType;
	
	template<>
	struct TFunctionRefBase<FVoxelDummyFunctionType, FVoxelDummyFunctionType>
	{
		// Go through intermediary struct to force ParamTypes
		template <typename StorageType, typename Ret, typename... ParamTypes>
		struct TFastCall
		{
			// Since ParamTypes is EXACTLY what we want as it's the function arg types, there's no need to forward
			static Ret Call(const TFunctionRefBase<StorageType, Ret(ParamTypes...)>& Function, ParamTypes... Params)
			{
#if VOXEL_DEBUG
				// 40% faster to skip CheckCallable in trivial functions
				Function.CheckCallable();
#endif
				return Function.Callable(Function.Storage.GetPtr(), Params...);
			}
		};
		template <typename StorageType, typename Ret, typename... ParamTypes>
		static TFastCall<StorageType, Ret, ParamTypes...> FastCall(const TFunctionRefBase<StorageType, Ret (ParamTypes...)>& Function)
		{
			return {};
		}
	};
	using FVoxelAccessor = TFunctionRefBase<FVoxelDummyFunctionType, FVoxelDummyFunctionType>;
}
}
}
}

template <typename FuncType>
class TVoxelFunctionRef : public TFunctionRef<FuncType>
{
public:
	using TFunctionRef<FuncType>::TFunctionRef;
	
	template<typename... ParamTypes>
	FORCEINLINE decltype(auto) operator()(ParamTypes&&... Params) const
	{
		return UE::Core::Private::Function::FVoxelAccessor::FastCall(*this).Call(*this, Forward<ParamTypes>(Params)...);
	}
};

// Copy of TFunction that skips the check on every call
template <typename FuncType>
class TVoxelFunction final : public UE::Core::Private::Function::TFunctionRefBase<UE::Core::Private::Function::TFunctionStorage<false>, FuncType>
{
	using Super = UE::Core::Private::Function::TFunctionRefBase<UE::Core::Private::Function::TFunctionStorage<false>, FuncType>;

public:
	/**
	 * Default constructor.
	 */
	TVoxelFunction(TYPE_OF_NULLPTR = nullptr)
	{
	}

	/**
	 * Constructor which binds a TFunction to any function object.
	 */
	template <
		typename FunctorType,
		typename = typename TEnableIf<
			TAnd<
				TNot<TIsTFunction<typename TDecay<FunctorType>::Type>>,
				UE::Core::Private::Function::TFuncCanBindToFunctor<FuncType, FunctorType>
			>::Value
		>::Type
	>
	TVoxelFunction(FunctorType&& InFunc)
		: Super(Forward<FunctorType>(InFunc))
	{
		// This constructor is disabled for TFunction types so it isn't incorrectly selected as copy/move constructors.

		// This is probably a mistake if you expect TFunction to take a copy of what
		// TFunctionRef is bound to, because that's not possible.
		//
		// If you really intended to bind a TFunction to a TFunctionRef, you can just
		// wrap it in a lambda (and thus it's clear you're just binding to a call to another
		// reference):
		//
		// TFunction<int32(float)> MyFunction = [MyFunctionRef](float F) { return MyFunctionRef(F); };
		static_assert(!TIsTFunctionRef<typename TDecay<FunctorType>::Type>::Value, "Cannot construct a TFunction from a TFunctionRef");
	}

	TVoxelFunction(TVoxelFunction&&) = default;
	TVoxelFunction(const TVoxelFunction& Other) = default;
	~TVoxelFunction() = default;

	/**
	 * Move assignment operator.
	 */
	TVoxelFunction& operator=(TVoxelFunction&& Other)
	{
		Swap(*this, Other);
		return *this;
	}

	/**
	 * Copy assignment operator.
	 */
	TVoxelFunction& operator=(const TVoxelFunction& Other)
	{
		TVoxelFunction Temp = Other;
		Swap(*this, Temp);
		return *this;
	}

	/**
	 * Tests if the TFunction is callable.
	 */
	FORCEINLINE explicit operator bool() const
	{
		return Super::IsSet();
	}
	
	template<typename... ParamTypes>
	FORCEINLINE decltype(auto) operator()(ParamTypes&&... Params) const
	{
		return UE::Core::Private::Function::FVoxelAccessor::FastCall(*this).Call(*this, Forward<ParamTypes>(Params)...);
	}
};