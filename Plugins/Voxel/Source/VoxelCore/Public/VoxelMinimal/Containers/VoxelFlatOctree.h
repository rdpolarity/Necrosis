// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "Containers/StaticArray.h"
#include "VoxelMinimal/VoxelIntBox.h"
#include "VoxelMinimal/Containers/VoxelSparseArray.h"

struct FVoxelFlatOctreeNode
{
public:
	FVoxelFlatOctreeNode() = default;

	bool IsValid() const
	{
		return Index != -1;
	}

	static FVoxelFlatOctreeNode Root()
	{
		return FVoxelFlatOctreeNode(0, 0);
	}

	bool operator==(const FVoxelFlatOctreeNode& Other) const
	{
		return Index == Other.Index;
	}
	bool operator!=(const FVoxelFlatOctreeNode& Other) const
	{
		return Index != Other.Index;
	}

private:
	FVoxelFlatOctreeNode(int32 GroupIndex, int32 ChildIndex)
		: Index(8 * GroupIndex + ChildIndex)
	{
		checkVoxelSlow(0 <= ChildIndex && ChildIndex < 8);
		checkVoxelSlow(0 <= Index);
		checkVoxelSlow(Index != 0 || ChildIndex == 0);
	}
	
	int32 Index = -1;

	template<typename>
	friend class TVoxelFlatOctree;
};

// While this is using more memory, it makes bounds check faster
#define VOXEL_FLAT_OCTREE_CACHE_BOUNDS 1

template<typename T>
class TVoxelFlatOctree
{
	struct FNodeStorage;

public:
	struct FNode : FVoxelFlatOctreeNode
	{
		FNode() = default;
		FNode(FVoxelFlatOctreeNode Node)
			: FVoxelFlatOctreeNode(Node)
		{
		}
		FNode(int32 GroupIndex, int32 ChildIndex, const TVoxelFlatOctree* Octree)
			: FVoxelFlatOctreeNode(GroupIndex, ChildIndex)
#if VOXEL_DEBUG
			, DebugOctree(reinterpret_cast<const FNodeStorage*>(&Octree->AllNodes[0]))
			, DebugChunkSize(Octree->ChunkSize)
#endif
		{
		}

#if VOXEL_DEBUG
		const FNodeStorage* DebugOctree = nullptr;
		int32 DebugChunkSize = 0;
#endif
	};
	
	using FNodeData = T;

	explicit TVoxelFlatOctree(int32 ChunkSize, int32 Depth)
		: ChunkSize(ChunkSize)
	{
		check(FMath::IsPowerOfTwo(ChunkSize));

		// Allocate root node
		AllNodes.Add({});

		FNodeStorage& RootNode = GetNode(FNode::Root());
		RootNode.Height = Depth;
		RootNode.Center = FIntVector::ZeroValue;
#if VOXEL_FLAT_OCTREE_CACHE_BOUNDS
		RootNode.CachedBounds = GetNodeBoundsImpl(RootNode);
#endif
	}
	
	void MoveFrom(TVoxelFlatOctree<T>& Other)
	{
		ensure(ChunkSize == Other.ChunkSize);
		AllNodes = ::MoveTemp(Other.AllNodes);
	}
	void CopyFrom(const TVoxelFlatOctree<T>& Other)
	{
		ensure(ChunkSize == Other.ChunkSize);
		AllNodes = Other.AllNodes;
	}

	FORCEINLINE int32 NumNodes() const
	{
		// -7: root node only has 1 node
		return AllNodes.Num() * 8 - 7;
	}
	FORCEINLINE int64 GetAllocatedSize() const
	{
		return AllNodes.GetAllocatedSize();
	}

	// Reference will be invalidated if you call CreateChildren!
	FORCEINLINE FNodeData& GetNodeData(FNode Node)
	{
		return GetNode(Node).Data;
	}
	FORCEINLINE const FNodeData& GetNodeData(FNode Node) const
	{
		return GetNode(Node).Data;
	}
	
