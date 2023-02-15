// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "Nodes/VoxelMeshMaterialNodes.h"
#include "VoxelDetailTextureNodes.generated.h"

struct FVoxelMarchingCubeSurface;

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelDetailTextureQueryData : public FVoxelQueryData
{
	GENERATED_BODY()
	GENERATED_VOXEL_QUERY_DATA_BODY()

public:
	int32 NumCells = 0;
	int32 TextureSize = 0;
	int32 NumCellsPerSide = 0;
	int32 AtlasTextureSize = 0;
	TVoxelBuffer<FVector> Normals;

private:
	uint64 GetHash() const
	{
		return FVoxelUtilities::MurmurHashMulti(
			NumCells,
			TextureSize,
			NumCellsPerSide,
			AtlasTextureSize,
			Normals.GetHash());
	}
	bool Identical(const FVoxelDetailTextureQueryData& Other) const
	{
		return
			NumCells == Other.NumCells &&
			TextureSize == Other.TextureSize &&
			NumCellsPerSide == Other.NumCellsPerSide &&
			AtlasTextureSize == Other.AtlasTextureSize &&
			Normals.X.GetDataPtr() == Other.Normals.X.GetDataPtr() &&
			Normals.Y.GetDataPtr() == Other.Normals.Y.GetDataPtr() &&
			Normals.Z.GetDataPtr() == Other.Normals.Z.GetDataPtr();
	}
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelDetailTexture
{
	GENERATED_BODY()

	FName Name;
	int32 SizeX = 0;
	int32 SizeY = 0;
	EPixelFormat Format = PF_Unknown;
	TSharedPtr<const TVoxelArray<uint8>> CpuData;
	TSharedPtr<const TRefCountPtr<IPooledRenderTarget>> GpuTexture;

	template<typename LambdaType>
	void SetCpuData(const FVoxelDetailTextureQueryData& QueryData, LambdaType&& Apply)
	{
		VOXEL_FUNCTION_COUNTER();
		ensure(Format != PF_Unknown);

		const int32 NumCells = QueryData.NumCells;
		const int32 TextureSize = QueryData.TextureSize;
		const int32 NumCellsPerSide = QueryData.NumCellsPerSide;
		const int32 AtlasTextureSize = QueryData.AtlasTextureSize;

		const TSharedRef<TVoxelArray<uint8>> Data = MakeShared<TVoxelArray<uint8>>();
		FVoxelUtilities::SetNumFast(*Data, GPixelFormats[Format].BlockBytes * AtlasTextureSize * AtlasTextureSize);
		FVoxelUtilities::Memzero(*Data);

		for (int32 CellIndex = 0; CellIndex < NumCells; CellIndex++)
		{
			const int32 CellX = TextureSize * (CellIndex % NumCellsPerSide);
			const int32 CellY = TextureSize * (CellIndex / NumCellsPerSide);
			const int32 BaseQueryIndex = CellIndex * TextureSize * TextureSize;

			int32 TextureIndex = 0;
			for (int32 Y = 0; Y < TextureSize; Y++)
			{
				for (int32 X = 0; X < TextureSize; X++)
				{
					checkVoxelSlow(TextureIndex == FVoxelUtilities::Get2DIndex<int32>(TextureSize, X, Y));
					const int32 DataIndex = FVoxelUtilities::Get2DIndex<int32>(AtlasTextureSize, CellX + X, CellY + Y);
					Apply(*Data, BaseQueryIndex + TextureIndex, DataIndex, CellIndex);
					TextureIndex++;
				}
			}
		}

		ensure(!CpuData);
		ensure(!GpuTexture);
		CpuData = Data;
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(Category = "Detail Textures")
struct VOXELMETAGRAPH_API FVoxelNode_MakeNormalDetailTexture : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelVectorBuffer, Normal, nullptr);
	VOXEL_INPUT_PIN(FName, Name, "Normal");
	VOXEL_INPUT_PIN(float, MaxNormalDifference, 0.5f);
	VOXEL_OUTPUT_PIN(FVoxelDetailTexture, DetailTexture);
};

USTRUCT(Category = "Detail Textures")
struct VOXELMETAGRAPH_API FVoxelNode_MakeColorDetailTexture : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelLinearColorBuffer, Color, nullptr);
	VOXEL_INPUT_PIN(FName, Name, "Color");
	VOXEL_OUTPUT_PIN(FVoxelDetailTexture, DetailTexture);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelNode_CreateDetailTextureMaterial_Internal_Input
{
	GENERATED_BODY()

	TSharedPtr<const FVoxelMeshMaterial> Material;
	TArray<TSharedRef<const FVoxelDetailTexture>> DetailTextures;
};

USTRUCT(meta = (Internal))
struct VOXELMETAGRAPH_API FVoxelNode_CreateDetailTextureMaterial_Internal : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
public:
	VOXEL_INPUT_PIN(int32, TextureSizeBias, 2);
	VOXEL_INPUT_PIN(FVoxelMeshMaterial, FallbackMaterial, nullptr);
	VOXEL_INPUT_PIN(FVoxelNode_CreateDetailTextureMaterial_Internal_Input, Input, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelMeshMaterial, Material);

	int32 GetTextureSize(int32 LOD, int32 TextureSizeBias) const;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(Category = "Detail Textures")
struct VOXELMETAGRAPH_API FVoxelNode_CreateDetailTextureMaterial : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
public:
	VOXEL_INPUT_PIN(int32, TextureSizeBias, 2);
	VOXEL_INPUT_PIN(FVoxelMeshMaterial, BaseMaterial, nullptr);
	VOXEL_INPUT_PIN_ARRAY(FVoxelDetailTexture, DetailTextures, nullptr, 1);
	VOXEL_OUTPUT_PIN(FVoxelMeshMaterial, Material);
};