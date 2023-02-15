// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelLandmassHeightmapPreviewMesh.h"
#include "VoxelLandmassSettings.h"

void FVoxelLandmassHeightmapPreviewMesh::Initialize(const UVoxelHeightmap& Heightmap)
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelHeightmap& HeightmapData = *Heightmap.Heightmap;
	const FVoxelHeightmapConfig& Config = Heightmap.Config;

	const int32 SizeX = FMath::Min(128, HeightmapData.GetSizeX());
	const int32 SizeY = FMath::Min(128, HeightmapData.GetSizeY());

	const int32 CellSizeX = FVoxelUtilities::DivideCeil(HeightmapData.GetSizeX(), SizeX);
	const int32 CellSizeY = FVoxelUtilities::DivideCeil(HeightmapData.GetSizeY(), SizeY);

	TVoxelArray<float> Heights;
	FVoxelUtilities::SetNumFast(Heights, SizeX * SizeY);

	for (int32 X = 0; X < SizeX; X++)
	{
		for (int32 Y = 0; Y < SizeY; Y++)
		{
			float Height = 0;
			for (int32 I = 0; I < CellSizeX; I++)
			{
				for (int32 J = 0; J < CellSizeY; J++)
				{
					const int32 FullX = X * CellSizeX + I - CellSizeX / 2;
					const int32 FullY = Y * CellSizeY + J - CellSizeY / 2;

					if (FullX < 0 ||
						FullY < 0 ||
						FullX >= HeightmapData.GetSizeX() ||
						FullY >= HeightmapData.GetSizeY())
					{
						continue;
					}

					Height = FMath::Max(Height, HeightmapData.GetHeight(FullX, FullY));
				}
			}
			Height = Config.ScaleZ * (Config.InternalOffsetZ + Config.InternalScaleZ * Height);
			Height += FMath::Abs(Height) * 0.01f;
			Heights[FVoxelUtilities::Get2DIndex<int32>(SizeX, SizeY, X, Y)] = Height;
		}
	}

	TVoxelArray<FVector3f> Vertices;
	TVoxelArray<int32> Indices;
	Vertices.Reserve(SizeX * SizeY * 4);
	Indices.Reserve(SizeX * SizeY * 6);

	const float ScaleX = Config.ScaleXY * HeightmapData.GetSizeX() / float(SizeX);
	const float ScaleY = Config.ScaleXY * HeightmapData.GetSizeY() / float(SizeY);

	for (int32 X = 0; X < SizeX - 1; X++)
	{
		for (int32 Y = 0; Y < SizeY - 1; Y++)
		{
			const float Height00 = Heights[FVoxelUtilities::Get2DIndex<int32>(SizeX, SizeY, X + 0, Y + 0)];
			const float Height01 = Heights[FVoxelUtilities::Get2DIndex<int32>(SizeX, SizeY, X + 1, Y + 0)];
			const float Height10 = Heights[FVoxelUtilities::Get2DIndex<int32>(SizeX, SizeY, X + 0, Y + 1)];
			const float Height11 = Heights[FVoxelUtilities::Get2DIndex<int32>(SizeX, SizeY, X + 1, Y + 1)];

			const int32 Index00 = Vertices.Add(FVector3f(ScaleX * (X + 0 - SizeX / 2.f), ScaleY * (Y + 0 - SizeY / 2.f), Height00));
			const int32 Index01 = Vertices.Add(FVector3f(ScaleX * (X + 1 - SizeX / 2.f), ScaleY * (Y + 0 - SizeY / 2.f), Height01));
			const int32 Index10 = Vertices.Add(FVector3f(ScaleX * (X + 0 - SizeX / 2.f), ScaleY * (Y + 1 - SizeY / 2.f), Height10));
			const int32 Index11 = Vertices.Add(FVector3f(ScaleX * (X + 1 - SizeX / 2.f), ScaleY * (Y + 1 - SizeY / 2.f), Height11));

			Indices.Add(Index11);
			Indices.Add(Index01);
			Indices.Add(Index00);

			Indices.Add(Index10);
			Indices.Add(Index11);
			Indices.Add(Index00);
		}
	}

	Bounds = FBox(FBox3f(Vertices));

	IndexBuffer.SetIndices(ReinterpretCastArray<uint32>(Indices), EIndexBufferStride::Force32Bit);
	PositionVertexBuffer.Init(Vertices.Num(), false);
	StaticMeshVertexBuffer.SetUseFullPrecisionUVs(false);
	StaticMeshVertexBuffer.Init(Vertices.Num(), 1, false);
	ColorVertexBuffer.Init(Vertices.Num(), false);

	for (int32 Index = 0; Index < Vertices.Num(); Index++)
	{
		PositionVertexBuffer.VertexPosition(Index) = Vertices[Index];
	}

	for (int32 Index = 0; Index < Vertices.Num(); Index++)
	{
		StaticMeshVertexBuffer.SetVertexTangents(
			Index,
			FVector3f(1, 0, 0),
			FVector3f(0, 1, 0),
			FVector3f(0, 0, 1));

		StaticMeshVertexBuffer.SetVertexUV(Index, 0, FVector2f(ForceInit));
		ColorVertexBuffer.VertexColor(Index) = FColor(ForceInit);
	}
}