	FORCEINLINE int32 GetHeight(FNode Node) const
	{
		return GetNode(Node).Height;
	}
	FORCEINLINE bool HasChildren(FNode Node) const
	{
		return GetNode(Node).Children != -1;
	}
	
	FORCEINLINE int32 GetNodeSize(FNode Node) const
	{
		return ChunkSize << GetHeight(Node);
	}
	FORCEINLINE FVoxelIntBox GetNodeBounds(FNode Node) const
	{
#if VOXEL_FLAT_OCTREE_CACHE_BOUNDS
		return GetNode(Node).CachedBounds;
#else
		return GetNodeBoundsImpl(GetNode(Node));
#endif
	}
	FORCEINLINE FIntVector GetNodeCenter(FNode Node) const
	{
		return GetNode(Node).Center;
	}
	
	void CreateChildren(FNode Node)
	{
		// Allocate first, so the NodeStorage reference doesn't get invalidated
		const int32 NewChildren = AllNodes.Add({});
		
		FNodeStorage& NodeStorage = GetNode(Node);
		
		checkVoxelSlow(NodeStorage.Children == -1);
		checkVoxelSlow(NodeStorage.Height > 0);

		NodeStorage.Children = NewChildren;

		const int32 ChildrenOffset = GetNodeSize(Node) / 4;
		for (int32 ChildIndex = 0; ChildIndex < 8; ChildIndex++)
		{
			FNodeStorage& ChildStorage = GetNode(FNode(NewChildren, ChildIndex, this));
			ChildStorage.Center = NodeStorage.Center + ChildrenOffset * FIntVector(
				bool(ChildIndex & 0x1) ? 1 : -1,
				bool(ChildIndex & 0x2) ? 1 : -1,
				bool(ChildIndex & 0x4) ? 1 : -1);
			ChildStorage.Height = NodeStorage.Height - 1;
#if VOXEL_FLAT_OCTREE_CACHE_BOUNDS
			ChildStorage.CachedBounds = GetNodeBoundsImpl(ChildStorage);
#endif
		}
	}
	void DestroyChildren(FNode Node)
	{
		const int32 Children = GetNode(Node).Children;
		checkVoxelSlow(Children != -1);
		
		for (int32 ChildIndex = 0; ChildIndex < 8; ChildIndex++)
		{
			const FNode Child(Children, ChildIndex, this);
			if (HasChildren(Child))
			{
				DestroyChildren(Child);
			}
		}
		
		AllNodes.RemoveAt(Children);
		GetNode(Node).Children = -1;
	}
	
	FORCEINLINE FNode GetChild(FNode Node, const FIntVector& Position) const
	{
		checkVoxelSlow(this->HasChildren(Node));
		const FNodeStorage& NodeStorage = GetNode(Node);
		const int32 ChildIndex = (Position.X >= NodeStorage.Center.X) + 2 * (Position.Y >= NodeStorage.Center.Y) + 4 * (Position.Z >= NodeStorage.Center.Z);
		return FNode(NodeStorage.Children, ChildIndex, this);
	}

	// Note: we do not allow iterating on NodeData directly, as that could lead to invalid references
	template<typename TLambda>
	void Traverse(FNode Node, TLambda Lambda) const
	{
		if (!CallLambda(Lambda, Node))
		{
			return;
		}

		const int32 Children = GetNode(Node).Children;
		if (Children == -1)
		{
			return;
		}
		
		for (int32 ChildIndex = 0; ChildIndex < 8; ChildIndex++)
		{
			Traverse(FNode(Children, ChildIndex, this), Lambda);
		}
	}
	template<typename TLambda>
	void Traverse(TLambda Lambda) const
	{
		Traverse(FNode::Root(), Lambda);
	}
	template<typename TLambda>
	void TraverseChildren(FNode Node, TLambda Lambda) const
	{
		Traverse(Node, [&](FNode ChildNode)
		{
			if (ChildNode != Node)
			{
				return CallLambda(Lambda, ChildNode);
			}
			else
			{
				return true;
			}
		});
	}
	template<typename TLambda>
	void TraverseBounds(const FVoxelIntBox& Bounds, TLambda Lambda) const
	{
		Traverse([&](FNode Node)
		{
			if (!GetNodeBounds(Node).Intersect(Bounds))
			{
				return false;
			}
			
			return CallLambda(Lambda, Node);
		});
	}
	
