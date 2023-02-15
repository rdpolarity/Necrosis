// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBlockAsset.h"
#include "VoxelBlockRegistry.h"
#include "Nodes/VoxelMeshNodes.h"
#include "Nodes/VoxelSurfaceNodes.h"
#include "Nodes/VoxelMeshMaterialNodes.h"
#include "VoxelNavmesh/VoxelNavmesh.h"
#include "VoxelCollision/VoxelCollider.h"
#include "VoxelBlockNodes.generated.h"

struct FVoxelBlockMesh;

USTRUCT()
struct VOXELBLOCK_API FVoxelBlockDataPinType : public FVoxelExposedPinType
{
	GENERATED_BODY()

	DEFINE_VOXEL_EXPOSED_PIN_TYPE(FVoxelBlockData, TSoftObjectPtr<UVoxelBlockAsset>)
	{
		const FVoxelBlockRegistry& BlockRegistry = GetSubsystem<FVoxelBlockRegistry>();

		FVoxelBlockId BlockId;
		if (const UVoxelBlockAsset* Asset = Value.LoadSynchronous())
		{
			BlockId = BlockRegistry.GetBlockId(Asset);
		}
		return MakeSharedCopy(BlockRegistry.GetBlockData(BlockId));
	}
};

DECLARE_VOXEL_TERMINAL_BUFFER(FVoxelBlockDataBufferView, FVoxelBlockDataBuffer, FVoxelBlockData, PF_R32_UINT);

USTRUCT()
struct VOXELBLOCK_API FVoxelBlockDataBufferView : public FVoxelTerminalBufferView
{
	GENERATED_BODY()
	GENERATED_VOXEL_TERMINAL_BUFFER_VIEW_BODY(FVoxelBlockData);
};

USTRUCT(DisplayName = "Block Buffer")
struct VOXELBLOCK_API FVoxelBlockDataBuffer : public FVoxelTerminalBuffer
{
	GENERATED_BODY()
	GENERATED_VOXEL_TERMINAL_BUFFER_BODY(FVoxelBlockData);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct VOXELBLOCK_API FVoxelBlockQueryData : public FVoxelQueryData
{
	GENERATED_BODY()
	GENERATED_VOXEL_QUERY_DATA_BODY()
		
public:
	virtual FVoxelIntVectorBuffer GetBlockPositions() const VOXEL_PURE_VIRTUAL({});

private:
	uint64 GetHash() const
	{
		return 0;
	}
	bool Identical(const FVoxelBlockQueryData& Other) const
	{
		return true;
	}
};

USTRUCT()
struct VOXELBLOCK_API FVoxelBlockSparseQueryData : public FVoxelBlockQueryData
{
	GENERATED_BODY()
	GENERATED_VOXEL_QUERY_DATA_BODY()
		
public:
	virtual FVoxelIntVectorBuffer GetBlockPositions() const override
	{
		return PrivatePositions;
	}

	void Initialize(TConstVoxelArrayView<FIntVector> Positions);
	void Initialize(const FVoxelIntVectorBuffer& Positions);

private:
	FVoxelIntVectorBuffer PrivatePositions;
	
	uint64 GetHash() const
	{
		return PrivatePositions.GetHash();
	}
	bool Identical(const FVoxelBlockSparseQueryData& Other) const
	{
		return PrivatePositions.Identical(Other.PrivatePositions);
	}
};

USTRUCT()
struct VOXELBLOCK_API FVoxelBlockChunkQueryData : public FVoxelBlockQueryData
{
	GENERATED_BODY()
	GENERATED_VOXEL_QUERY_DATA_BODY()

public:
	FORCEINLINE const FVoxelIntBox& Bounds() const
	{
		return PrivateBounds;
	}
	virtual FVoxelIntVectorBuffer GetBlockPositions() const override
	{
		return CachedPositions;
	}

	void Initialize(const FVoxelIntBox& Bounds);

private:
	FVoxelIntBox PrivateBounds;
	FVoxelIntVectorBuffer CachedPositions;
	
	uint64 GetHash() const
	{
		return FVoxelUtilities::MurmurHash(PrivateBounds);
	}
	bool Identical(const FVoxelBlockChunkQueryData& Other) const
	{
		return PrivateBounds == Other.PrivateBounds;
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct VOXELBLOCK_API FVoxelBlockSurface : public FVoxelSurface
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	float BlockSize = 0.f;

	struct FFaceMesh
	{
		TVoxelArray<FIntVector> Positions;
		TVoxelArray<FVoxelBlockData> BlockDatas;
	};
	TVoxelStaticArray<FFaceMesh, 6> FaceMeshes;

	int32 GetNumFaces() const;

	FORCEINLINE FVector3f GetVertexPosition(int32 Direction, int32 FaceIndex, const FVector3f& CornerPosition) const
	{
		const FVector3f VertexPosition = CornerPosition + FVector3f(FaceMeshes[Direction].Positions[FaceIndex]) - FVector3f(0.5f);
		return VertexPosition * BlockSize;
	}

	static void GetFaceData(
		EVoxelBlockFace Face,
		TVoxelStaticArray<FVector3f, 4>& OutPositions,
		FVector3f& OutNormal,
		FVector3f& OutTangent);
};

USTRUCT(Category = "Mesh|Block")
struct VOXELBLOCK_API FVoxelNode_FVoxelBlockSurface_GenerateSurface : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelBlockDataBuffer, Block, nullptr);
	VOXEL_INPUT_PIN(float, BlockSize, 100.f);
	VOXEL_OUTPUT_PIN(FVoxelBlockSurface, Surface);
};

USTRUCT(Category = "Mesh|Block")
struct VOXELBLOCK_API FVoxelNode_FVoxelBlockSurface_CreateCollider : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelBlockSurface, Surface, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelCollider, Collider);
};

USTRUCT(Category = "Mesh|Block")
struct VOXELBLOCK_API FVoxelNode_FVoxelBlockSurface_CreateNavmesh: public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelBlockSurface, Surface, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelNavmesh, Navmesh);
};

USTRUCT(Category = "Mesh|Block")
struct VOXELBLOCK_API FVoxelNode_FVoxelBlockSurface_CreateMesh : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelBlockSurface, Surface, nullptr);
	VOXEL_INPUT_PIN(FVoxelMeshMaterial, Material, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelMesh, Mesh);
};

USTRUCT(Category = "Mesh|Block")
struct VOXELBLOCK_API FVoxelNode_GetBlockPosition : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_OUTPUT_PIN(FVoxelIntVectorBuffer, BlockPosition);
};