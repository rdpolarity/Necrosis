// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelMeshNodes.h"
#include "Nodes/VoxelPositionNodes.h"

DEFINE_VOXEL_NODE(FVoxelNode_MakeVertexData, VertexData)
{
	FindVoxelQueryData(FVoxelPositionQueryData, PositionQueryData);

	const TValue<TBufferView<FVector>> Normals = GetBufferView(NormalPin, Query);
	const TValue<TBufferView<FLinearColor>> Colors = GetBufferView(ColorPin, Query);
	const TArray<TValue<TBufferView<FVector2D>>> TextureCoordinates = GetBufferView(TextureCoordinatePins, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, PositionQueryData, Normals, Colors, TextureCoordinates)
	{
		const int32 Num = ComputeVoxelBuffersNum(PositionQueryData->GetPositions(), Normals, Colors);
		
		const TSharedRef<FVoxelVertexData> VertexData = MakeShared<FVoxelVertexData>();
		FVoxelUtilities::SetNumFast(VertexData->Normals, Num);
		FVoxelUtilities::SetNumFast(VertexData->Colors, Num);

		for (int32 Index = 0; Index < Num; Index++)
		{
			VertexData->Normals[Index] = Normals[Index];
			VertexData->Colors[Index] = Colors[Index].ToFColor(false);
		}

		for (const TBufferView<FVector2D>& TextureCoordinate : TextureCoordinates)
		{
			CheckVoxelBuffersNum(PositionQueryData->GetPositions(), TextureCoordinate);

			TVoxelArray<FVector2f>& NewTextureCoordinate = VertexData->AllTextureCoordinates.Emplace_GetRef();
			FVoxelUtilities::SetNumFast(NewTextureCoordinate, Num);

			for (int32 Index = 0; Index < Num; Index++)
			{
				NewTextureCoordinate[Index] = TextureCoordinate[Index];
			}
		}

		return VertexData;
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelChunkExecObject_CreateMeshComponent::Create(FVoxelRuntime& Runtime) const
{
	if (!ensure(Mesh))
	{
		return;
	}

	ensure(!MeshId.IsValid());
	MeshId = Runtime.GetSubsystem<FVoxelMeshRenderer>().CreateMesh(Position, Mesh.ToSharedRef());
}

void FVoxelChunkExecObject_CreateMeshComponent::Destroy(FVoxelRuntime& Runtime) const
{
	ensure(MeshId.IsValid());
	Runtime.GetSubsystem<FVoxelMeshRenderer>().DestroyMesh(MeshId);
	MeshId = {};
}

TVoxelFutureValue<FVoxelChunkExecObject> FVoxelChunkExecNode_CreateMeshComponent::Execute(const FVoxelQuery& Query) const
{
	FindVoxelQueryData(FVoxelBoundsQueryData, BoundsQueryData);
	const TValue<FVoxelMesh> Mesh = GetNodeRuntime().Get(MeshPin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, Mesh)
	{
		if (Mesh->GetStruct() == FVoxelMesh::StaticStruct())
		{
			return {};
		}

		const TSharedRef<FVoxelChunkExecObject_CreateMeshComponent> Object = MakeShared<FVoxelChunkExecObject_CreateMeshComponent>();
		Object->Position = BoundsQueryData->Bounds.Min;
		Object->Mesh = Mesh;
		return Object;
	};
}