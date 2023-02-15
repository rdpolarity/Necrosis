// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelDetailTextureNodes.h"
#include "Nodes/VoxelPositionNodes.h"
#include "Nodes/MarchingCube/VoxelMarchingCubeNodes.h"
#include "VoxelGpuTexture.h"

DEFINE_VOXEL_NODE_CPU(FVoxelNode_MakeNormalDetailTexture, DetailTexture)
{
	FindVoxelQueryData(FVoxelDetailTextureQueryData, DetailTextureQueryData);

	const TValue<TBufferView<FVector>> Normals = GetBufferView(NormalPin, Query);
	const TValue<FName> Name = Get(NamePin, Query);
	const TValue<float> MaxNormalDifference = Get(MaxNormalDifferencePin, Query);
	const TValue<TBufferView<FVector>> CellNormals = DetailTextureQueryData->Normals.MakeView();
	
	return VOXEL_ON_COMPLETE(AsyncThread, DetailTextureQueryData, Normals, Name, MaxNormalDifference, CellNormals)
	{
		FindVoxelQueryData(FVoxelPositionQueryData, PositionQueryData);
		CheckVoxelBuffersNum(Normals, PositionQueryData->GetPositions());

		const TSharedRef<FVoxelDetailTexture> DetailTexture = MakeShared<FVoxelDetailTexture>();
		DetailTexture->Name = Name;
		DetailTexture->SizeX = DetailTextureQueryData->AtlasTextureSize;
		DetailTexture->SizeY = DetailTextureQueryData->AtlasTextureSize;
		DetailTexture->Format = PF_R16_UINT;
		DetailTexture->SetCpuData(*DetailTextureQueryData, [&](TVoxelArray<uint8>& Data, int32 QueryIndex, int32 DataIndex, int32 CellIndex)
		{
			const FVector3f CellNormal = CellNormals[CellIndex];

			FVector3f Normal = Normals[QueryIndex].GetSafeNormal(KINDA_SMALL_NUMBER, FVector3f::UpVector);

			const float Dot = FVector3f::DotProduct(Normal, CellNormal);
			if (Dot < MaxNormalDifference)
			{
				Normal = FMath::Lerp(CellNormal, Normal, (MaxNormalDifference - 1.f) / (Dot - 1.f)).GetSafeNormal();
			}

			const FVector2f Octahedron = FVoxelUtilities::UnitVectorToOctahedron(Normal);

			Data[2 * DataIndex + 0] = FVoxelUtilities::FloatToUINT8(Octahedron.X);
			Data[2 * DataIndex + 1] = FVoxelUtilities::FloatToUINT8(Octahedron.Y);
		});
		
		return DetailTexture;
	};
}

BEGIN_VOXEL_NAMESPACE(MetaGraph)

BEGIN_VOXEL_COMPUTE_SHADER(MakeNormalDetailTexture)
	VOXEL_SHADER_PARAMETER_CST(int32, TextureSize)
	VOXEL_SHADER_PARAMETER_CST(int32, NumCells)
	VOXEL_SHADER_PARAMETER_CST(int32, NumCellsPerSide)

	VOXEL_SHADER_PARAMETER_CST(FIntVector, ConstantNormals)

	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, NormalsX)
	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, NormalsY)
	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, NormalsZ)
	
	VOXEL_SHADER_PARAMETER_TEXTURE_UAV(Texture2D<uint>, Texture)
END_VOXEL_SHADER()

END_VOXEL_NAMESPACE(MetaGraph)

