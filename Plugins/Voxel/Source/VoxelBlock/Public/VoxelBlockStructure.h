// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBlockTypes.h"
#include "VoxelBlockChunk.h"
#include "VoxelBlockStructure.generated.h"

class FVoxelRuntime;
class UVoxelBlockAsset;

USTRUCT()
struct FVoxelBlockStructure
{
	GENERATED_BODY()

	FVoxelBlockStructure() = default;
	virtual ~FVoxelBlockStructure() = default;

	virtual int32 GetSpawnRadius() const VOXEL_PURE_VIRTUAL({});
	virtual UScriptStruct* GetConfigStruct() const VOXEL_PURE_VIRTUAL({});
};

USTRUCT(BlueprintType)
struct FVoxelBlockStructureConfig
{
	GENERATED_BODY()

	FVoxelBlockStructureConfig() = default;
	virtual ~FVoxelBlockStructureConfig() = default;
};

USTRUCT(BlueprintType)
struct FVoxelBlockReference
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	TObjectPtr<UVoxelBlockAsset> Asset;

public:
	FORCEINLINE operator FVoxelBlockData() const
	{
		return Data;
	}

	void Resolve(const FVoxelRuntime& Runtime);

private:
	FVoxelBlockData Data;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct FVoxelBlockLayer
{
	GENERATED_BODY()

	FVoxelBlockLayer() = default;
	virtual ~FVoxelBlockLayer() = default;
};

USTRUCT()
struct FVoxelBlock2DFloatLayer : public FVoxelBlockLayer
{
	GENERATED_BODY()

	struct FChunk
	{
		TVoxelStaticArray<float, FMath::Square(BlockChunkSize)> Data{ NoInit };
	};

	FIntPoint Start;
	FIntPoint Size = 0;
	int32 Step = 0;

	TArrayView<FChunk*> Chunks;

	FORCEINLINE float Get(int32 X, int32 Y) const
	{
		const FIntPoint ChunkKey = FVoxelUtilities::DivideFloor(FIntPoint(X, Y), BlockChunkSize);

		if (ChunkKey.X < Start.X || Start.X + Size.X <= ChunkKey.X ||
			ChunkKey.Y < Start.Y || Start.Y + Size.Y <= ChunkKey.Y)
		{
			Generate(ChunkKey);
		}

		FChunk* Chunk = Chunks[FVoxelUtilities::Get2DIndex(Size, ChunkKey, Start)];
		checkVoxelSlow(Chunk);
		return Chunk->Data[FVoxelUtilities::Get2DIndex(BlockChunkSize, FIntPoint(X, Y) - ChunkKey * BlockChunkSize)];
	}

	FORCENOINLINE void Generate(const FIntPoint& ChunkKey) const
	{
		
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct FVoxelBlockParentStructure : public FVoxelBlockStructure
{
	GENERATED_BODY()

	virtual void GenerateChildren(TArray<TVoxelInstancedStruct<FVoxelBlockStructure>>& OutChildren) const VOXEL_PURE_VIRTUAL();
};

USTRUCT()
struct FVoxelBlockLeafStructure : public FVoxelBlockStructure
{
	GENERATED_BODY()

	virtual void Generate2(
		int32 NumInstances,
		TConstVoxelArrayView<uint8> InstancesData,
		FVoxelBlockChunkArray& Chunks) const VOXEL_PURE_VIRTUAL();
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UCLASS()
class VOXELBLOCK_API UVoxelBlockStructureAsset : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config", meta = (BaseStruct = FVoxelBlockStructure))
	FVoxelInstancedStruct Structure;
	
	UPROPERTY(EditAnywhere, Category = "Config", meta = (StructTypeConst))
	FVoxelInstancedStruct Config;

public:
	//~ Begin UObject Interface
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface

private:
	void FixupConfig();
};