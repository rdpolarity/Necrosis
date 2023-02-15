// Copyright Voxel Plugin, Inc. All Rights Reserved.

#if 0 // TODO
#include "Nodes/VoxelCubicGreedyNodes.h"
#include "Nodes/VoxelCubicGreedyHelpers.h"
#include "Nodes/VoxelPositionNodes.h"
#include "VoxelTextureAtlas.h"
#include "VoxelMetaGraphRuntimeUtilities.h"

struct FVoxelCubicGreedyProcessor
{
	static constexpr int32 ChunkSize = FVoxelPackedQuad::ChunkSize;
	static constexpr int32 ChunkSizeWithNeighbors = (ChunkSize + 2);
	static constexpr int32 NumVoxels = FMath::Cube(ChunkSize);
	static constexpr int32 NumVoxelsWithNeighbors = FMath::Cube(ChunkSizeWithNeighbors);

	TVoxelStaticBitArray<NumVoxelsWithNeighbors> Values;

	FORCEINLINE bool IsEmpty(int32 X, int32 Y, int32 Z) const
	{
		checkVoxelSlow(-1 <= X && X < ChunkSize + 1);
		checkVoxelSlow(-1 <= Y && Y < ChunkSize + 1);
		checkVoxelSlow(-1 <= Z && Z < ChunkSize + 1);
		return !Values.Test((X + 1) + (Y + 1) * (ChunkSize + 2) + (Z + 1) * (ChunkSize + 2) * (ChunkSize + 2));
	}
	FORCEINLINE static int32 GetFaceIndex(int32 X, int32 Y, int32 Z)
	{
		return X + Y * ChunkSize + Z * ChunkSize * ChunkSize;
	}

	void FindFaces(TVoxelStaticArray<TVoxelStaticBitArray<NumVoxels>, 6>& Faces)
	{
		VOXEL_FUNCTION_COUNTER();

		Faces.Memzero();

		for (int32 Z = 0; Z < ChunkSize; Z++)
		{
			for (int32 Y = 0; Y < ChunkSize; Y++)
			{
				const int32 Index = FVoxelUtilities::Get3DIndex(ChunkSize + 2, 0, Y, Z, -1) / 32;
				if (ChunkSize <= 32 && Values.GetWord(Index) == 0 && Values.GetWord(Index + 1) == 0)
				{
					// Both words are entirely empty
					// A bit too strict, but can lead to big perf increase
					continue;
				}

				for (int32 X = 0; X < ChunkSize; X++)
				{
					if (IsEmpty(X, Y, Z))
					{
						continue;
					}

					if (IsEmpty(X - 1, Y, Z)) Faces[0].Set(GetFaceIndex(Y, Z, X), true);
					if (IsEmpty(X + 1, Y, Z)) Faces[1].Set(GetFaceIndex(Y, Z, X), true);
					if (IsEmpty(X, Y - 1, Z)) Faces[2].Set(GetFaceIndex(Z, X, Y), true);
					if (IsEmpty(X, Y + 1, Z)) Faces[3].Set(GetFaceIndex(Z, X, Y), true);
					if (IsEmpty(X, Y, Z - 1)) Faces[4].Set(GetFaceIndex(X, Y, Z), true);
					if (IsEmpty(X, Y, Z + 1)) Faces[5].Set(GetFaceIndex(X, Y, Z), true);
				}
			}
		}
	}

