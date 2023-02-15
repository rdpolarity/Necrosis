// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Nodes/VoxelMeshNodes.h"
#include "Nodes/VoxelSurfaceNodes.h"
#include "Nodes/VoxelPhysicalMaterial.h"
#include "Nodes/VoxelMeshMaterialNodes.h"
#include "VoxelNavmesh/VoxelNavmesh.h"
#include "VoxelCollision/VoxelCollider.h"
#include "VoxelMarchingCubeNodes.generated.h"

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelMarchingCubeSurface : public FVoxelSurface
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	int32 LOD = 0;
	int32 ChunkSize = 0;
	float ScaledVoxelSize = 0.f;
	
	TVoxelBuffer<FVoxelInt4> Cells;
	TVoxelBuffer<int32> Indices;
	TVoxelBuffer<FVector> Vertices;

	virtual int64 GetAllocatedSize() const override
	{
		int64 AllocatedSize = sizeof(*this);
		AllocatedSize += Cells.GetAllocatedSize();
		AllocatedSize += Indices.GetAllocatedSize();
		AllocatedSize += Vertices.GetAllocatedSize();
		return AllocatedSize;
	}
};

USTRUCT(Category = "Mesh|MarchingCube")
struct VOXELMETAGRAPH_API FVoxelNode_FVoxelMarchingCubeSurface_GenerateSurface : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Density, nullptr, DensityPin);
	VOXEL_INPUT_PIN(float, VoxelSize, 100.f);
	VOXEL_INPUT_PIN(bool, EnableDistanceChecks, true);
	VOXEL_INPUT_PIN(float, DistanceChecksTolerance, 1.f);
	VOXEL_OUTPUT_PIN(FVoxelMarchingCubeSurface, Surface);
};

USTRUCT(Category = "Mesh|MarchingCube")
struct VOXELMETAGRAPH_API FVoxelNode_FVoxelMarchingCubeSurface_CreateCollider : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelMarchingCubeSurface, Surface, nullptr);
	VOXEL_INPUT_PIN(FVoxelPhysicalMaterialBuffer, PhysicalMaterial, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelCollider, Collider);
};

USTRUCT(Category = "Mesh|MarchingCube")
struct VOXELMETAGRAPH_API FVoxelNode_FVoxelMarchingCubeSurface_CreateNavmesh : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelMarchingCubeSurface, Surface, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelNavmesh, Navmesh);
};

USTRUCT(Category = "Mesh|MarchingCube")
struct VOXELMETAGRAPH_API FVoxelNode_FVoxelMarchingCubeSurface_CreateMesh : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelMarchingCubeSurface, Surface, nullptr);
	VOXEL_INPUT_PIN(FVoxelVertexData, VertexData, nullptr);
	VOXEL_INPUT_PIN(FVoxelMeshMaterial, Material, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelMesh, Mesh);
};