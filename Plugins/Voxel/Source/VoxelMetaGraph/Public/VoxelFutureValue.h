// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelSharedPinValue.h"

class FVoxelTask;

enum class EVoxelTaskThread
{
	AnyThread,
	GameThread,
	RenderThread,
	AsyncThread
};

class VOXELMETAGRAPH_API FVoxelFuturePinValueState : public TSharedFromThis<FVoxelFuturePinValueState>
{
public:
	const FVoxelPinType Type;

	explicit FVoxelFuturePinValueState(const FVoxelPinType& Type)
		: Type(Type)
	{
	}
	~FVoxelFuturePinValueState();
	UE_NONCOPYABLE(FVoxelFuturePinValueState);

	void AddDependentTask(const TSharedRef<FVoxelTask>& Task);
	void AddLinkedState(const TSharedRef<FVoxelFuturePinValueState>& State);

	void SetValue(const FVoxelSharedPinValue& NewValue);

	FORCEINLINE bool IsComplete() const
	{
		return bIsComplete;
	}
	FORCEINLINE const FVoxelSharedPinValue& GetValue_CheckCompleted() const
	{
		checkVoxelSlow(bIsComplete);
		return Value;
	}

private:
	FThreadSafeBool bIsComplete = false;
	FVoxelSharedPinValue Value;

	// TODO Use UE::FSpinLock?
	FVoxelCriticalSection CriticalSection;
	TVoxelArray<TSharedPtr<FVoxelTask>> DependentTasks;
	TVoxelArray<TSharedPtr<FVoxelFuturePinValueState>> LinkedStates;
};

class VOXELMETAGRAPH_API FVoxelFutureValue
{
public:
	using Type = void;

	FVoxelFutureValue() = default;
	FVoxelFutureValue(const FVoxelSharedPinValue& Value)
	{
		if (!Value.IsValid())
		{
			return;
		}

		State = MakeShared<FVoxelFuturePinValueState>(Value.GetType());
		State->SetValue(Value);
	}
	explicit FVoxelFutureValue(const TSharedRef<FVoxelFuturePinValueState>& State)
		: State(State)
	{
	}
	
	FORCEINLINE bool IsValid() const
	{
		return State.IsValid();
	}
	FORCEINLINE bool IsComplete() const
	{
		return State->IsComplete();
	}

	// Not the type of the actual value - just the type of this future
	// Use .Get().GetType() to get actual type once complete
	FORCEINLINE const FVoxelPinType& GetParentType() const
	{
		return State->Type;
	}
	FORCEINLINE const FVoxelSharedPinValue& Get_CheckCompleted() const
	{
		return State->GetValue_CheckCompleted();
	}

	template<typename T>
	FORCEINLINE const T& Get_CheckCompleted() const
	{
		return Get_CheckCompleted().Get<T>();
	}
	template<typename T>
	FORCEINLINE TSharedRef<const T> GetShared_CheckCompleted() const
	{
		return Get_CheckCompleted().GetSharedStruct<T>().ToSharedRef();
	}
	template<typename T>
	FORCEINLINE TSharedRef<T> GetSharedCopy_CheckCompleted() const
	{
		return Get_CheckCompleted().GetSharedStructCopy<T>().ToSharedRef();
	}

	void OnComplete(EVoxelTaskThread Thread, TVoxelFunction<void()>&& Lambda) const;

private:
	TSharedPtr<FVoxelFuturePinValueState> State;

	friend class FVoxelTask;
};

template<typename T>
class TVoxelFutureValueImpl : public FVoxelFutureValue
{
public:
	using Type = T;

	FORCEINLINE TVoxelFutureValueImpl()
		: FVoxelFutureValue(FVoxelSharedPinValue::Make<T>())
	{
	}
	FORCEINLINE TVoxelFutureValueImpl(const FVoxelSharedPinValue& Value)
		: FVoxelFutureValue(Value)
	{
	}
	template<typename OtherType, typename = typename TEnableIf<TIsDerivedFrom<OtherType, T>::Value && !TIsSame<OtherType, T>::Value>::Type>
	FORCEINLINE TVoxelFutureValueImpl(const TVoxelFutureValueImpl<OtherType>& Value)
		: FVoxelFutureValue(FVoxelFutureValue(Value))
	{
		checkVoxelSlow(Value.GetParentType().template IsDerivedFrom<T>());
	}
	explicit TVoxelFutureValueImpl(const FVoxelFutureValue& Value)
		: FVoxelFutureValue(Value)
	{
		check(Value.GetParentType().IsDerivedFrom<T>());
	}

