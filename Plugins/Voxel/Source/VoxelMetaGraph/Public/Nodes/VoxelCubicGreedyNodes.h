// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#if 0 // TODO
#include "VoxelMinimal.h"
#include "VoxelTextureAtlas.h"
#include "VoxelMesh/VoxelMeshRenderer.h"
#include "Nodes/VoxelMeshMaterialNodes.h"
#include "VoxelCubicGreedyNodes.generated.h"

struct FVoxelCubicGreedyMesh;
class FVoxelTextureAtlasTextureData;

struct VOXELMETAGRAPH_API FVoxelPackedQuad
{
	static constexpr int32 ChunkSize = 32;
	static constexpr int32 NumBitsPerPosition = FVoxelUtilities::CeilLog2<ChunkSize>();
	
	uint32 Direction : 3;
	uint32 Layer : NumBitsPerPosition;
	uint32 StartX : NumBitsPerPosition;
	uint32 StartY : NumBitsPerPosition;
	uint32 SizeXMinus1 : NumBitsPerPosition;
	uint32 SizeYMinus1 : NumBitsPerPosition;

	FORCEINLINE int32 SizeX() const
	{
		return SizeXMinus1 + 1;
	}
	FORCEINLINE int32 SizeY() const
	{
		return SizeYMinus1 + 1;
	}

	FORCEINLINE FIntVector GetAxes() const
	{
		const int32 ZAxis = Direction / 2;
		return FIntVector((ZAxis + 1) % 3, (ZAxis + 2) % 3, ZAxis);
	}
	FORCEINLINE bool IsInverted() const
	{
		return Direction & 0x1;
	}
	FORCEINLINE FIntVector GetPosition(bool bOffsetX, bool bOffsetY, bool bOffsetZ) const
	{
		const FIntVector Axes = GetAxes();

		FIntVector Position;
		Position[Axes.X] = StartX + SizeX() * bOffsetX;
		Position[Axes.Y] = StartY + SizeY() * bOffsetY;
		Position[Axes.Z] = Layer + bOffsetZ;
		return Position;
	}
};
static_assert(sizeof(FVoxelPackedQuad) == sizeof(uint32), "");

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelCubicGreedyMeshData : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	struct FChunk
	{
		FIntVector Offset;
		TVoxelArray<FVoxelPackedQuad> Quads;
	};
	struct FInnerMeshData
	{
		TVoxelArray<FChunk> Chunks;
		TVoxelArray<FVector3f> PositionsToQuery;
	};

	float ScaledVoxelSize = 0.f;
	FIntVector Offset = FIntVector::ZeroValue;
	int32 NumQuads = 0;
	int32 NumPixels = 0;
	TSharedPtr<const FInnerMeshData> InnerMeshData;
	TMap<FName, TSharedPtr<const FVoxelTextureAtlasTextureData>> TextureDatas;

	virtual int64 GetAllocatedSize() const override
	{
		int64 AllocatedSize = sizeof(*this);
		if (InnerMeshData)
		{
			for (const FChunk& Chunk : InnerMeshData->Chunks)
			{
				AllocatedSize += Chunk.Quads.GetAllocatedSize();
			}
			AllocatedSize += InnerMeshData->PositionsToQuery.GetAllocatedSize();
		}
		for (auto& It : TextureDatas)
		{
			AllocatedSize += It.Value->GetAllocatedSize();
		}
		return AllocatedSize;
	}
};

USTRUCT(Category = "Mesh|CubicGreedy")
struct VOXELMETAGRAPH_API FVoxelNode_MakeCubicGreedyMeshData : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_INPUT_PIN(FVoxelBoolBuffer, Values, nullptr);
	VOXEL_INPUT_PIN(float, VoxelSize, 100.f);
	VOXEL_OUTPUT_PIN(FVoxelCubicGreedyMeshData, MeshData);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// To query the color in your material: use Voxel_GetCubicGreedyColor with a texture named VoxelTextureAtlas_<Name>Texture
USTRUCT(Category = "Mesh|CubicGreedy")
struct VOXELMETAGRAPH_API FVoxelNode_CubicGreedy_AddColorTexture : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_INPUT_PIN(FVoxelCubicGreedyMeshData, MeshData, nullptr);
	VOXEL_INPUT_PIN(FVoxelLinearColorBuffer, Colors, nullptr);
	VOXEL_INPUT_PIN(FName, Name, "Color");
	VOXEL_OUTPUT_PIN(FVoxelCubicGreedyMeshData, MeshData);
};

// To query the index in your material: use Voxel_GetCubicGreedyIndex with a texture named VoxelTextureAtlas_<Name>Texture
// @param	FullPrecision	If true the data will be passed as int32, otherwise as uint8
USTRUCT(Category = "Mesh|CubicGreedy")
struct VOXELMETAGRAPH_API FVoxelNode_CubicGreedy_AddIndexTexture : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_INPUT_PIN(FVoxelCubicGreedyMeshData, MeshData, nullptr);
	VOXEL_INPUT_PIN(FVoxelInt32Buffer, Indices, nullptr);
	VOXEL_INPUT_PIN(FName, Name, "Index");
	VOXEL_INPUT_PIN(bool, FullPrecision, true);
	VOXEL_OUTPUT_PIN(FVoxelCubicGreedyMeshData, MeshData);
};

// To query the index in your material: use Voxel_GetCubicGreedyScalar with a texture named VoxelTextureAtlas_<Name>Texture
USTRUCT(Category = "Mesh|CubicGreedy")
struct VOXELMETAGRAPH_API FVoxelNode_CubicGreedy_AddScalarTexture : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_INPUT_PIN(FVoxelCubicGreedyMeshData, MeshData, nullptr);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Scalars, nullptr);
	VOXEL_INPUT_PIN(FName, Name, "Scalar");
	VOXEL_OUTPUT_PIN(FVoxelCubicGreedyMeshData, MeshData);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelCubicGreedyMeshResult : public FVoxelChunkResult
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	FVector3d Position = FVector3d::ZeroVector;
	TArray<TSharedPtr<FVoxelCubicGreedyMesh>> Meshes;

	virtual void Create(FVoxelRuntime& Runtime) const override;
	virtual void Destroy(FVoxelRuntime& Runtime) const override;

private:
	mutable FVoxelMeshRendererId MeshId;
};

USTRUCT(Category = "Mesh|CubicGreedy")
struct VOXELMETAGRAPH_API FVoxelNode_RenderCubicGreedyMesh : public FVoxelNode_ChunkResultBase
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_NODE_SUBSYSTEM_REF(FVoxelMeshRenderer, Renderer);

	VOXEL_INPUT_PIN(FVoxelCubicGreedyMeshData, MeshData, nullptr);
	VOXEL_INPUT_PIN(FVoxelMeshMaterial, Material, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelChunkResult, Result);
};
#endif