DEFINE_VOXEL_NODE_GPU(FVoxelNode_MakeNormalDetailTexture, DetailTexture)
{
	FindVoxelQueryData(FVoxelDetailTextureQueryData, DetailTextureQueryData);

	const TValue<FVoxelVectorBuffer> Normals = Get(NormalPin, Query);
	const TValue<FName> Name = Get(NamePin, Query);
	const TValue<float> MaxNormalDifference = Get(MaxNormalDifferencePin, Query);

	return VOXEL_ON_COMPLETE(RenderThread, DetailTextureQueryData, Normals, Name, MaxNormalDifference)
	{
		VOXEL_USE_NAMESPACE(MetaGraph);

		FindVoxelQueryData(FVoxelPositionQueryData, PositionQueryData);
		CheckVoxelBuffersNum(Normals, PositionQueryData->GetPositions());

		const FRDGTextureRef Texture = GraphBuilder.CreateTexture(
			FRDGTextureDesc::Create2D(
				FIntPoint(DetailTextureQueryData->AtlasTextureSize, DetailTextureQueryData->AtlasTextureSize),
				PF_R16_UINT,
				FClearValueBinding::Black,
				TexCreate_ShaderResource | TexCreate_UAV),
			TEXT("NormalTexture"));

		BEGIN_VOXEL_SHADER_CALL(MakeNormalDetailTexture)
		{
			Parameters.TextureSize = DetailTextureQueryData->TextureSize;
			Parameters.NumCells = DetailTextureQueryData->NumCells;
			Parameters.NumCellsPerSide = DetailTextureQueryData->NumCellsPerSide;

			Parameters.ConstantNormals.X = Normals.X.IsConstant();
			Parameters.ConstantNormals.Y = Normals.Y.IsConstant();
			Parameters.ConstantNormals.Z = Normals.Z.IsConstant();

			Parameters.NormalsX = Normals.X.GetGpuBuffer();
			Parameters.NormalsY = Normals.Y.GetGpuBuffer();
			Parameters.NormalsZ = Normals.Z.GetGpuBuffer();

			Parameters.Texture = GraphBuilder.CreateUAV(Texture);

			Execute(FComputeShaderUtils::GetGroupCount(
				FIntVector(
					DetailTextureQueryData->TextureSize,
					DetailTextureQueryData->TextureSize,
					DetailTextureQueryData->NumCells),
				4));
		}
		END_VOXEL_SHADER_CALL()

		const TSharedRef<TRefCountPtr<IPooledRenderTarget>> ExtractedTexture = MakeShared<TRefCountPtr<IPooledRenderTarget>>();
		GraphBuilder.QueueTextureExtraction(Texture, &ExtractedTexture.Get(), ERHIAccess::SRVGraphics);
		FVoxelRenderUtilities::KeepAlive(GraphBuilder, ExtractedTexture);

		const TSharedRef<FVoxelDetailTexture> DetailTexture = MakeShared<FVoxelDetailTexture>();
		DetailTexture->Name = Name;
		DetailTexture->SizeX = DetailTextureQueryData->AtlasTextureSize;
		DetailTexture->SizeY = DetailTextureQueryData->AtlasTextureSize;
		DetailTexture->Format = PF_R16_UINT;
		DetailTexture->GpuTexture = ExtractedTexture;
		return DetailTexture;
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE_CPU(FVoxelNode_MakeColorDetailTexture, DetailTexture)
{
	FindVoxelQueryData(FVoxelDetailTextureQueryData, DetailTextureQueryData);

	const TValue<TBufferView<FLinearColor>> Colors = GetBufferView(ColorPin, Query);
	const TValue<FName> Name = Get(NamePin, Query);
	
	return VOXEL_ON_COMPLETE(AsyncThread, DetailTextureQueryData, Colors, Name)
	{
		FindVoxelQueryData(FVoxelPositionQueryData, PositionQueryData);
		CheckVoxelBuffersNum(Colors, PositionQueryData->GetPositions());

		const TSharedRef<FVoxelDetailTexture> DetailTexture = MakeShared<FVoxelDetailTexture>();
		DetailTexture->Name = Name;
		DetailTexture->SizeX = DetailTextureQueryData->AtlasTextureSize;
		DetailTexture->SizeY = DetailTextureQueryData->AtlasTextureSize;
		DetailTexture->Format = PF_R8G8B8A8;
		DetailTexture->SetCpuData(*DetailTextureQueryData, [&](TVoxelArray<uint8>& Data, int32 QueryIndex, int32 DataIndex, int32 CellIndex)
		{
			const FColor Color = Colors[QueryIndex].ToFColor(false);

			Data[4 * DataIndex + 0] = Color.R;
			Data[4 * DataIndex + 1] = Color.G;
			Data[4 * DataIndex + 2] = Color.B;
			Data[4 * DataIndex + 3] = Color.A;
		});
		
		return DetailTexture;
	};
}

BEGIN_VOXEL_NAMESPACE(MetaGraph)

BEGIN_VOXEL_COMPUTE_SHADER(MakeColorDetailTexture)
	VOXEL_SHADER_PARAMETER_CST(int32, TextureSize)
	VOXEL_SHADER_PARAMETER_CST(int32, NumCells)
	VOXEL_SHADER_PARAMETER_CST(int32, NumCellsPerSide)

	VOXEL_SHADER_PARAMETER_CST(FUintVector4, ConstantColors)

	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, ColorsR)
	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, ColorsG)
	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, ColorsB)
	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, ColorsA)
	
	VOXEL_SHADER_PARAMETER_TEXTURE_UAV(Texture2D<float2>, Texture)
END_VOXEL_SHADER()

END_VOXEL_NAMESPACE(MetaGraph)

