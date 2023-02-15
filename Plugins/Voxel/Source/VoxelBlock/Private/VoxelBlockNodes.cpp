// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelBlockNodes.h"
#include "VoxelBlockMesh.h"
#include "VoxelBlockRenderer.h"
#include "VoxelMetaGraphRuntimeUtilities.h"
#include "VoxelCollision/VoxelCollisionCooker.h"
#include "VoxelCollision/VoxelTriangleMeshCollider.h"

void FVoxelBlockSparseQueryData::Initialize(TConstVoxelArrayView<FIntVector> Positions)
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelArray<int32> PositionsX = FVoxelInt32Buffer::Allocate(Positions.Num());
	TVoxelArray<int32> PositionsY = FVoxelInt32Buffer::Allocate(Positions.Num());
	TVoxelArray<int32> PositionsZ = FVoxelInt32Buffer::Allocate(Positions.Num());

	for (int32 Index = 0; Index < Positions.Num(); Index++)
	{
		const FIntVector Position = Positions[Index];

		PositionsX[Index] = Position.X;
		PositionsY[Index] = Position.Y;
		PositionsZ[Index] = Position.Z;
	}

	PrivatePositions = FVoxelIntVectorBuffer::MakeCpu(PositionsX, PositionsY, PositionsZ);
}