	template<typename OtherType, typename = typename TEnableIf<TIsDerivedFrom<OtherType, T>::Value>::Type>
	FORCEINLINE TVoxelFutureValueImpl(const OtherType& Value)
		: FVoxelFutureValue(FVoxelSharedPinValue::Make(Value))
	{
	}
	template<typename OtherType, typename = typename TEnableIf<TIsDerivedFrom<OtherType, T>::Value>::Type>
	FORCEINLINE TVoxelFutureValueImpl(const TSharedPtr<OtherType>& Value)
		: FVoxelFutureValue(Value.IsValid() ? FVoxelSharedPinValue::Make(Value.ToSharedRef()) : FVoxelSharedPinValue::Make<T>())
	{
	}
	template<typename OtherType, typename = typename TEnableIf<TIsDerivedFrom<OtherType, T>::Value>::Type>
	FORCEINLINE TVoxelFutureValueImpl(const TSharedPtr<const OtherType>& Value)
		: FVoxelFutureValue(Value.IsValid() ? FVoxelSharedPinValue::Make(Value.ToSharedRef()) : FVoxelSharedPinValue::Make<T>())
	{
	}
	template<typename OtherType, typename = typename TEnableIf<TIsDerivedFrom<OtherType, T>::Value>::Type>
	FORCEINLINE TVoxelFutureValueImpl(const TSharedRef<OtherType>& Value)
		: FVoxelFutureValue(FVoxelSharedPinValue::Make(Value))
	{
	}
	template<typename OtherType, typename = typename TEnableIf<TIsDerivedFrom<OtherType, T>::Value>::Type>
	FORCEINLINE TVoxelFutureValueImpl(const TSharedRef<const OtherType>& Value)
		: FVoxelFutureValue(FVoxelSharedPinValue::Make(Value))
	{
	}
	
public:
	template<typename OtherType, typename = typename TEnableIf<TIsDerivedFrom<OtherType, T>::Value && !TIsSame<OtherType, T>::Value>::Type>
	FORCEINLINE const OtherType& Get_CheckCompleted() const
	{
		return FVoxelFutureValue::Get_CheckCompleted<OtherType>();
	}
	template<typename OtherType, typename = typename TEnableIf<TIsDerivedFrom<OtherType, T>::Value && !TIsSame<OtherType, T>::Value>::Type>
	FORCEINLINE TSharedRef<const OtherType> GetShared_CheckCompleted() const
	{
		return FVoxelFutureValue::GetShared_CheckCompleted<OtherType>();
	}
	template<typename OtherType, typename = typename TEnableIf<TIsDerivedFrom<OtherType, T>::Value && !TIsSame<OtherType, T>::Value>::Type>
	FORCEINLINE TSharedRef<OtherType> GetSharedCopy_CheckCompleted() const
	{
		return FVoxelFutureValue::GetSharedCopy_CheckCompleted<OtherType>();
	}
	
public:
	FORCEINLINE const T& Get_CheckCompleted() const
	{
		return FVoxelFutureValue::Get_CheckCompleted<T>();
	}
	FORCEINLINE TSharedRef<const T> GetShared_CheckCompleted() const
	{
		return FVoxelFutureValue::GetShared_CheckCompleted<T>();
	}
	FORCEINLINE TSharedRef<T> GetSharedCopy_CheckCompleted() const
	{
		return FVoxelFutureValue::GetSharedCopy_CheckCompleted<T>();
	}
};

template<typename T>
using TVoxelFutureValue = typename TChooseClass<TIsSame<T, void>::Value, FVoxelFutureValue, TVoxelFutureValueImpl<T>>::Result;