	void Process(const FVoxelBoolBuffer& BoolValues, TVoxelArray<FVoxelPackedQuad>& Quads)
	{
		VOXEL_FUNCTION_COUNTER();

		if (BoolValues.IsConstant())
		{
			Values.SetAll(BoolValues.GetTypedConstant());
		}
		else
		{
			VOXEL_USE_NAMESPACE(MetaGraph);

			FRuntimeUtilities::UnpackData(
				BoolValues.MakeView().GetRawView(),
				Values,
				FIntVector(ChunkSizeWithNeighbors));
		}

		if (Values.TryGetAll().IsSet())
		{
			return;
		}

		TVoxelStaticArray<TVoxelStaticBitArray<NumVoxels>, 6> Faces{ NoInit };
		FindFaces(Faces);
		
		VOXEL_SCOPE_COUNTER("GreedyMeshing");

		TArray<FVoxelUtilities::FGreedyMeshingQuad> LocalQuads;
		LocalQuads.Reserve(NumVoxels);
		Quads.Reserve(NumVoxels);

		for (int32 Direction = 0; Direction < 6; Direction++)
		{
			LocalQuads.Reset();
			FVoxelUtilities::GreedyMeshing2D_Static<ChunkSize>(Faces[Direction], LocalQuads);

			for (const FVoxelUtilities::FGreedyMeshingQuad& Quad : LocalQuads)
			{
				checkVoxelSlow(Quad.Layer < ChunkSize);
				checkVoxelSlow(Quad.StartX < ChunkSize);
				checkVoxelSlow(Quad.StartY < ChunkSize);
				checkVoxelSlow(0 < Quad.SizeX);
				checkVoxelSlow(0 < Quad.SizeY);
				checkVoxelSlow(Quad.SizeX - 1 < ChunkSize);
				checkVoxelSlow(Quad.SizeY - 1 < ChunkSize);

				FVoxelPackedQuad PackedQuad;
				PackedQuad.Direction = Direction;
				PackedQuad.Layer = Quad.Layer;
				PackedQuad.StartX = Quad.StartX;
				PackedQuad.StartY = Quad.StartY;
				PackedQuad.SizeXMinus1 = Quad.SizeX - 1;
				PackedQuad.SizeYMinus1 = Quad.SizeY - 1;
				Quads.Emplace(PackedQuad);
			}
		}
	}
};

DEFINE_VOXEL_NODE(FVoxelNode_MakeCubicGreedyMeshData)
{
	ComputeVoxelQueryData(FVoxelBoundsQueryData, BoundsQueryData);
	ComputeVoxelQueryData(FVoxelLODQueryData, LODQueryData);

	const FVoxelBox Bounds = BoundsQueryData.Bounds;
	const int32 LOD = LODQueryData.LOD;

	const float VoxelSize = VoxelSizePin.Get2(Query);
	const float ScaledVoxelSize = VoxelSize * (1 << LOD);

	const FIntVector Start = FVoxelUtilities::RoundToInt(Bounds.Min / ScaledVoxelSize);
	const FIntVector End = FVoxelUtilities::RoundToInt(Bounds.Max / ScaledVoxelSize);
	const FIntVector Size = End - Start;

	if (Size % FVoxelPackedQuad::ChunkSize != FIntVector::ZeroValue)
	{
		VOXEL_MESSAGE(Error, "{0}: Chunk Size should be a multiple of 32 x Voxel Size x pow(2, LOD). Chunk Size = {1}, Voxel Size = {2}, LOD = {3}",
			this,
			Bounds.Size().ToString(),
			VoxelSize,
			LOD);
		return nullptr;
	}

	if (Size.GetMax() > 256)
	{
		VOXEL_MESSAGE(Error, "{0}: Size would be {1}, which is above the limit of 256x256x256", this, *Size.ToString());
		return nullptr;
	}

	const TSharedRef<FVoxelCubicGreedyMeshData> MeshData = MakeShared<FVoxelCubicGreedyMeshData>();
	MeshData->ScaledVoxelSize = ScaledVoxelSize;
	MeshData->Offset = Start;

	const TSharedRef<FVoxelCubicGreedyMeshData::FInnerMeshData> InnerMeshData = MakeShared<FVoxelCubicGreedyMeshData::FInnerMeshData>();
	MeshData->InnerMeshData = InnerMeshData;
	
	for (int32 X = Start.X; X < End.X; X += FVoxelPackedQuad::ChunkSize)
	{
		for (int32 Y = Start.Y; Y < End.Y; Y += FVoxelPackedQuad::ChunkSize)
		{
			for (int32 Z = Start.Z; Z < End.Z; Z += FVoxelPackedQuad::ChunkSize)
			{
				const FIntVector ChunkPosition(X, Y, Z);

				FVoxelQuery ValueQuery = Query;
				ValueQuery.Add<FVoxelDensePositionQueryData>().Initialize(
					FVector3f(ChunkPosition) * ScaledVoxelSize - ScaledVoxelSize, 
					ScaledVoxelSize, 
					FIntVector(FVoxelCubicGreedyProcessor::ChunkSizeWithNeighbors));

				const FVoxelBoolBuffer BoolValues = ValuesPin.Get2(ValueQuery);

				TVoxelArray<FVoxelPackedQuad> Quads;
				FVoxelCubicGreedyProcessor().Process(BoolValues, Quads);

				if (Quads.Num() == 0)
				{
					continue;
				}
				MeshData->NumQuads += Quads.Num();

				for (int32 Index = 0; Index < Quads.Num(); Index++)
				{
					const FVoxelPackedQuad& Quad = Quads[Index];
					MeshData->NumPixels += Quad.SizeX() * Quad.SizeY();
				}
				InnerMeshData->PositionsToQuery.Reserve(MeshData->NumPixels);
				
				{
					VOXEL_SCOPE_COUNTER("Find positions");
					for (int32 Index = 0; Index < Quads.Num(); Index++)
					{
						const FVoxelPackedQuad& Quad = Quads[Index];

						const FIntVector Axes = Quad.GetAxes();

						for (int32 QuadY = 0; QuadY < Quad.SizeY(); QuadY++)
						{
							for (int32 QuadX = 0; QuadX < Quad.SizeX(); QuadX++)
							{
								FIntVector Position;
								Position[Axes.X] = Quad.StartX + QuadX;
								Position[Axes.Y] = Quad.StartY + QuadY;
								Position[Axes.Z] = Quad.Layer;

								InnerMeshData->PositionsToQuery.Add(FVector3f(ChunkPosition + Position) * ScaledVoxelSize);
							}
						}
					}
				}

				InnerMeshData->Chunks.Add({ FIntVector(X, Y, Z) - Start, MoveTemp(Quads) });
			}
		}
	}

	return MeshData;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_CubicGreedy_AddColorTexture)
{
	const TSharedRef<FVoxelCubicGreedyMeshData> MeshData = MeshDataPin.GetCopy(Query);
	if (MeshData->NumQuads == 0)
	{
		return nullptr;
	}
	
	const FName Name = NamePin.Get2(Query);
	if (MeshData->TextureDatas.Contains(Name))
	{
		VOXEL_MESSAGE(Error, "{0}: cannot add texture named {1}, one already exists", this, Name);
		return nullptr;
	}

	FVoxelQuery ColorsQuery = Query;
	ColorsQuery.Add<FVoxelSparsePositionQueryData>().Initialize(MeshData->InnerMeshData->PositionsToQuery);
	const FVoxelLinearColorBuffer Colors = ColorsPin.Get2(ColorsQuery);
	const TVoxelBufferView<FLinearColor> ColorsView = Colors.MakeView();
	
	const TSharedRef<FVoxelTextureAtlasTextureData> TextureData = MakeShared<FVoxelTextureAtlasTextureData>(PF_B8G8R8A8);
	FVoxelUtilities::SetNumFast(TextureData->Data, MeshData->NumPixels * sizeof(FColor));
	{
		VOXEL_SCOPE_COUNTER("Copy colors");

		const TVoxelArrayView<FColor> TextureColorView = ReinterpretCastVoxelArrayView<FColor>(TextureData->Data);
		for (int32 Index = 0; Index < MeshData->NumPixels; Index++)
		{
			TextureColorView[Index] = ColorsView[Index].ToFColor(false);
		}
	}

	MeshData->TextureDatas.Add(Name, TextureData);

	return MeshData;
}

