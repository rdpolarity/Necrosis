// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelDensityCanvasData.generated.h"

class IVoxelNodeOuter;
class FVoxelDependency;
struct FVoxelNode_ApplyDensityCanvas;

BEGIN_VOXEL_NAMESPACE(DensityCanvas)

constexpr int32 ChunkSize = 16;
constexpr int32 ChunkSizeLog2 = FVoxelUtilities::ExactLog2<ChunkSize>();
constexpr int32 MaxDistance = 128;
constexpr int32 ChunkCount = FMath::Cube(ChunkSize);

using FDensity = int16;
using FChunk = TVoxelStaticArray<FDensity, ChunkCount>;

FORCEINLINE FDensity ToDensity(const float Value)
{
	constexpr int32 Max = TNumericLimits<FDensity>::Max();
	return FMath::Clamp(FMath::RoundToInt(Max * Value / MaxDistance), -Max, Max);
}
FORCEINLINE float FromDensity(const FDensity Value)
{
	constexpr int32 Max = TNumericLimits<FDensity>::Max();
	return Value / float(Max) * MaxDistance;
}

class VOXELCANVAS_API FData : public TSharedFromThis<FData>
{
public:
	float VoxelSize = 100.f;
	mutable FVoxelSharedCriticalSection CriticalSection;

	FData() = default;

	FORCEINLINE FChunk* FindChunk(const FIntVector& Key)
	{
		checkVoxelSlow(CriticalSection.IsLocked_Read_Debug());
		const TSharedPtr<FChunk>* Chunk = Chunks.Find(Key);
		if (!Chunk)
		{
			return nullptr;
		}
		return Chunk->Get();
	}
	FORCEINLINE const FChunk* FindChunk(const FIntVector& Key) const
	{
		return VOXEL_CONST_CAST(this)->FindChunk(Key);
	}

	void UseNode(const FVoxelNode_ApplyDensityCanvas* InNode) const;
	void AddDependency(const FVoxelBox& Bounds, const TSharedRef<FVoxelDependency>& Dependency) const;
	bool HasChunks(const FVoxelBox& Bounds) const;

	void ClearData();
	void SphereEdit(
		const FVector3f& Center,
		float Radius,
		float Falloff,
		float Strength,
		bool bSmooth,
		bool bAdd);
	void Serialize(FArchive& Ar);

private:
	struct FNodeData
	{
		bool bHasChunks = false;
	};
	struct FOctree : TVoxelFlatOctree<FNodeData>
	{
		FOctree()
			: TVoxelFlatOctree<FNodeData>(DensityCanvas::ChunkSize, 20)
		{
		}
	};

	TSharedRef<FOctree> Octree = MakeShared<FOctree>();
	TVoxelIntVectorMap<TSharedPtr<FChunk>> Chunks;
	bool bEditQueued = false;
	
	struct FDependencyRef
	{
		FVoxelIntBox Bounds;
		TWeakPtr<FVoxelDependency> Dependency;
	};
	mutable FVoxelCriticalSection DependenciesCriticalSection;
	mutable TVoxelArray<FDependencyRef> Dependencies;

	mutable const FVoxelNode_ApplyDensityCanvas* CanvasNode = nullptr;
	mutable TWeakPtr<IVoxelNodeOuter> WeakOuter;
};

END_VOXEL_NAMESPACE(DensityCanvas)

USTRUCT(DisplayName = "Density Canvas")
struct VOXELCANVAS_API FVoxelDensityCanvasData
{
	GENERATED_BODY()
	VOXEL_USE_NAMESPACE_TYPES(DensityCanvas, FData);

	TSharedPtr<FData> Data;
};