void FVoxelBlockSparseQueryData::Initialize(const FVoxelIntVectorBuffer& Positions)
{
	PrivatePositions = Positions;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelBlockChunkQueryData::Initialize(const FVoxelIntBox& Bounds)
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_USE_NAMESPACE(MetaGraph);

	PrivateBounds = Bounds;

	const FIntVector Size = PrivateBounds.Size();

	TVoxelArray<int32> PositionsX = FVoxelInt32Buffer::Allocate(Size.X * Size.Y * Size.Z);
	TVoxelArray<int32> PositionsY = FVoxelInt32Buffer::Allocate(Size.X * Size.Y * Size.Z);
	TVoxelArray<int32> PositionsZ = FVoxelInt32Buffer::Allocate(Size.X * Size.Y * Size.Z);

	FRuntimeUtilities::WriteIntPositions_Unpacked(
		PositionsX,
		PositionsY,
		PositionsZ,
		PrivateBounds.Min,
		1,
		Size);

	CachedPositions = FVoxelIntVectorBuffer::MakeCpu(PositionsX, PositionsY, PositionsZ);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int32 FVoxelBlockSurface::GetNumFaces() const
{
	int32 NumFaces = 0;
	for (const FFaceMesh& FaceMesh : FaceMeshes)
	{
		check(FaceMesh.Positions.Num() == FaceMesh.BlockDatas.Num());
		NumFaces += FaceMesh.Positions.Num();
	}
	return NumFaces;
}

void FVoxelBlockSurface::GetFaceData(
	const EVoxelBlockFace Face, 
	TVoxelStaticArray<FVector3f, 4>& OutPositions, 
	FVector3f& OutNormal, 
	FVector3f& OutTangent)
{
	/**
	 * 0 --- 1
	 * |  \  |
	 * 3 --- 2
	 *
	 * Triangles: 0 1 2, 0 2 3
	 */

	switch (Face)
	{
	default: VOXEL_ASSUME(false);
	case EVoxelBlockFace::XMin:
	{
		OutPositions[0] = FVector3f(0, 0, 1);
		OutPositions[1] = FVector3f(0, 1, 1);
		OutPositions[2] = FVector3f(0, 1, 0);
		OutPositions[3] = FVector3f(0, 0, 0);
		OutNormal = FVector3f(-1, 0, 0);
		OutTangent = FVector3f(0, 1, 0);
	}
	break;
	case EVoxelBlockFace::XMax:
	{
		OutPositions[0] = FVector3f(1, 1, 1);
		OutPositions[1] = FVector3f(1, 0, 1);
		OutPositions[2] = FVector3f(1, 0, 0);
		OutPositions[3] = FVector3f(1, 1, 0);
		OutNormal = FVector3f(1, 0, 0);
		OutTangent = FVector3f(0, -1, 0);
	}
	break;
	case EVoxelBlockFace::YMin:
	{
		OutPositions[0] = FVector3f(1, 0, 1);
		OutPositions[1] = FVector3f(0, 0, 1);
		OutPositions[2] = FVector3f(0, 0, 0);
		OutPositions[3] = FVector3f(1, 0, 0);
		OutNormal = FVector3f(0, -1, 0);
		OutTangent = FVector3f(-1, 0, 0);
	}
	break;
	case EVoxelBlockFace::YMax:
	{
		OutPositions[0] = FVector3f(0, 1, 1);
		OutPositions[1] = FVector3f(1, 1, 1);
		OutPositions[2] = FVector3f(1, 1, 0);
		OutPositions[3] = FVector3f(0, 1, 0);
		OutNormal = FVector3f(0, 1, 0);
		OutTangent = FVector3f(1, 0, 0);
	}
	break;
	case EVoxelBlockFace::ZMin:
	{
		OutPositions[0] = FVector3f(0, 1, 0);
		OutPositions[1] = FVector3f(1, 1, 0);
		OutPositions[2] = FVector3f(1, 0, 0);
		OutPositions[3] = FVector3f(0, 0, 0);
		OutNormal = FVector3f(0, 0, -1);
		OutTangent = FVector3f(1, 0, 0);
	}
	break;
	case EVoxelBlockFace::ZMax:
	{
		OutPositions[0] = FVector3f(1, 0, 1);
		OutPositions[1] = FVector3f(1, 1, 1);
		OutPositions[2] = FVector3f(0, 1, 1);
		OutPositions[3] = FVector3f(0, 0, 1);
		OutNormal = FVector3f(0, 0, 1);
		OutTangent = FVector3f(1, 0, 0);
	}
	break;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_FVoxelBlockSurface_GenerateSurface, Surface)
{
	FindVoxelQueryData(FVoxelBoundsQueryData, BoundsQueryData);
	FindVoxelQueryData(FVoxelLODQueryData, LODQueryData);
	if (LODQueryData->LOD != 0)
	{
		return {};
	}

	const TValue<float> BlockSize = Get(BlockSizePin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, LODQueryData, BlockSize)
	{
		if (BlockSize <= 0)
		{
			VOXEL_MESSAGE(Error, "{0}: invalid block size ({1})", this, BlockSize);
			return {};
		}

		const FVoxelBox Bounds = BoundsQueryData->Bounds;
		const FIntVector Size = FVoxelUtilities::CeilToInt(Bounds.Size() / BlockSize);
		const FIntVector SizeWithNeighbors = Size + 2;
		const FIntVector Start = FVoxelUtilities::FloorToInt(Bounds.Min / BlockSize) - 1;

		const int64 NumQueriedVoxels = int64(SizeWithNeighbors.X) * int64(SizeWithNeighbors.Y) * int64(SizeWithNeighbors.Z);
		if (NumQueriedVoxels >= 32 * 1024 * 1024)
		{
			VOXEL_MESSAGE(Error, "{0}: too many voxels queried ({1})", this, NumQueriedVoxels);
			return {};
		}

		FVoxelQuery BlockQuery = Query;
		BlockQuery.Add<FVoxelBlockChunkQueryData>().Initialize(FVoxelIntBox(Start, Start + SizeWithNeighbors));
		const TValue<TBufferView<FVoxelBlockData>> Blocks = GetBufferView(BlockPin, BlockQuery);

		return VOXEL_ON_COMPLETE(AsyncThread, BlockSize, Blocks, Size, SizeWithNeighbors)
		{
			const TSharedRef<FVoxelBlockSurface> Surface = MakeShared<FVoxelBlockSurface>();
			Surface->BlockSize = BlockSize;

			{
				VOXEL_SCOPE_COUNTER("FindFaces");

				for (int32 Z = 0; Z < Size.Z; Z++)
				{
					for (int32 Y = 0; Y < Size.Y; Y++)
					{
						int32 Index = FVoxelUtilities::Get3DIndex<int32>(SizeWithNeighbors, 0, Y, Z, -1);
						for (int32 X = 0; X < Size.X; X++)
						{
							checkVoxelSlow(Index == FVoxelUtilities::Get3DIndex<int32>(SizeWithNeighbors, X, Y, Z, -1));
							ON_SCOPE_EXIT
							{
								Index++;
							};

							const FVoxelBlockData BlockData = Blocks[Index];
							if (BlockData.IsAir())
							{
								continue;
							}

#define CASE(FaceIndex, InX, InY, InZ) \
							{ \
								const int32 OtherIndex = Index + InX + InY * SizeWithNeighbors.X + InZ * SizeWithNeighbors.X * SizeWithNeighbors.Y; \
								checkVoxelSlow(OtherIndex == FVoxelUtilities::Get3DIndex<int32>(SizeWithNeighbors, X + InX, Y + InY, Z + InZ, -1)); \
								const FVoxelBlockData OtherBlockData = Blocks[OtherIndex]; \
								\
								if (OtherBlockData.IsAir()) \
								{ \
									Surface->FaceMeshes[FaceIndex].Positions.Add(FIntVector(X, Y, Z)); \
									Surface->FaceMeshes[FaceIndex].BlockDatas.Add(BlockData); \
									goto End_ ## FaceIndex; \
								} \
								\
								if (!BlockData.IsMasked() && !OtherBlockData.IsMasked()) \
								{ \
									goto End_ ## FaceIndex; \
								} \
								\
								if ((BlockData.IsMasked() && (BlockData.AlwaysRenderInnerFaces() || BlockData.GetId() != OtherBlockData.GetId())) || \
									(OtherBlockData.IsMasked() && (OtherBlockData.AlwaysRenderInnerFaces() || BlockData.GetId() != OtherBlockData.GetId()))) \
								{ \
									Surface->FaceMeshes[FaceIndex].Positions.Add(FIntVector(X, Y, Z)); \
									Surface->FaceMeshes[FaceIndex].BlockDatas.Add(BlockData); \
								} \
							} \
							End_ ## FaceIndex:

							CASE(0, -1, 0, 0);
							CASE(1, +1, 0, 0);
							CASE(2, 0, -1, 0);
							CASE(3, 0, +1, 0);
							CASE(4, 0, 0, -1);
							CASE(5, 0, 0, +1);

#undef CASE
						}
					}
				}
			}

			return Surface;
		};
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_FVoxelBlockSurface_CreateCollider, Collider)
{
	const TValue<FVoxelBlockSurface> MeshData = Get(SurfacePin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, MeshData)
	{
		const int32 NumFaces = MeshData->GetNumFaces();
		if (NumFaces == 0)
		{
			return {};
		}

		TVoxelArray<int32> Indices;
		Indices.Reserve(NumFaces * 6);

		TVoxelArray<FVector3f> Vertices;
		Vertices.Reserve(NumFaces * 4);

		for (int32 Direction = 0; Direction < 6; Direction++)
		{
			TVoxelStaticArray<FVector3f, 4> CornerPositions{ NoInit };
			FVector3f Normal;
			FVector3f Tangent;
			FVoxelBlockSurface::GetFaceData(EVoxelBlockFace(Direction), CornerPositions, Normal, Tangent);

			const FVoxelBlockSurface::FFaceMesh& FaceMesh = MeshData->FaceMeshes[Direction];
			for (int32 FaceIndex = 0; FaceIndex < FaceMesh.Positions.Num(); FaceIndex++)
			{
				const int32 Index0 = Vertices.Add(MeshData->GetVertexPosition(Direction, FaceIndex, CornerPositions[0]));
				const int32 Index1 = Vertices.Add(MeshData->GetVertexPosition(Direction, FaceIndex, CornerPositions[1]));
				const int32 Index2 = Vertices.Add(MeshData->GetVertexPosition(Direction, FaceIndex, CornerPositions[2]));
				const int32 Index3 = Vertices.Add(MeshData->GetVertexPosition(Direction, FaceIndex, CornerPositions[3]));

				Indices.Add(Index2);
				Indices.Add(Index1);
				Indices.Add(Index0);

				Indices.Add(Index3);
				Indices.Add(Index2);
				Indices.Add(Index0);
			}
		}

		return FVoxelCollisionCooker::CookTriangleMesh(Indices, Vertices, {});
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_FVoxelBlockSurface_CreateNavmesh, Navmesh)
{
	const TValue<FVoxelBlockSurface> MeshData = Get(SurfacePin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, MeshData)
	{
		const int32 NumFaces = MeshData->GetNumFaces();
		if (NumFaces == 0)
		{
			return {};
		}

		TVoxelArray<int32> Indices;
		Indices.Reserve(NumFaces * 6);

		TVoxelArray<FVector3f> Vertices;
		Vertices.Reserve(NumFaces * 4);

		for (int32 Direction = 0; Direction < 6; Direction++)
		{
			TVoxelStaticArray<FVector3f, 4> CornerPositions{ NoInit };
			FVector3f Normal;
			FVector3f Tangent;
			FVoxelBlockSurface::GetFaceData(EVoxelBlockFace(Direction), CornerPositions, Normal, Tangent);

			const FVoxelBlockSurface::FFaceMesh& FaceMesh = MeshData->FaceMeshes[Direction];
			for (int32 FaceIndex = 0; FaceIndex < FaceMesh.Positions.Num(); FaceIndex++)
			{
				const int32 Index0 = Vertices.Add(MeshData->GetVertexPosition(Direction, FaceIndex, CornerPositions[0]));
				const int32 Index1 = Vertices.Add(MeshData->GetVertexPosition(Direction, FaceIndex, CornerPositions[1]));
				const int32 Index2 = Vertices.Add(MeshData->GetVertexPosition(Direction, FaceIndex, CornerPositions[2]));
				const int32 Index3 = Vertices.Add(MeshData->GetVertexPosition(Direction, FaceIndex, CornerPositions[3]));

				Indices.Add(Index0);
				Indices.Add(Index1);
				Indices.Add(Index2);

				Indices.Add(Index0);
				Indices.Add(Index2);
				Indices.Add(Index3);
			}
		}

		const TSharedRef<FVoxelNavmesh> Navmesh = MakeShared<FVoxelNavmesh>();
		Navmesh->Bounds = FBox(FBox3f(Navmesh->Vertices));
		Navmesh->Indices = MoveTemp(Indices);
		Navmesh->Vertices = MoveTemp(Vertices);
		return Navmesh;
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_FVoxelBlockSurface_CreateMesh, Mesh)
{
	FindVoxelQueryData(FVoxelLODQueryData, LODQueryData);

	const TValue<FVoxelBlockSurface> MeshData = Get(SurfacePin, Query);
	const TValue<FVoxelMeshMaterial> Material = Get(MaterialPin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, LODQueryData, MeshData, Material)
	{
		const int32 NumFaces = MeshData->GetNumFaces();
		if (NumFaces == 0)
		{
			return {};
		}

		FVoxelBlockRenderer& BlockRenderer = GetSubsystem<FVoxelBlockRenderer>();

		const TSharedRef<FVoxelBlockMesh> Mesh = MakeVoxelMesh<FVoxelBlockMesh>();
		Mesh->LOD = LODQueryData->LOD;
		{
			const TSharedRef<FVoxelMeshMaterialInstance> Instance = MakeShared<FVoxelMeshMaterialInstance>();
			Instance->Parent = Material;
			Instance->Parameters = BlockRenderer.GetInstanceParameters();
			Mesh->MeshMaterial = Instance;
		}

		const int32 NumVertices = NumFaces * 4;
		{
			VOXEL_SCOPE_COUNTER("Initialize buffers");

			Mesh->PositionVertexBuffer.Init(NumVertices, false);
			Mesh->StaticMeshVertexBuffer.SetUseFullPrecisionUVs(true);
			Mesh->StaticMeshVertexBuffer.Init(NumVertices, 1, false);
			Mesh->ColorVertexBuffer.Init(NumVertices, false);
		}

		TVoxelArray<uint32> Indices;
		Indices.Reserve(2 * NumFaces * 6);

		int32 VertexIndex = 0;
		for (int32 Direction = 0; Direction < 6; Direction++)
		{
			VOXEL_SCOPE_COUNTER("Process faces");

			const EVoxelBlockFace Face = EVoxelBlockFace(Direction);
			const FVoxelBlockSurface::FFaceMesh& FaceMesh = MeshData->FaceMeshes[Direction];
			check(FaceMesh.Positions.Num() == FaceMesh.BlockDatas.Num());

			TVoxelStaticArray<FVector3f, 4> CornerPositions{ NoInit };
			FVector3f Normal;
			FVector3f Tangent;
			FVoxelBlockSurface::GetFaceData(Face, CornerPositions, Normal, Tangent);

			for (int32 FaceIndex = 0; FaceIndex < FaceMesh.Positions.Num(); FaceIndex++)
			{
				const FVoxelBlockData BlockData = FaceMesh.BlockDatas[FaceIndex];

				TVoxelStaticArray<int32, 4> PositionsIndices{ NoInit };
				for (int32 CornerIndex = 0; CornerIndex < 4; CornerIndex++)
				{
					const FVector3f VertexPositionInBlock = CornerPositions[CornerIndex];

					Mesh->PositionVertexBuffer.VertexPosition(VertexIndex) = MeshData->GetVertexPosition(Direction, FaceIndex, VertexPositionInBlock);
					Mesh->StaticMeshVertexBuffer.SetVertexTangents(VertexIndex, Tangent, FVector3f::CrossProduct(Normal, Tangent), Normal);

					const EVoxelBlockRotation InvertedRotation = FVoxelBlockRotation::Invert(BlockData.GetRotation());
					const EVoxelBlockFace RotatedFace = FVoxelBlockRotation::Rotate(Face, InvertedRotation);

					{
						const FVector3f Position = FVoxelBlockRotation::Rotate(VertexPositionInBlock - 0.5f, InvertedRotation) + 0.5f;

						float U;
						float V;
						switch (RotatedFace)
						{
						default: VOXEL_ASSUME(false);
						case EVoxelBlockFace::XMin: U = Position.Y; V = Position.Z; break;
						case EVoxelBlockFace::XMax: U = 1 - Position.Y; V = Position.Z; break;
						case EVoxelBlockFace::YMin: U = 1 - Position.X; V = Position.Z; break;
						case EVoxelBlockFace::YMax: U = Position.X; V = Position.Z; break;
						case EVoxelBlockFace::ZMin: U = Position.X; V = Position.Y; break;
						case EVoxelBlockFace::ZMax: U = Position.X; V = 1 - Position.Y; break;
						}

						// Texture Y is down, so 1 - V
						Mesh->StaticMeshVertexBuffer.SetVertexUV(VertexIndex, 0, FVector2f(U, 1 - V));
					}

					Mesh->ColorVertexBuffer.VertexColor(VertexIndex) = BlockRenderer.GetPackedColor(BlockData.GetId(), RotatedFace);

					PositionsIndices[CornerIndex] = VertexIndex;

					VertexIndex++;
				}

				if (BlockData.IsTwoSided())
				{
					Indices.Add(PositionsIndices[0]);
					Indices.Add(PositionsIndices[1]);
					Indices.Add(PositionsIndices[2]);

					Indices.Add(PositionsIndices[0]);
					Indices.Add(PositionsIndices[2]);
					Indices.Add(PositionsIndices[3]);
				}

				Indices.Add(PositionsIndices[2]);
				Indices.Add(PositionsIndices[1]);
				Indices.Add(PositionsIndices[0]);

				Indices.Add(PositionsIndices[3]);
				Indices.Add(PositionsIndices[2]);
				Indices.Add(PositionsIndices[0]);
			}
		}
		ensure(VertexIndex == NumVertices);

		{
			VOXEL_SCOPE_COUNTER("Indices");

			Mesh->IndexBuffer.SetIndices(
				Indices,
				NumVertices > MAX_uint16
				? EIndexBufferStride::Force32Bit
				: EIndexBufferStride::Force16Bit);
		}

		{
			VOXEL_SCOPE_COUNTER("Compute bounds");

			Mesh->Bounds = FBox(ForceInit);

			for (uint32 Index = 0; Index < Mesh->PositionVertexBuffer.GetNumVertices(); Index++)
			{
				Mesh->Bounds += FVector(Mesh->PositionVertexBuffer.VertexPosition(Index));
			}
		}

		return Mesh;
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_GetBlockPosition, BlockPosition)
{
	FindVoxelQueryData(FVoxelBlockQueryData, BlockQueryData);

	return BlockQueryData->GetBlockPositions();
}