	template<typename TLambda>
	void ForAllNodes(TLambda Lambda) const
	{
		Lambda(FNode::Root());

		for (auto It = AllNodes.CreateConstIterator(); It; ++It)
		{
			if (It.GetIndex() != 0)
			{
				// Ignore root
				for (int32 ChildIndex = 0; ChildIndex < 8; ChildIndex++)
				{
					Lambda(FNode(It.GetIndex(), ChildIndex, this));
				}
			}
		}
	}
	
	template<typename TLambda>
	void ForAllNodeDatas(TLambda Lambda)
	{
		ForAllNodes([&](FNode Node)
		{
			Lambda(GetNodeData(Node));
		});
	}
	template<typename TLambda>
	void ForAllNodeDatas(TLambda Lambda) const
	{
		ForAllNodes([&](FNode Node)
		{
			Lambda(GetNodeData(Node));
		});
	}

	FNode CreateLeaf(const FIntVector& Position)
	{
		if (!ensure(GetNodeBounds(FNode::Root()).Contains(Position)))
		{
			return {};
		}

		FNode Node = FNode::Root();
		while (GetHeight(Node) > 0)
		{
			if (!HasChildren(Node))
			{
				CreateChildren(Node);
			}
			Node = GetChild(Node, Position);
		}
		ensure(GetHeight(Node) == 0);

		return Node;
	}

private:
	struct FNodeStorage
	{
		FNodeData Data;
		// GetChild is much faster if we store the center instead of the lower corner
		FIntVector Center{ ForceInit };
		int32 Height = 0;
		int32 Children = -1;
#if VOXEL_FLAT_OCTREE_CACHE_BOUNDS
		FVoxelIntBox CachedBounds;
#endif
	};
	
	const int32 ChunkSize;
	// First element is the root + 7 dummies
	// This avoids branches when querying a node from its id
	TVoxelSparseArray<TStaticArray<FNodeStorage, 8>> AllNodes;
	
	FORCEINLINE FNodeStorage& GetNode(FNode Node)
	{
		checkVoxelSlow(Node.IsValid());

		// Much faster, compiler is not smart enough to optimize it to that
		FNodeStorage* RESTRICT Data = reinterpret_cast<FNodeStorage*>(&AllNodes[0]);
		FNodeStorage& Result = Data[Node.Index];
		
#if VOXEL_DEBUG
		const int32 GroupIndex = Node.Index / 8;
		const int32 ChildIndex = Node.Index % 8;
		checkVoxelSlow(&Result == &AllNodes[GroupIndex][ChildIndex]);
#endif

		return Result;
	}
	FORCEINLINE const FNodeStorage& GetNode(FNode Node) const
	{
		return VOXEL_CONST_CAST(*this).GetNode(Node);
	}
	
	FORCEINLINE FVoxelIntBox GetNodeBoundsImpl(const FNodeStorage& NodeStorage) const
	{
		const int32 NodeSize = ChunkSize << NodeStorage.Height;
		return FVoxelIntBox(NodeStorage.Center - NodeSize / 2, NodeStorage.Center + NodeSize / 2);
	}

	template<typename TLambda>
	FORCEINLINE static typename TEnableIf<TIsSame<decltype(DeclVal<TLambda>()(FNode())), bool>::Value, bool>::Type CallLambda(const TLambda& Lambda, FNode Node)
	{
		return Lambda(Node);
	}
	template<typename TLambda>
	FORCEINLINE static typename TEnableIf<TIsSame<decltype(DeclVal<TLambda>()(FNode())), void>::Value, bool>::Type CallLambda(const TLambda& Lambda, FNode Node)
	{
		Lambda(Node);
		return true;
	}
};