// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBlockTypes.h"

class UVoxelBlockAsset;

struct VOXELBLOCK_API FVoxelBlockUtilities
{
public:
	static bool BuildTextures(
		const TMap<FVoxelBlockId, TObjectPtr<UVoxelBlockAsset>>& Assets,
		TMap<FVoxelBlockId, int32>& OutScalarIds,
		TMap<FVoxelBlockId, FVoxelBlockFaceIds>& OutFacesIds,
		TMap<FName, TObjectPtr<UTexture2D>>& InOutTextures,
		TMap<FName, TObjectPtr<UTexture2DArray>>& InOutTextureArrays);

	static void SetupInstance(
		UMaterialInstanceDynamic* Instance,
		const TMap<FName, TObjectPtr<UTexture2D>>& Textures,
		const TMap<FName, TObjectPtr<UTexture2DArray>>& TextureArrays)
	{
		GetInstanceParameters(Textures, TextureArrays).ApplyTo(Instance);
	}

	static FVoxelMaterialParameters GetInstanceParameters(
		const TMap<FName, TObjectPtr<UTexture2D>>& Textures,
		const TMap<FName, TObjectPtr<UTexture2DArray>>& TextureArrays);

private:
	static constexpr const TCHAR* MaterialParamPrefix = TEXT("MaterialParam_");
	
	struct FScalarSet
	{
		TArray<float> Scalars;

		friend uint32 GetTypeHash(const FScalarSet& FloatSet)
		{
			return FCrc::MemCrc32(FloatSet.Scalars.GetData(), FloatSet.Scalars.Num() * FloatSet.Scalars.GetTypeSize());
		}
		friend bool operator==(const FScalarSet& A, const FScalarSet& B)
		{
			return A.Scalars == B.Scalars;
		}
	};
	struct FTextureSet
	{
		TArray<UTexture2D*> Textures;

		friend uint32 GetTypeHash(const FTextureSet& TextureSet)
		{
			return FCrc::MemCrc32(TextureSet.Textures.GetData(), TextureSet.Textures.Num() * TextureSet.Textures.GetTypeSize());
		}
		friend bool operator==(const FTextureSet& A, const FTextureSet& B)
		{
			return A.Textures == B.Textures;
		}
	};

private:
	static void FindScalars(
		const TArray<FName>& ScalarNames,
		const TMap<FVoxelBlockId, TObjectPtr<UVoxelBlockAsset>>& Assets,
		TMap<FVoxelBlockId, int32>& OutScalarIds,
		TArray<FScalarSet>& OutScalarSets);

	static void FindTextures(
		int32 TextureSize,
		const TArray<FName>& TextureNames,
		const TMap<FVoxelBlockId, TObjectPtr<UVoxelBlockAsset>>& Assets,
		TMap<FVoxelBlockId, FVoxelBlockFaceIds>& OutFacesIds,
		TArray<FTextureSet>& OutTextureSets);

private:
	static void BuildScalarTextures(
		const TArray<FName>& ScalarNames,
		const TArray<FScalarSet>& ScalarSets,
		TMap<FName, TObjectPtr<UTexture2D>>& InOutTextures);

	static void BuildTextureArrays(
		int32 TextureSize,
		const TArray<FName>& TextureNames,
		const TArray<FTextureSet>& TextureSets,
		TMap<FName, TObjectPtr<UTexture2DArray>>& InOutTextureArrays);
};