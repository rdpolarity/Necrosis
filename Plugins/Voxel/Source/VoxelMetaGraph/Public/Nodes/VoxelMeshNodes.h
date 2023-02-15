// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelExecNode.h"
#include "VoxelMesh/VoxelMeshRenderer.h"
#include "VoxelMeshNodes.generated.h"

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelVertexData : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	TVoxelArray<FVector3f> Normals;
	TVoxelArray<FColor> Colors;
	TVoxelArray<TVoxelArray<FVector2f>> AllTextureCoordinates;

	virtual int64 GetAllocatedSize() const override
	{
		int64 AllocatedSize = sizeof(*this);
		AllocatedSize += Normals.GetAllocatedSize();
		AllocatedSize += Colors.GetAllocatedSize();
		AllocatedSize += AllTextureCoordinates.GetAllocatedSize();
		for (const TVoxelArray<FVector2f>& TextureCoordinates : AllTextureCoordinates)
		{
			AllocatedSize += TextureCoordinates.GetAllocatedSize();
		}
		return AllocatedSize;
	}
};

USTRUCT(Category = "Mesh")
struct VOXELMETAGRAPH_API FVoxelNode_MakeVertexData : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelVectorBuffer, Normal, FVector::UpVector);
	VOXEL_INPUT_PIN(FVoxelLinearColorBuffer, Color, FLinearColor::Black);
	VOXEL_INPUT_PIN_ARRAY(FVoxelVector2DBuffer, TextureCoordinate, FVector2D::ZeroVector, 1);

	VOXEL_OUTPUT_PIN(FVoxelVertexData, VertexData);
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelChunkExecObject_CreateMeshComponent : public FVoxelChunkExecObject
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	FVector3d Position = FVector3d::ZeroVector;
	TSharedPtr<const FVoxelMesh> Mesh;

	virtual void Create(FVoxelRuntime& Runtime) const override;
	virtual void Destroy(FVoxelRuntime& Runtime)const  override;

private:
	mutable FVoxelMeshRendererId MeshId;
};

USTRUCT(Category = "Mesh")
struct VOXELMETAGRAPH_API FVoxelChunkExecNode_CreateMeshComponent : public FVoxelChunkExecNode_Default
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelMesh, Mesh, nullptr);

	virtual TValue<FVoxelChunkExecObject> Execute(const FVoxelQuery& Query) const override;
};