DEFINE_VOXEL_NODE_GPU(FVoxelNode_MakeColorDetailTexture, DetailTexture)
{
	FindVoxelQueryData(FVoxelDetailTextureQueryData, DetailTextureQueryData);

	const TValue<FVoxelLinearColorBuffer> Colors = Get(ColorPin, Query);
	const TValue<FName> Name = Get(NamePin, Query);

	return VOXEL_ON_COMPLETE(RenderThread, DetailTextureQueryData, Colors, Name)
	{
		VOXEL_USE_NAMESPACE(MetaGraph);

		FindVoxelQueryData(FVoxelPositionQueryData, PositionQueryData);
		CheckVoxelBuffersNum(Colors, PositionQueryData->GetPositions());

		const FRDGTextureRef Texture = GraphBuilder.CreateTexture(
			FRDGTextureDesc::Create2D(
				FIntPoint(DetailTextureQueryData->AtlasTextureSize, DetailTextureQueryData->AtlasTextureSize),
				PF_R8G8B8A8,
				FClearValueBinding::Black,
				TexCreate_ShaderResource | TexCreate_UAV),
			TEXT("ColorTexture"));

		BEGIN_VOXEL_SHADER_CALL(MakeColorDetailTexture)
		{
			Parameters.TextureSize = DetailTextureQueryData->TextureSize;
			Parameters.NumCells = DetailTextureQueryData->NumCells;
			Parameters.NumCellsPerSide = DetailTextureQueryData->NumCellsPerSide;

			Parameters.ConstantColors.X = Colors.R.IsConstant();
			Parameters.ConstantColors.Y = Colors.G.IsConstant();
			Parameters.ConstantColors.Z = Colors.B.IsConstant();
			Parameters.ConstantColors.W = Colors.A.IsConstant();

			Parameters.ColorsR = Colors.R.GetGpuBuffer();
			Parameters.ColorsG = Colors.G.GetGpuBuffer();
			Parameters.ColorsB = Colors.B.GetGpuBuffer();
			Parameters.ColorsA = Colors.A.GetGpuBuffer();

			Parameters.Texture = GraphBuilder.CreateUAV(Texture);

			Execute(FComputeShaderUtils::GetGroupCount(
				FIntVector(
					DetailTextureQueryData->TextureSize,
					DetailTextureQueryData->TextureSize,
					DetailTextureQueryData->NumCells),
				4));
		}
		END_VOXEL_SHADER_CALL()

		const TSharedRef<TRefCountPtr<IPooledRenderTarget>> ExtractedTexture = MakeShared<TRefCountPtr<IPooledRenderTarget>>();
		GraphBuilder.QueueTextureExtraction(Texture, &ExtractedTexture.Get(), ERHIAccess::SRVGraphics);
		FVoxelRenderUtilities::KeepAlive(GraphBuilder, ExtractedTexture);

		const TSharedRef<FVoxelDetailTexture> DetailTexture = MakeShared<FVoxelDetailTexture>();
		DetailTexture->Name = Name;
		DetailTexture->SizeX = DetailTextureQueryData->AtlasTextureSize;
		DetailTexture->SizeY = DetailTextureQueryData->AtlasTextureSize;
		DetailTexture->Format = PF_R8G8B8A8;
		DetailTexture->GpuTexture = ExtractedTexture;
		return DetailTexture;
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE_CPU(FVoxelNode_CreateDetailTextureMaterial_Internal, Material)
{
	FindVoxelQueryData(FVoxelBoundsQueryData, BoundsQueryData);
	FindVoxelQueryData(FVoxelLODQueryData, LODQueryData);
	FindVoxelQueryData(FVoxelSurfaceQueryData, SurfaceQueryData);

	const TValue<int32> TextureSizeBias = Get(TextureSizeBiasPin, Query);
	const TValue<FVoxelMeshMaterial> FallbackMaterial = Get(FallbackMaterialPin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, LODQueryData, SurfaceQueryData, TextureSizeBias, FallbackMaterial)
	{
		if (!SurfaceQueryData->Surface)
		{
			return FallbackMaterial;
		}
		if (!SurfaceQueryData->Surface->IsA<FVoxelMarchingCubeSurface>())
		{
			VOXEL_MESSAGE(Error, "{0}: Surface needs to be a Marching Cube Surface", this);
			return FallbackMaterial;
		}
		const TSharedRef<const FVoxelMarchingCubeSurface> Surface = CastChecked<FVoxelMarchingCubeSurface>(SurfaceQueryData->Surface.ToSharedRef());

		const TValue<TBufferView<FVoxelInt4>> SurfaceCells = Surface->Cells.MakeView();
		const TValue<TBufferView<int32>> SurfaceIndices = Surface->Indices.MakeView();
		const TValue<TBufferView<FVector>> SurfaceVertices = Surface->Vertices.MakeView();

		return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, LODQueryData, SurfaceQueryData, TextureSizeBias, FallbackMaterial, Surface, SurfaceCells, SurfaceIndices, SurfaceVertices)
		{
			const int32 TextureSize = GetTextureSize(LODQueryData->LOD, TextureSizeBias);
			if (TextureSize == -1)
			{
				return FallbackMaterial;
			}

			const int32 ChunkSize = Surface->ChunkSize;
			const float VoxelSize = Surface->ScaledVoxelSize;

			if (SurfaceCells.Num() > MAX_uint16)
			{
				VOXEL_MESSAGE(Error, "{0}: Too many vertices for detail textures, please reduce your chunk size", this);
				return FallbackMaterial;
			}

			const int32 NumCells = SurfaceCells.Num();
			const int32 NumCellsPerSide = FMath::CeilToInt(FMath::Sqrt(double(NumCells)));
			const int32 AtlasTextureSize = NumCellsPerSide * TextureSize;
			
			FVoxelQuery DetailTextureQuery = Query;
			DetailTextureQuery.Add<FVoxelGradientStepQueryData>().Step = VoxelSize / TextureSize;

			TVoxelArray<float> NormalsX = FVoxelFloatBuffer::Allocate(NumCells);
			TVoxelArray<float> NormalsY = FVoxelFloatBuffer::Allocate(NumCells);
			TVoxelArray<float> NormalsZ = FVoxelFloatBuffer::Allocate(NumCells);

			TVoxelArray<uint16> Indirection;
			FVoxelUtilities::SetNumFast(Indirection, FMath::Cube(ChunkSize));
			FVoxelUtilities::SetAll(Indirection, MAX_uint16);

			{
				VOXEL_SCOPE_COUNTER("Trace rays");

				TVoxelArray<float> QueryX = FVoxelFloatBuffer::Allocate(TextureSize * TextureSize * NumCells);
				TVoxelArray<float> QueryY = FVoxelFloatBuffer::Allocate(TextureSize * TextureSize * NumCells);
				TVoxelArray<float> QueryZ = FVoxelFloatBuffer::Allocate(TextureSize * TextureSize * NumCells);
				int32 QueryIndex = 0;

				for (int32 CellIndex = 0; CellIndex < NumCells; CellIndex++)
				{
					const FVoxelInt4 Cell = SurfaceCells[CellIndex];
					const FIntVector CellPosition = FIntVector(Cell.X, Cell.Y, Cell.Z);
					const int32 NumTriangles = Cell.W & 0xFF;
					const int32 FirstTriangle = Cell.W >> 8;

					constexpr int32 MaxTrianglesPerCell = 5;
					checkVoxelSlow(NumTriangles <= MaxTrianglesPerCell);

					TVoxelArray<FVector3f, TFixedAllocator<MaxTrianglesPerCell * 3 * 2>> Vertices;

					FVector3f Normal = FVector3f::ZeroVector;
					for (int32 TriangleIndex = 0; TriangleIndex < NumTriangles; TriangleIndex++)
					{
						const FVector3f PositionA = SurfaceVertices[SurfaceIndices[3 * (FirstTriangle + TriangleIndex) + 0]];
						const FVector3f PositionB = SurfaceVertices[SurfaceIndices[3 * (FirstTriangle + TriangleIndex) + 1]];
						const FVector3f PositionC = SurfaceVertices[SurfaceIndices[3 * (FirstTriangle + TriangleIndex) + 2]];

						Normal += FVoxelUtilities::GetTriangleNormal(PositionA, PositionB, PositionC);

						Vertices.Add(PositionA);
						Vertices.Add(PositionB);
						Vertices.Add(PositionC);

						// Add bigger triangles to ensure all rays hit
						const FVector3f Centroid = (PositionA + PositionB + PositionC) / 3.f;
						Vertices.Add(Centroid + 10 * (PositionA - Centroid).GetSafeNormal());
						Vertices.Add(Centroid + 10 * (PositionB - Centroid).GetSafeNormal());
						Vertices.Add(Centroid + 10 * (PositionC - Centroid).GetSafeNormal());
					}
					Normal.Normalize();

					NormalsX[CellIndex] = Normal.X;
					NormalsY[CellIndex] = Normal.Y;
					NormalsZ[CellIndex] = Normal.Z;

					int32 Direction;
					{
						const FVector3f Abs = Normal.GetAbs();
						if (Abs.X >= Abs.Y && Abs.X >= Abs.Z)
						{
							Direction = 0;
						}
						else if (Abs.Y >= Abs.X && Abs.Y >= Abs.Z)
						{
							Direction = 1;
						}
						else
						{
							ensureVoxelSlow(Abs.Z >= Abs.X && Abs.Z >= Abs.Y);
							Direction = 2;
						}
					}

					// Write indirection
					{
						constexpr int32 MaxCellIndex = MAX_uint16 >> 2;
						ensure(CellIndex < MaxCellIndex);

						const uint32 Value = uint32(CellIndex) | (uint32(Direction) << 14);
						ensureVoxelSlow(Value < MAX_uint16);
						Indirection[FVoxelUtilities::Get3DIndex<int32>(ChunkSize, CellPosition)] = Value;
					}

					FVector3f RayDirection = FVector3f(ForceInit);
					RayDirection[Direction] = 1;

					for (int32 Y = 0; Y < TextureSize; Y++)
					{
						for (int32 X = 0; X < TextureSize; X++)
						{
							FVector3f Offset;
							switch (Direction)
							{
							default: VOXEL_ASSUME(false);
							case 0: Offset = FVector3f(0, X, Y); break;
							case 1: Offset = FVector3f(X, 0, Y); break;
							case 2: Offset = FVector3f(X, Y, 0); break;
							}

							Offset = (Offset * (TextureSize + 1) / TextureSize - 0.5f);
							Offset[Direction] = 0;

							const FVector3f RayOrigin = FVector3f(CellPosition) + Offset / TextureSize;

							FVector3f HitPosition = RayOrigin;

							checkVoxelSlow(Vertices.Num() % 3 == 0);
							for (int32 Index = 0; Index < Vertices.Num(); Index += 3)
							{
								float Time;
								if (!FVoxelUtilities::RayTriangleIntersection(
									RayOrigin,
									RayDirection,
									Vertices[Index + 0],
									Vertices[Index + 1],
									Vertices[Index + 2],
									true,
									Time))
								{
									continue;
								}

								HitPosition = RayOrigin + Time * RayDirection;
								break;
							}

							HitPosition *= VoxelSize;
							HitPosition += FVector3f(BoundsQueryData->Bounds.Min);

							QueryX[QueryIndex] = HitPosition.X;
							QueryY[QueryIndex] = HitPosition.Y;
							QueryZ[QueryIndex] = HitPosition.Z;
							QueryIndex++;
						}
					}
				}
				ensure(QueryIndex == TextureSize * TextureSize * NumCells);

				FVoxelVectorBuffer Positions = FVoxelVectorBuffer::MakeCpu(QueryX, QueryY, QueryZ);
				Positions.SetBounds(BoundsQueryData->Bounds);
				DetailTextureQuery.Add<FVoxelSparsePositionQueryData>().Initialize(Positions);
			}

			{
				VOXEL_SCOPE_COUNTER("Fixup indirection");

				for (int32 X = 0; X < ChunkSize; X++)
				{
					for (int32 Y = 0; Y < ChunkSize; Y++)
					{
						for (int32 Z = 0; Z < ChunkSize; Z++)
						{
							uint16& Value = Indirection[FVoxelUtilities::Get3DIndex<int32>(ChunkSize, X, Y, Z)];
							if (Value != MAX_uint16)
							{
								continue;
							}

#define CHECK(DX, DY, DZ) \
							if ((DX != -1 || X != 0) && \
								(DX != +1 || X != ChunkSize - 1) && \
								(DY != -1 || Y != 0) && \
								(DY != +1 || Y != ChunkSize - 1) && \
								(DZ != -1 || Z != 0) && \
								(DZ != +1 || Z != ChunkSize - 1)) \
							{ \
								const uint16 NeighborValue = Indirection[FVoxelUtilities::Get3DIndex<int32>(ChunkSize, X + DX, Y + DY, Z + DZ)]; \
								if (NeighborValue != MAX_uint16) \
								{ \
									Value = NeighborValue; \
									continue; \
								} \
							}

							CHECK(0, 0, -1);
							CHECK(0, 0, +1);

							CHECK(-1, 0, 0);
							CHECK(+1, 0, 0);

							CHECK(0, -1, 0);
							CHECK(0, +1, 0);

#undef CHECK
						}
					}
				}
			}

			{
				FVoxelDetailTextureQueryData& QueryData = DetailTextureQuery.Add<FVoxelDetailTextureQueryData>();
				QueryData.NumCells = NumCells;
				QueryData.TextureSize = TextureSize;
				QueryData.NumCellsPerSide = NumCellsPerSide;
				QueryData.AtlasTextureSize = AtlasTextureSize;
				QueryData.Normals = FVoxelVectorBuffer::MakeCpu(NormalsX, NormalsY, NormalsZ);
			}

			const TValue<FVoxelNode_CreateDetailTextureMaterial_Internal_Input> Input = Get(InputPin, DetailTextureQuery);

			const TSharedRef<FVoxelDetailTexture> IndirectionTexture = MakeShared<FVoxelDetailTexture>();
			IndirectionTexture->Name = STATIC_FNAME("Indirection");
			IndirectionTexture->SizeX = ChunkSize;
			IndirectionTexture->SizeY = FMath::Square(ChunkSize);
			IndirectionTexture->Format = PF_R16_UINT;
			IndirectionTexture->CpuData = MakeSharedCopy(ReinterpretCastVoxelArray_Copy<uint8>(Indirection));

			const TSharedRef<FVoxelMeshMaterialInstance> Instance = MakeShared<FVoxelMeshMaterialInstance>();
			Instance->Parameters.ScalarParameters.Add(STATIC_FNAME("VoxelDetailTextures_Enable"), 1);
			Instance->Parameters.ScalarParameters.Add(STATIC_FNAME("VoxelDetailTextures_VoxelSize"), VoxelSize);
			Instance->Parameters.ScalarParameters.Add(STATIC_FNAME("VoxelDetailTextures_ChunkSize"), ChunkSize);
			Instance->Parameters.ScalarParameters.Add(STATIC_FNAME("VoxelDetailTextures_NumCellsPerSide"), NumCellsPerSide);
			Instance->Parameters.ScalarParameters.Add(STATIC_FNAME("VoxelDetailTextures_TextureSize"), TextureSize);
			Instance->Parameters.ScalarParameters.Add(STATIC_FNAME("VoxelDetailTextures_AtlasTextureSize"), AtlasTextureSize);
			
			return VOXEL_ON_COMPLETE(GameThread, Input, IndirectionTexture, Instance)
			{
				struct FTexture
				{
					TSharedPtr<const FVoxelDetailTexture> DetailTexture;
					TSharedPtr<FVoxelTextureRef> TexturePoolRef;
					TRefCountPtr_RenderThread<FRHITexture2D> RHI;
				};
				const TSharedRef<TArray<FTexture>> Textures = MakeShared<TArray<FTexture>>();

				TArray<TSharedRef<const FVoxelDetailTexture>> DetailTextures = Input->DetailTextures;
				DetailTextures.Add(IndirectionTexture);

				Instance->Parent = Input->Material;

				for (const TSharedRef<const FVoxelDetailTexture>& DetailTexture : DetailTextures)
				{
					if (!DetailTexture->CpuData.IsValid())
					{
						continue;
					}

					const TSharedRef<FVoxelTextureRef> TexturePoolRef = FVoxelTextureRef::Make(
						"Detail Texture " + DetailTexture->Name,
						FVoxelTextureKey(
							EVoxelTextureType::Texture2D,
							DetailTexture->SizeX,
							DetailTexture->SizeY,
							DetailTexture->Format));

					Textures->Add(
					{
						DetailTexture,
						TexturePoolRef
					});
				}

				check(GRHISupportsAsyncTextureCreation);

				return VOXEL_ON_COMPLETE(AsyncThread, Instance, Textures)
				{
					VOXEL_SCOPE_COUNTER("RHIAsyncCreateTexture2D");

					for (FTexture& Texture : *Textures)
					{
						TArray<void*> MipData;
						MipData.Add(VOXEL_CONST_CAST(Texture.DetailTexture->CpuData->GetData()));

						Texture.RHI = RHIAsyncCreateTexture2D(
							Texture.DetailTexture->SizeX,
							Texture.DetailTexture->SizeY,
							Texture.DetailTexture->Format,
							1,
							TexCreate_ShaderResource,
							MipData.GetData(),
							1);
					}
					
					return VOXEL_ON_COMPLETE(RenderThread, Instance, Textures)
					{
						for (const FTexture& Texture : *Textures)
						{
							Instance->Parameters.Resources.Add(Texture.TexturePoolRef);
							Instance->Parameters.TextureParameters.Add("VoxelDetailTextures_" + Texture.DetailTexture->Name + "Texture", Texture.TexturePoolRef->Get());

							FVoxelRenderUtilities::UpdateTextureRef(
								Texture.TexturePoolRef->Get<UTexture2D>(),
								Texture.RHI);
						}

						return Instance;
					};
				};
			};
		};
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

BEGIN_VOXEL_NAMESPACE(MetaGraph)

BEGIN_VOXEL_COMPUTE_SHADER(MakeDetailTextureData)
	VOXEL_SHADER_PARAMETER_CST(int32, TextureSize)
	VOXEL_SHADER_PARAMETER_CST(int32, ChunkSize)
	VOXEL_SHADER_PARAMETER_CST(float, VoxelSize)
	VOXEL_SHADER_PARAMETER_CST(int32, NumCells)
	VOXEL_SHADER_PARAMETER_CST(FVector3f, PositionOffset)

	VOXEL_SHADER_PARAMETER_SRV(Buffer<uint4>, SurfaceCells)
	VOXEL_SHADER_PARAMETER_SRV(Buffer<uint>, SurfaceIndices)
	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, SurfaceVerticesX)
	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, SurfaceVerticesY)
	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, SurfaceVerticesZ)

	VOXEL_SHADER_PARAMETER_UAV(Buffer<float>, NormalsX)
	VOXEL_SHADER_PARAMETER_UAV(Buffer<float>, NormalsY)
	VOXEL_SHADER_PARAMETER_UAV(Buffer<float>, NormalsZ)
	VOXEL_SHADER_PARAMETER_UAV(Buffer<float>, QueryPositionsX)
	VOXEL_SHADER_PARAMETER_UAV(Buffer<float>, QueryPositionsY)
	VOXEL_SHADER_PARAMETER_UAV(Buffer<float>, QueryPositionsZ)
	VOXEL_SHADER_PARAMETER_TEXTURE_UAV(Texture2D<uint>, IndirectionTexture)
