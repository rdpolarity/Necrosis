// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"

template<typename ValueType>
class TVoxelQueryMap
{
public:
	struct FHashedQuery
	{
		explicit FHashedQuery(const FVoxelQuery& Query)
		{
			VOXEL_FUNCTION_COUNTER();

			TSet<UScriptStruct*> Structs;
			for (auto& It : Query.QueryDatas)
			{
				if (Structs.Contains(It.Value->GetStruct()))
				{
					continue;
				}
				Structs.Add(It.Value->GetStruct());
				Datas.Add(It.Value.ToSharedRef());
			}

			Datas.Sort([&](const TSharedRef<const FVoxelQueryData>& A, const TSharedRef<const FVoxelQueryData>& B)
			{
				return A->GetStruct() < B->GetStruct();
			});

			Hash = 0;
			for (const TSharedRef<const FVoxelQueryData>& Data : Datas)
			{
				Hash = FVoxelUtilities::MurmurHash64(
					Hash ^
					FVoxelUtilities::MurmurHash64(uint64(Data->GetStruct())) ^
					FVoxelUtilities::MurmurHash64(uint64(Data->GetQueryTypeHash())));
			}
		}

		uint64 Hash = 0;
		TVoxelArray<TSharedRef<const FVoxelQueryData>> Datas;
	};

	TVoxelQueryMap() = default;

	int32 Num() const
	{
		return Map.Num();
	}

	ValueType* Find(const FHashedQuery& Query)
	{
		return Map.Find(&Query);
	}
	ValueType& Add(const FHashedQuery& Query, const ValueType& Value = {})
	{
		return Map.Add(Queries.Add_GetRef(MakeUniqueCopy(Query)).Get(), Value);
	}
	
	ValueType FindRef(const FHashedQuery& Query)
	{
		if (ValueType* Data = this->Find(Query))
		{
			return *Data;
		}

		return ValueType{};
	}
	ValueType& FindOrAdd(const FHashedQuery& Query)
	{
		if (ValueType* Data = this->Find(Query))
		{
			return *Data;
		}

		return this->Add(Query);
	}
	
	void Remove(const FHashedQuery& Query)
	{
		ensure(Map.Remove(&Query));
	}

	FORCEINLINE auto begin() const -> decltype(auto) { return Map.begin(); }
	FORCEINLINE auto end() const -> decltype(auto) { return Map.end(); }

	auto CreateIterator() -> decltype(auto)
	{
		return Map.CreateIterator();
	}

private:
	struct FFuncs : public TDefaultMapKeyFuncs<const FHashedQuery*, ValueType, false>
	{
		FORCEINLINE static bool Matches(const FHashedQuery* A, const FHashedQuery* B)
		{
			checkVoxelSlow(A);
			checkVoxelSlow(B);

			if (A == B)
			{
				return true;
			}

			if (A->Hash != B->Hash)
			{
				return false;
			}

			return MatchesSlow(A, B);
		}
		FORCEINLINE static uint32 GetKeyHash(const FHashedQuery* Query)
		{
			return Query->Hash;
		}

		FORCENOINLINE static bool MatchesSlow(const FHashedQuery* A, const FHashedQuery* B)
		{
			VOXEL_FUNCTION_COUNTER();

			if (A->Datas.Num() != B->Datas.Num())
			{
				ensureVoxelSlow(false);
				return false;
			}

			const int32 Num = A->Datas.Num();
			for (int32 Index = 0; Index < Num; Index++)
			{
				if (A->Datas[Index]->GetStruct() != B->Datas[Index]->GetStruct())
				{
					ensureVoxelSlow(false);
					return false;
				}
			}

			for (int32 Index = 0; Index < Num; Index++)
			{
				if (!A->Datas[Index]->IsQueryIdentical(*B->Datas[Index]))
				{
					ensureVoxelSlow(false);
					return false;
				}
			}

			return true;
		}
	};
	TVoxelArray<TUniquePtr<FHashedQuery>> Queries;
	TMap<const FHashedQuery*, ValueType, FDefaultSetAllocator, FFuncs> Map;
};