int64 FVoxelLandmassHeightmapPreviewMesh::GetAllocatedSize() const
{
	return
		IndexBuffer.GetAllocatedSize() +
		PositionVertexBuffer.GetNumVertices() * PositionVertexBuffer.GetStride() +
		StaticMeshVertexBuffer.GetResourceSize() +
		ColorVertexBuffer.GetNumVertices() * ColorVertexBuffer.GetStride();
}

void FVoxelLandmassHeightmapPreviewMesh::Initialize_GameThread()
{
	Material = FVoxelMaterialRef::Make(GetDefault<UVoxelLandmassSettings>()->HeightmapMaterial.LoadSynchronous());
}

void FVoxelLandmassHeightmapPreviewMesh::Initialize_RenderThread(ERHIFeatureLevel::Type FeatureLevel)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInRenderingThread());

	IndexBuffer.InitResource();
	PositionVertexBuffer.InitResource();
	StaticMeshVertexBuffer.InitResource();
	ColorVertexBuffer.InitResource();

	check(!VertexFactory);
	VertexFactory = MakeShared<FLocalVertexFactory>(FeatureLevel, "FVoxelDefaultMeshRenderData");

	FLocalVertexFactory::FDataType Data;
	PositionVertexBuffer.BindPositionVertexBuffer(VertexFactory.Get(), Data);
	StaticMeshVertexBuffer.BindTangentVertexBuffer(VertexFactory.Get(), Data);
	StaticMeshVertexBuffer.BindPackedTexCoordVertexBuffer(VertexFactory.Get(), Data);
	ColorVertexBuffer.BindColorVertexBuffer(VertexFactory.Get(), Data);

	VertexFactory->SetData(Data);
	VertexFactory->InitResource();
}

void FVoxelLandmassHeightmapPreviewMesh::Destroy_RenderThread()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInRenderingThread());

	IndexBuffer.ReleaseResource();
	PositionVertexBuffer.ReleaseResource();
	StaticMeshVertexBuffer.ReleaseResource();
	ColorVertexBuffer.ReleaseResource();

	check(VertexFactory);
	VertexFactory->ReleaseResource();
	VertexFactory.Reset();
}

bool FVoxelLandmassHeightmapPreviewMesh::Draw_RenderThread(
	const FPrimitiveSceneProxy& Proxy,
	FMeshBatch& MeshBatch) const
{
	MeshBatch.Type = PT_TriangleList;
	MeshBatch.VertexFactory = VertexFactory.Get();
	
	FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
	BatchElement.IndexBuffer = &IndexBuffer;
	BatchElement.FirstIndex = 0;
	BatchElement.NumPrimitives = IndexBuffer.GetNumIndices() / 3;
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = PositionVertexBuffer.GetNumVertices() - 1;
	
	return true;
}