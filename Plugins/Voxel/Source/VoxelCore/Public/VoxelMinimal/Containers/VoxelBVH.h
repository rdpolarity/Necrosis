// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "VoxelMinimal/Containers/VoxelBVH_Chaos.h"

class FVoxelBVH : public FVoxelBVH_Chaos
{
public:
	using FVoxelBVH_Chaos::FVoxelBVH_Chaos;
};

template<typename T>
class TVoxelBVH
{
public:
	explicit TVoxelBVH(const TArray<T>& Nodes)
		: Nodes(Nodes)
	{
		TVoxelArray<FVoxelBox> Bounds;
		Bounds.Reserve(Nodes.Num());
		for (const T& Node : Nodes)
		{
			Bounds.Add(Node.GetBVHBounds());
		}

		BVH = MakeUnique<FVoxelBVH>(Bounds);
	}

	template<typename LambdaType>
	void ForEachLeaf(const FVoxelBox& Bounds, LambdaType Lambda) const
	{
		BVH->ForEachLeaf(Bounds, [&](int32 Id)
		{
			const T& Node = Nodes[Id];
			if (Node.GetBVHBounds().Intersect(Bounds))
			{
				Lambda(Node);
			}
		});
	}
	template<typename LambdaType>
	void ForEachLeaf(const FVector3d& Position, LambdaType Lambda) const
	{
		BVH->ForEachLeaf(FVoxelBox(Position), [&](int32 Id)
		{
			const T& Node = Nodes[Id];
			if (Node.GetBVHBounds().Contains(Position))
			{
				Lambda(Node);
			}
		});
	}
	template<typename LambdaType>
	void ForEachLeaf(const FVector3f& Position, LambdaType Lambda) const
	{
		ForEachLeaf(FVector3d(Position), Lambda);
	}

	const FVoxelBVH& GetBVH() const
	{
		return *BVH;
	}

private:
	TUniquePtr<FVoxelBVH> BVH;
	TVoxelArray<T> Nodes;
};