DEFINE_VOXEL_NODE(FVoxelNode_CubicGreedy_AddIndexTexture)
{
	const TSharedRef<FVoxelCubicGreedyMeshData> MeshData = MeshDataPin.GetCopy(Query);
	if (MeshData->NumQuads == 0)
	{
		return nullptr;
	}
	
	const FName Name = NamePin.Get2(Query);
	if (MeshData->TextureDatas.Contains(Name))
	{
		VOXEL_MESSAGE(Error, "{0}: cannot add texture named {1}, one already exists", this, Name);
		return nullptr;
	}

	const bool bFullPrecision = FullPrecisionPin.Get2(Query);

	FVoxelQuery IndicesQuery = Query;
	IndicesQuery.Add<FVoxelSparsePositionQueryData>().Initialize(MeshData->InnerMeshData->PositionsToQuery);
	const FVoxelInt32Buffer Indices = IndicesPin.Get2(IndicesQuery);
	const TVoxelBufferView<int32> IndicesView = Indices.MakeView();

	if (bFullPrecision)
	{
		const TSharedRef<FVoxelTextureAtlasTextureData> TextureData = MakeShared<FVoxelTextureAtlasTextureData>(PF_R32_SINT);
		FVoxelUtilities::SetNumFast(TextureData->Data, MeshData->NumPixels * sizeof(int32));
		{
			VOXEL_SCOPE_COUNTER("Copy indices");

			const TVoxelArrayView<int32> TextureIndexView = ReinterpretCastVoxelArrayView<int32>(TextureData->Data);
			for (int32 Index = 0; Index < MeshData->NumPixels; Index++)
			{
				TextureIndexView[Index] = IndicesView[Index];
			}
		}
		MeshData->TextureDatas.Add(Name, TextureData);
	}
	else
	{
		const TSharedRef<FVoxelTextureAtlasTextureData> TextureData = MakeShared<FVoxelTextureAtlasTextureData>(PF_R8_UINT);
		FVoxelUtilities::SetNumFast(TextureData->Data, MeshData->NumPixels * sizeof(uint8));
		{
			VOXEL_SCOPE_COUNTER("Copy indices");

			for (int32 Index = 0; Index < MeshData->NumPixels; Index++)
			{
				const int32 Value = IndicesView[Index];
				if (Value < 0 || Value > 255)
				{
					VOXEL_MESSAGE(Error, "{0}: value out of bound with FullPrecision = false: 0 <= {1} < 256", this, Value);
				}

				TextureData->Data[Index] = FVoxelUtilities::ClampToUINT8(Value);
			}
		}
		MeshData->TextureDatas.Add(Name, TextureData);
	}

	return MeshData;
}

