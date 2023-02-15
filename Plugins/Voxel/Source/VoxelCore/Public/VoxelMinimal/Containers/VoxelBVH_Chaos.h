// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "Chaos/AABBTree.h"
#include "VoxelMinimal/VoxelBox.h"
#include "VoxelMinimal/Containers/VoxelArray.h"

class FVoxelDebugManager;

class VOXELCORE_API FVoxelBVH_Chaos
{
public:
	explicit FVoxelBVH_Chaos(const TVoxelArray<FVoxelBox>& AllBounds);

	template<typename LambdaType>
	void ForEachLeaf(const FVoxelBox& Bounds, LambdaType Lambda) const
	{
		struct FVisitor
		{
			LambdaType& Lambda;

			explicit FVisitor(LambdaType& Lambda)
				: Lambda(Lambda)
			{
			}

			FORCEINLINE bool VisitOverlap(const Chaos::TSpatialVisitorData<int32>& Data) const
			{
				Lambda(Data.Payload);
				return true;
			}
			bool VisitSweep(const Chaos::TSpatialVisitorData<int32>& Data, Chaos::FQueryFastData& CurData)
			{
				check(false);
				return false;
			}
			bool VisitRaycast(const Chaos::TSpatialVisitorData<int32>& Data, Chaos::FQueryFastData& CurData)
			{
				check(false);
				return false;
			}
			const void* GetQueryData() const
			{
				return nullptr;
			}
			const void* GetQueryPayload() const
			{
				return nullptr;
			}
		};
		FVisitor Visitor(Lambda);
		Tree.OverlapFast(Chaos::FAABB3(FVector(Bounds.Min), FVector(Bounds.Max)), Visitor);
	}

private:
	struct FWrapper : FVoxelBox
	{
		template <typename TPayloadType>
		TPayloadType GetPayload(int32 Index) const
		{
			return Index;
		}
		bool HasBoundingBox() const
		{
			return true;
		}
		Chaos::FAABB3 BoundingBox() const
		{
			return Chaos::FAABB3(FVector(Min), FVector(Max));
		}
	};
	Chaos::TAABBTree<int32, Chaos::TAABBTreeLeafArray<int32>> Tree;
};