END_VOXEL_SHADER()

END_VOXEL_NAMESPACE(MetaGraph)

DEFINE_VOXEL_NODE_GPU(FVoxelNode_CreateDetailTextureMaterial_Internal, Material)
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	FindVoxelQueryData(FVoxelBoundsQueryData, BoundsQueryData);
	FindVoxelQueryData(FVoxelLODQueryData, LODQueryData);
	FindVoxelQueryData(FVoxelSurfaceQueryData, SurfaceQueryData);

	const TValue<int32> TextureSizeBias = Get(TextureSizeBiasPin, Query);
	const TValue<FVoxelMeshMaterial> FallbackMaterial = Get(FallbackMaterialPin, Query);

	return VOXEL_ON_COMPLETE(RenderThread, BoundsQueryData, LODQueryData, SurfaceQueryData, TextureSizeBias, FallbackMaterial)
	{
		if (!SurfaceQueryData->Surface)
		{
			return FallbackMaterial;
		}
		if (!SurfaceQueryData->Surface->IsA<FVoxelMarchingCubeSurface>())
		{
			VOXEL_MESSAGE(Error, "{0}: Surface needs to be a Marching Cube Surface", this);
			return FallbackMaterial;
		}
		const TSharedRef<const FVoxelMarchingCubeSurface> Surface = CastChecked<FVoxelMarchingCubeSurface>(SurfaceQueryData->Surface.ToSharedRef());

		const int32 TextureSize = GetTextureSize(LODQueryData->LOD, TextureSizeBias);
		if (TextureSize == -1)
		{
			return FallbackMaterial;
		}

		const int32 NumCells = Surface->Cells.Num();
		const int32 ChunkSize = Surface->ChunkSize;
		const float VoxelSize = Surface->ScaledVoxelSize;
		const int32 NumCellsPerSide = FMath::CeilToInt(FMath::Sqrt(double(NumCells)));
		const int32 AtlasTextureSize = NumCellsPerSide * TextureSize;

		if (NumCells > MAX_uint16)
		{
			VOXEL_MESSAGE(Error, "{0}: Too many vertices for detail textures, please reduce your chunk size", this);
			return FallbackMaterial;
		}

		const int32 NumPositions = TextureSize * TextureSize * NumCells;

		FVoxelVectorBuffer Normals;
		Normals.X = FVoxelFloatBuffer::MakeGpu(NumCells);
		Normals.Y = FVoxelFloatBuffer::MakeGpu(NumCells);
		Normals.Z = FVoxelFloatBuffer::MakeGpu(NumCells);

		FVoxelVectorBuffer QueryPositions;
		QueryPositions.X = FVoxelFloatBuffer::MakeGpu(NumPositions);
		QueryPositions.Y = FVoxelFloatBuffer::MakeGpu(NumPositions);
		QueryPositions.Z = FVoxelFloatBuffer::MakeGpu(NumPositions);

		QueryPositions.SetBounds(BoundsQueryData->Bounds);

		const FRDGTextureRef IndirectionTextureRef = GraphBuilder.CreateTexture(
			FRDGTextureDesc::Create2D(
				FIntPoint(ChunkSize, FMath::Square(ChunkSize)),
				PF_R16_UINT,
				FClearValueBinding::Black,
				TexCreate_ShaderResource | TexCreate_UAV),
			TEXT("IndirectionTexture"));

		BEGIN_VOXEL_SHADER_CALL(MakeDetailTextureData)
		{
			Parameters.TextureSize = TextureSize;
			Parameters.ChunkSize = ChunkSize;
			Parameters.VoxelSize = VoxelSize;
			Parameters.NumCells = NumCells;
			Parameters.PositionOffset = FVector3f(BoundsQueryData->Bounds.Min);

			Parameters.SurfaceCells = Surface->Cells.GetGpuBuffer();
			Parameters.SurfaceIndices = Surface->Indices.GetGpuBuffer();
			Parameters.SurfaceVerticesX = Surface->Vertices.X.GetGpuBuffer();
			Parameters.SurfaceVerticesY = Surface->Vertices.Y.GetGpuBuffer();
			Parameters.SurfaceVerticesZ = Surface->Vertices.Z.GetGpuBuffer();

			Parameters.NormalsX = Normals.X.GetGpuBuffer();
			Parameters.NormalsY = Normals.Y.GetGpuBuffer();
			Parameters.NormalsZ = Normals.Z.GetGpuBuffer();
			Parameters.QueryPositionsX = QueryPositions.X.GetGpuBuffer();
			Parameters.QueryPositionsY = QueryPositions.Y.GetGpuBuffer();
			Parameters.QueryPositionsZ = QueryPositions.Z.GetGpuBuffer();
			Parameters.IndirectionTexture = GraphBuilder.CreateUAV(IndirectionTextureRef);

			Execute(FComputeShaderUtils::GetGroupCount(NumCells, 64));
		}
		END_VOXEL_SHADER_CALL()

		const TSharedRef<TRefCountPtr<IPooledRenderTarget>> IndirectionGpuTexture = MakeShared<TRefCountPtr<IPooledRenderTarget>>();
		GraphBuilder.QueueTextureExtraction(IndirectionTextureRef, &IndirectionGpuTexture.Get(), ERHIAccess::SRVGraphics);
		FVoxelRenderUtilities::KeepAlive(GraphBuilder, IndirectionGpuTexture);

		FVoxelQuery DetailTextureQuery = Query;
		DetailTextureQuery.Add<FVoxelGradientStepQueryData>().Step = VoxelSize / TextureSize;
		DetailTextureQuery.Add<FVoxelSparsePositionQueryData>().Initialize(QueryPositions);
		{
			FVoxelDetailTextureQueryData& QueryData = DetailTextureQuery.Add<FVoxelDetailTextureQueryData>();
			QueryData.NumCells = NumCells;
			QueryData.TextureSize = TextureSize;
			QueryData.NumCellsPerSide = NumCellsPerSide;
			QueryData.AtlasTextureSize = AtlasTextureSize;
			QueryData.Normals = Normals;
		}

		const TValue<FVoxelNode_CreateDetailTextureMaterial_Internal_Input> Input = Get(InputPin, DetailTextureQuery);

		const TSharedRef<FVoxelDetailTexture> IndirectionTexture = MakeShared<FVoxelDetailTexture>();
		IndirectionTexture->Name = STATIC_FNAME("Indirection");
		IndirectionTexture->SizeX = ChunkSize;
		IndirectionTexture->SizeY = FMath::Square(ChunkSize);
		IndirectionTexture->Format = PF_R16_UINT;
		IndirectionTexture->GpuTexture = IndirectionGpuTexture;

		const TSharedRef<FVoxelMeshMaterialInstance> Instance = MakeShared<FVoxelMeshMaterialInstance>();
		Instance->Parameters.ScalarParameters.Add(STATIC_FNAME("VoxelDetailTextures_Enable"), 1);
		Instance->Parameters.ScalarParameters.Add(STATIC_FNAME("VoxelDetailTextures_VoxelSize"), VoxelSize);
		Instance->Parameters.ScalarParameters.Add(STATIC_FNAME("VoxelDetailTextures_ChunkSize"), ChunkSize);
		Instance->Parameters.ScalarParameters.Add(STATIC_FNAME("VoxelDetailTextures_NumCellsPerSide"), NumCellsPerSide);
		Instance->Parameters.ScalarParameters.Add(STATIC_FNAME("VoxelDetailTextures_TextureSize"), TextureSize);
		Instance->Parameters.ScalarParameters.Add(STATIC_FNAME("VoxelDetailTextures_AtlasTextureSize"), AtlasTextureSize);

		return VOXEL_ON_COMPLETE(GameThread, Input, IndirectionTexture, Instance)
		{
			struct FTextureToUpdate
			{
				TSharedPtr<const TRefCountPtr<IPooledRenderTarget>> RenderTarget;
				TSharedPtr<FVoxelTextureRef> TextureRef;
			};
			TArray<TSharedPtr<FTextureToUpdate>> TexturesToUpdate;

			TArray<TSharedRef<const FVoxelDetailTexture>> DetailTextures = Input->DetailTextures;
			DetailTextures.Add(IndirectionTexture);

			Instance->Parent = Input->Material;

			for (const TSharedRef<const FVoxelDetailTexture>& DetailTexture : DetailTextures)
			{
				if (!DetailTexture->GpuTexture.IsValid())
				{
					continue;
				}

				const TSharedRef<FVoxelTextureRef> TexturePoolRef = FVoxelTextureRef::Make(
					"Detail Texture " + DetailTexture->Name,
					FVoxelTextureKey(
						EVoxelTextureType::GpuTexture2D,
						DetailTexture->SizeX,
						DetailTexture->SizeY,
						DetailTexture->Format));

				Instance->Parameters.Resources.Add(TexturePoolRef);
				Instance->Parameters.TextureParameters.Add("VoxelDetailTextures_" + DetailTexture->Name + "Texture", TexturePoolRef->Get());

				TexturesToUpdate.Add(MakeSharedCopy(FTextureToUpdate{ DetailTexture->GpuTexture, TexturePoolRef }));
			}
			
			return VOXEL_ON_COMPLETE(RenderThread, Instance, TexturesToUpdate)
			{
				for (const TSharedPtr<FTextureToUpdate>& TextureToUpdate : TexturesToUpdate)
				{
					if (!ensure(*TextureToUpdate->RenderTarget))
					{
						continue;
					}

					UVoxelGpuTexture* Texture = TextureToUpdate->TextureRef->Get<UVoxelGpuTexture>();
					if (!ensure(Texture))
					{
						continue;
					}

					Texture->Update_RenderThread(*TextureToUpdate->RenderTarget);
				}
				return Instance;
			};
		};
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int32 FVoxelNode_CreateDetailTextureMaterial_Internal::GetTextureSize(int32 LOD, int32 TextureSizeBias) const
{
	int32 TextureSize = 1 << FMath::Clamp(LOD, 0, 31);
	TextureSize = FMath::Clamp(TextureSize, 0, 8);
	TextureSize += TextureSizeBias;

	if (TextureSize < 2)
	{
		return -1;
	}

	// 2x2 isn't enough with the padding pixel
	if (TextureSize == 2 ||
		TextureSize == 3)
	{
		TextureSize = 4;
	}

	if (TextureSize > 128)
	{
		VOXEL_MESSAGE(Error, "{0}: invalid detail texture size ({1})", this, TextureSize);
		return -1;
	}

	return TextureSize;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_CreateDetailTextureMaterial, Material)
{
	const TValue<FVoxelMeshMaterial> BaseMaterial = Get(BaseMaterialPin, Query);

	return VOXEL_CALL_NODE(FVoxelNode_CreateDetailTextureMaterial_Internal, MaterialPin)
	{
		VOXEL_CALL_NODE_BIND(TextureSizeBiasPin)
		{
			return Get(TextureSizeBiasPin, Query);
		};
		VOXEL_CALL_NODE_BIND(FallbackMaterialPin, BaseMaterial)
		{
			return BaseMaterial;
		};
		VOXEL_CALL_NODE_BIND(InputPin, BaseMaterial)
		{
			const TArray<TValue<FVoxelDetailTexture>> DetailTextures = Get(DetailTexturesPins, Query);

			return VOXEL_ON_COMPLETE(AnyThread, BaseMaterial, DetailTextures)
			{
				const TSharedRef<FVoxelNode_CreateDetailTextureMaterial_Internal_Input> Input = MakeShared<FVoxelNode_CreateDetailTextureMaterial_Internal_Input>();
				Input->Material = BaseMaterial;
				Input->DetailTextures = DetailTextures;
				return Input;
			};
		};
	};
}