// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "Async/Async.h"
#include "Misc/ScopedSlowTask.h"

namespace FVoxelUtilities
{
	// Call this when you pin a shared ptr on another thread that needs to always be deleted on the game thread
	template<typename T>
	void RunOnGameThread(T&& Lambda)
	{
		if (IsInGameThread())
		{
			Lambda();
		}
		else
		{
			check(FTaskGraphInterface::IsRunning());
			AsyncTask(ENamedThreads::GameThread, [Lambda = MoveTemp(Lambda)]
			{
				VOXEL_LLM_SCOPE();
				Lambda();
			});
		}
	}
	
	template<typename T>
	struct TGameThreadDeleter
	{
		void operator()(T* Object) const
		{
			if (!Object) return;
			
			FVoxelUtilities::RunOnGameThread([=]()
			{
				delete Object;
			});
		}

		template<typename... TArgs>
		static TSharedRef<T> New(TArgs&&... Args)
		{
			return TSharedPtr<T>(new T(Forward<TArgs>(Args)...), TGameThreadDeleter<T>()).ToSharedRef();
		}
	};
	
	template<typename KeyType, typename ValueType, typename SetAllocator, typename KeyFuncs>
	struct TMapWithPairs : TMap<KeyType, ValueType, SetAllocator, KeyFuncs>
	{
		auto& GetPairs()
		{
			return TMap<KeyType, ValueType, SetAllocator, KeyFuncs>::Pairs;
		}
	};

	template<typename KeyType, typename ValueType, typename SetAllocator, typename KeyFuncs, typename LambdaType>
	void ParallelMapIterate(TMap<KeyType, ValueType, SetAllocator, KeyFuncs>& Map, EParallelForFlags Flags, LambdaType Lambda)
	{
		auto& Pairs = static_cast<TMapWithPairs<KeyType, ValueType, SetAllocator, KeyFuncs>&>(Map).GetPairs();

		if (!ensure(Pairs.GetMaxIndex() == Pairs.Num()))
		{
			// This is needed for the parallel for to not have holes - otherwise Index passed to the lambda is invalid
			Map.Compact();
		}
		ensure(Pairs.GetMaxIndex() == Pairs.Num());

		ParallelForTemplate(Pairs.GetMaxIndex(), [&](int32 Index)
		{
			TPair<KeyType, ValueType>& Pair = Pairs[FSetElementId::FromInteger(Index)];
			Lambda(Index, static_cast<const KeyType&>(Pair.Key), Pair.Value);
		}, Flags);
	}
	template<typename KeyType, typename ValueType, typename SetAllocator, typename KeyFuncs, typename LambdaType>
	void ParallelMapIterate(const TMap<KeyType, ValueType, SetAllocator, KeyFuncs>& Map, EParallelForFlags Flags, LambdaType Lambda)
	{
		ParallelMapIterate(VOXEL_CONST_CAST(Map), Flags, [&](int32 Index, const KeyType& Key, const ValueType& Value)
		{
			Lambda(Index, Key, Value);
		});
	}
	
	template<typename MapType, typename LambdaType>
	void ParallelMapIterate_WithProgress(MapType& Map, EParallelForFlags Flags, const FText& Text, LambdaType Lambda)
	{
		FScopedSlowTask SlowTask(Map.Num(), Text);

		FThreadSafeCounter NumProcessed;
		int32 LastNumProcessed = 0;

		ParallelMapIterate(Map,
			Flags,
			[&](auto Index, auto& Key, auto& Value)
			{
				Lambda(Index, Key, Value);

				const int32 NewNumProcessed = NumProcessed.Increment();

				if (IsInGameThread() && NewNumProcessed > LastNumProcessed)
				{
					SlowTask.EnterProgressFrame(NewNumProcessed - LastNumProcessed);
					LastNumProcessed = NewNumProcessed;
				};
			});
	}
	
	template<typename LambdaType>
	void ParallelFor_WithProgress(int32 Num, const FText& Text, LambdaType Lambda, EParallelForFlags Flags = EParallelForFlags::None)
	{
		FScopedSlowTask SlowTask(Num, Text);

		FThreadSafeCounter NumProcessed;
		int32 LastNumProcessed = 0;

		ParallelFor(Num, [&](int32 Index)
		{
			Lambda(Index);

			const int32 NewNumProcessed = NumProcessed.Increment();

			if (IsInGameThread() && NewNumProcessed > LastNumProcessed)
			{
				SlowTask.EnterProgressFrame(NewNumProcessed - LastNumProcessed);
				LastNumProcessed = NewNumProcessed;
			};
		}, Flags);
	}
}

template<typename T, typename... TArgs>
TSharedRef<T> MakeShared_GameThread(TArgs&&... Args)
{
	return FVoxelUtilities::TGameThreadDeleter<T>::New(Forward<TArgs>(Args)...);
}

template<typename ArrayType, typename LambdaType>
typename TEnableIf<sizeof(GetNum(DeclVal<ArrayType>())) != 0>::Type ParallelFor(
	ArrayType& Array,
	LambdaType Lambda,
	EParallelForFlags Flags = EParallelForFlags::None,
	int32 DefaultNumThreads = FTaskGraphInterface::Get().GetNumWorkerThreads())
{
	const int64 ArrayNum = GetNum(Array);
	const int64 NumThreads = FMath::Clamp<int64>(DefaultNumThreads, 1, ArrayNum);
	::ParallelFor(NumThreads, [&](int64 ThreadIndex)
	{
		const int64 ElementsPerThreads = FVoxelUtilities::DivideCeil_Positive(ArrayNum, NumThreads);

		const int64 StartIndex = ThreadIndex * ElementsPerThreads;
		const int64 EndIndex = FMath::Min((ThreadIndex + 1) * ElementsPerThreads, ArrayNum);

		for (int64 ElementIndex = StartIndex; ElementIndex < EndIndex; ElementIndex++)
		{
			Lambda(Array[ElementIndex]);
		}
	}, Flags);
}