DEFINE_VOXEL_NODE(FVoxelNode_CubicGreedy_AddScalarTexture)
{
	const TSharedRef<FVoxelCubicGreedyMeshData> MeshData = MeshDataPin.GetCopy(Query);
	if (MeshData->NumQuads == 0)
	{
		return nullptr;
	}
	
	const FName Name = NamePin.Get2(Query);
	if (MeshData->TextureDatas.Contains(Name))
	{
		VOXEL_MESSAGE(Error, "{0}: cannot add texture named {1}, one already exists", this, Name);
		return nullptr;
	}

	FVoxelQuery ScalarQuery = Query;
	ScalarQuery.Add<FVoxelSparsePositionQueryData>().Initialize(MeshData->InnerMeshData->PositionsToQuery);
	const FVoxelFloatBuffer Scalars = ScalarsPin.Get2(ScalarQuery);
	const TVoxelBufferView<float> ScalarsView = Scalars.MakeView();

	const TSharedRef<FVoxelTextureAtlasTextureData> TextureData = MakeShared<FVoxelTextureAtlasTextureData>(PF_R32_FLOAT);
	FVoxelUtilities::SetNumFast(TextureData->Data, MeshData->NumPixels * sizeof(float));
	{
		VOXEL_SCOPE_COUNTER("Copy scalars");

		const TVoxelArrayView<float> TextureScalarView = ReinterpretCastVoxelArrayView<float>(TextureData->Data);
		for (int32 Index = 0; Index < MeshData->NumPixels; Index++)
		{
			TextureScalarView[Index] = ScalarsView[Index];
		}
	}

	MeshData->TextureDatas.Add(Name, TextureData);

	return MeshData;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelCubicGreedyMeshResult::Create(FVoxelRuntime& Runtime) const
{
	FVoxelMeshRenderer* Renderer = Runtime.GetSubsystem<FVoxelMeshRenderer>();
	if (!ensure(Renderer))
	{
		return;
	}

	ensure(!MeshId.IsValid());
	MeshId = Renderer->CreateMesh(Position, TArray<TSharedPtr<FVoxelMesh>>(Meshes));
}

void FVoxelCubicGreedyMeshResult::Destroy(FVoxelRuntime& Runtime) const
{
	FVoxelMeshRenderer* Renderer = Runtime.GetSubsystem<FVoxelMeshRenderer>();
	if (!ensure(Renderer))
	{
		return;
	}

	ensure(MeshId.IsValid());
	Renderer->DestroyMesh(MeshId);
	MeshId = {};
}

DEFINE_VOXEL_NODE(FVoxelNode_RenderCubicGreedyMesh)
{
	ComputeVoxelQueryData(FVoxelLODQueryData, LODQueryData);

	const TSharedRef<const FVoxelCubicGreedyMeshData> MeshData = MeshDataPin.Get2(Query);
	if (MeshData->NumQuads == 0)
	{
		return nullptr;
	}

	const TSharedRef<FVoxelCubicGreedyMesh> Mesh = MakeVoxelMesh<FVoxelCubicGreedyMesh>();
	Mesh->LOD = LODQueryData.LOD;
	Mesh->GetMaterial_GameThread = MaterialPin.Get2(Query)->GetGetMaterial();

	// Make sure this stays in sync with the ones in VoxelCubicGreedyVertexFactory.usf
	struct FVoxelPackedQuad0
	{
		uint32 Direction : 3;  // 0-2
		uint32 Layer : 5;  // 3-7
		uint32 StartX : 5;  // 8-12
		uint32 StartY : 5;  // 13-17
		uint32 SizeXMinus1 : 5;  // 18-22
		uint32 SizeYMinus1 : 5;  // 23-27
		uint32 ChunkX : 4;  // 28-31
	};
	struct FVoxelPackedQuad1
	{
		uint32 ChunkY : 4;  // 0-3
		uint32 ChunkZ : 4;  // 4-7
		uint32 TextureIndex : 24;  // 8-31
	};

	constexpr int32 ChunkSize = FVoxelPackedQuad::ChunkSize;

	Mesh->Bounds = FBox(ForceInit);
	Mesh->NumQuads = MeshData->NumQuads;
	Mesh->ScaledVoxelSize = MeshData->ScaledVoxelSize;
	Mesh->TextureDatas = MeshData->TextureDatas;

	FVoxelUtilities::SetNumFast(Mesh->QuadsBuffer.Quads, 2 * MeshData->NumQuads);

	int32 QuadIndex = 0;
	int32 TextureIndex = 0;
	for (const FVoxelCubicGreedyMeshData::FChunk& Chunk : MeshData->InnerMeshData->Chunks)
	{
		VOXEL_SCOPE_COUNTER("AddChunk");

		checkVoxelSlow(Chunk.Offset % ChunkSize == 0);
		const FIntVector ChunkPosition = Chunk.Offset / ChunkSize;
		checkVoxelSlow(ChunkPosition.X >= 0);
		checkVoxelSlow(ChunkPosition.Y >= 0);
		checkVoxelSlow(ChunkPosition.Z >= 0);

		if (ChunkPosition.X >= (1 << 4) ||
			ChunkPosition.Y >= (1 << 4) ||
			ChunkPosition.Z >= (1 << 4))
		{
			ensure(false);
		}

		for (const FVoxelPackedQuad& Quad : Chunk.Quads)
		{
			{
				const FVector MinPosition(Chunk.Offset + Quad.GetPosition(false, false, false));
				const FVector MaxPosition(Chunk.Offset + Quad.GetPosition(true, true, true));
				Mesh->Bounds += FBox(MinPosition, MaxPosition);
			}

			FVoxelPackedQuad0 Quad0;
			FVoxelPackedQuad1 Quad1;

			Quad0.Direction = Quad.Direction;
			Quad0.Layer = Quad.Layer;
			Quad0.StartX = Quad.StartX;
			Quad0.StartY = Quad.StartY;
			Quad0.SizeXMinus1 = Quad.SizeXMinus1;
			Quad0.SizeYMinus1 = Quad.SizeYMinus1;

			Quad0.ChunkX = ChunkPosition.X;
			Quad1.ChunkY = ChunkPosition.Y;
			Quad1.ChunkZ = ChunkPosition.Z;

			checkVoxelSlow(TextureIndex < (1 << 24));
			Quad1.TextureIndex = TextureIndex;

			static_assert(sizeof(FVoxelPackedQuad0) == sizeof(uint32), "");
			static_assert(sizeof(FVoxelPackedQuad1) == sizeof(uint32), "");
			Mesh->QuadsBuffer.Quads[2 * QuadIndex + 0] = *reinterpret_cast<uint32*>(&Quad0);
			Mesh->QuadsBuffer.Quads[2 * QuadIndex + 1] = *reinterpret_cast<uint32*>(&Quad1);

			QuadIndex++;
			TextureIndex += Quad.SizeX() * Quad.SizeY();
		}
	}
	ensure(QuadIndex == MeshData->NumQuads);
	ensure(TextureIndex == MeshData->NumPixels);

	Mesh->Bounds.Min *= Mesh->ScaledVoxelSize;
	Mesh->Bounds.Max *= Mesh->ScaledVoxelSize;

	const TSharedRef<FVoxelCubicGreedyMeshResult> Result = MakeShared<FVoxelCubicGreedyMeshResult>();
	Result->Position = FVector3d(MeshData->Offset) * MeshData->ScaledVoxelSize;
	Result->Meshes.Add(Mesh);
	return Result;
}
#endif