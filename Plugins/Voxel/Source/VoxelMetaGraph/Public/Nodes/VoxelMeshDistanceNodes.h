// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelNodeTypes.h"
#include "VoxelMeshVoxelizer.h"
#include "VoxelMeshDistanceNodes.generated.h"

class VOXELMETAGRAPH_API FVoxelMeshDistanceData
{
public:
	const TVoxelArray<FVector3f> Vertices;
	const TVoxelArray<int32> Indices;

	// TODO Octree storing the triangles, with children using relative indices to be able to use uint16/uint8?
	// And for leaves use bit arrays into the parent triangles with a single uint32 for optimal memory?

	FIntVector Size = FIntVector::ZeroValue;
	FBox3f Bounds = FBox3f(ForceInit);
	FVector3f CellSize = FVector3f::ZeroVector;
	TVoxelArray<int32> Triangles;
	TVoxelArray<int32> Cells;

	FVoxelMeshDistanceData(
		TVoxelArray<FVector3f>&& Vertices,
		TVoxelArray<int32>&& Indices)
		: Vertices(MoveTemp(Vertices))
		, Indices(MoveTemp(Indices))
	{
		Build();
	}

	void Build();
};

USTRUCT(DisplayName = "Static Mesh")
struct VOXELMETAGRAPH_API FVoxelStaticMeshDistanceData
{
	GENERATED_BODY()

	TSharedPtr<const FVoxelMeshDistanceData> Data;
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelStaticMeshDistanceDataPinType : public FVoxelExposedPinType
{
	GENERATED_BODY()

	DEFINE_VOXEL_EXPOSED_PIN_TYPE(FVoxelStaticMeshDistanceData, TSoftObjectPtr<UStaticMesh>)
	{
		const UStaticMesh* StaticMesh = Value.LoadSynchronous();
		if (!StaticMesh)
		{
			return {};
		}

		FVoxelMeshVoxelizerInputData MeshData = UVoxelMeshVoxelizerLibrary::CreateMeshDataFromStaticMesh(*StaticMesh);

		const TSharedRef<FVoxelStaticMeshDistanceData> DistanceData = MakeShared<FVoxelStaticMeshDistanceData>();
		DistanceData->Data = MakeShared<FVoxelMeshDistanceData>(MoveTemp(MeshData.Vertices), MoveTemp(MeshData.Indices));
		return DistanceData;
	}
};

USTRUCT(Category = "Mesh")
struct VOXELMETAGRAPH_API FVoxelNode_MeshDistance : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelStaticMeshDistanceData, Mesh, nullptr);
	VOXEL_INPUT_PIN(FVoxelVectorBuffer, Position, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Distance);
};