// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/MarchingCube/VoxelMarchingCubeNodes.h"
#include "Nodes/MarchingCube/VoxelMarchingCubeProcessor.h"
#include "Nodes/MarchingCube/VoxelMarchingCubeMesh_LocalVF.h"
#include "Nodes/VoxelPositionNodes.h"
#include "VoxelMetaGraphRuntimeUtilities.h"
#include "VoxelCollision/VoxelCollisionCooker.h"
#include "VoxelCollision/VoxelTriangleMeshCollider.h"
#include "Transvoxel.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

BEGIN_VOXEL_COMPUTE_SHADER(UnpackDensities)
	VOXEL_SHADER_PARAMETER_CST(FIntVector, BlockSize)
	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, InDensities)
	VOXEL_SHADER_PARAMETER_UAV(Buffer<float>, OutDensities)
END_VOXEL_SHADER()

BEGIN_VOXEL_SHADER_PERMUTATION_DOMAIN(GenerateMarchingCubeMesh)
{
	class FComputeNums : SHADER_PERMUTATION_BOOL("COMPUTE_NUMS");
}
END_VOXEL_SHADER_PERMUTATION_DOMAIN(GenerateMarchingCubeMesh, FComputeNums)

BEGIN_VOXEL_COMPUTE_SHADER(GenerateMarchingCubeMesh)
	VOXEL_SHADER_ALL_PARAMETERS_ARE_OPTIONAL()

	VOXEL_SHADER_PARAMETER_CST(int32, ChunkSize)
	VOXEL_SHADER_PARAMETER_CST(int32, DataSize)
	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, Densities)
	
	VOXEL_SHADER_PARAMETER_UAV(Buffer<uint4>, OutCells)
	VOXEL_SHADER_PARAMETER_UAV(Buffer<uint>, OutIndices)
	VOXEL_SHADER_PARAMETER_UAV(Buffer<float>, OutVerticesX)
	VOXEL_SHADER_PARAMETER_UAV(Buffer<float>, OutVerticesY)
	VOXEL_SHADER_PARAMETER_UAV(Buffer<float>, OutVerticesZ)

	VOXEL_SHADER_PARAMETER_UAV(Buffer<uint>, Nums)

	VOXEL_SHADER_PARAMETER_SRV(Buffer<uint>, RegularCellClass)
	VOXEL_SHADER_PARAMETER_SRV(Buffer<uint>, RegularCellData)
	VOXEL_SHADER_PARAMETER_SRV(Buffer<uint>, RegularVertexData)
END_VOXEL_SHADER()

END_VOXEL_NAMESPACE(MetaGraph)

DEFINE_VOXEL_NODE(FVoxelNode_FVoxelMarchingCubeSurface_GenerateSurface, Surface)
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	FindVoxelQueryData(FVoxelBoundsQueryData, BoundsQueryData);
	FindVoxelQueryData(FVoxelLODQueryData, LODQueryData);

	const TValue<float> VoxelSize = Get(VoxelSizePin, Query);
	const TValue<bool> EnableDistanceChecks = Get(EnableDistanceChecksPin, Query);
	const TValue<float> DistanceChecksTolerance = Get(DistanceChecksTolerancePin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, LODQueryData, VoxelSize, EnableDistanceChecks, DistanceChecksTolerance)
	{
		const FVoxelBox Bounds = BoundsQueryData->Bounds;
		const int32 LOD = LODQueryData->LOD;

		const float ScaledVoxelSize = VoxelSize * (1 << LOD);

		if (!Bounds.Min.Equals(2 * ScaledVoxelSize * FVector3d(FVoxelUtilities::RoundToInt(Bounds.Min / (2 * ScaledVoxelSize)))) ||
			!Bounds.Max.Equals(2 * ScaledVoxelSize * FVector3d(FVoxelUtilities::RoundToInt(Bounds.Max / (2 * ScaledVoxelSize)))))
		{
			const FVector ChunkSize = Bounds.Size() / (1 << LOD);
			ensure(ChunkSize == FVector(ChunkSize.X));

			VOXEL_MESSAGE(Error,
				"{0}: Chunk Size should be a multiple of 2x Voxel Size. Chunk Size = {1}, Voxel Size = {2}. "
				"Please change the Chunk Size on your Chunk Spawner subsystem to be a multiple of {3}",
				this,
				ChunkSize.X,
				VoxelSize,
				2 * VoxelSize);

			return {};
		}

		int32 ChunkSize;
		{
			const FIntVector Start = FVoxelUtilities::RoundToInt(Bounds.Min / ScaledVoxelSize);
			const FIntVector End = FVoxelUtilities::RoundToInt(Bounds.Max / ScaledVoxelSize);
			const FIntVector Size = End - Start;

			ChunkSize = Size.X;
			if (!ensure(ChunkSize == Size.Y) ||
				!ensure(ChunkSize == Size.Z) ||
				!ensure(ChunkSize % 2 == 0))
			{
				return {};
			}
		}

		if (ChunkSize > 256)
		{
			VOXEL_MESSAGE(Error, "{0}: Size would be {1}, which is above the limit of 256. Please increase Voxel Size", this, ChunkSize);
			return {};
		}

		const int32 DataSize = ChunkSize + 2;

		const TValue<bool> ShouldSkip = INLINE_LAMBDA -> TValue<bool>
		{
			if (!EnableDistanceChecks)
			{
				return false;
			}

			const float Size = Bounds.Size().GetMax();
			const float Tolerance = FMath::Max(DistanceChecksTolerance, 0.f);

			FVoxelQuery DensityQuery = Query.MakeCpuQuery();
			DensityQuery.Add<FVoxelGradientStepQueryData>().Step = ScaledVoxelSize;
			DensityQuery.Add<FVoxelDensePositionQueryData>().Initialize(FVector3f(Bounds.Min) + Size / 4.f, Size / 2.f, FIntVector(2));

			const TValue<TBufferView<float>> Densities = GetBufferView(DensityPin, DensityQuery);

			return VOXEL_ON_COMPLETE_CUSTOM(bool, "DistanceChecks", AsyncThread, Densities, Size, Tolerance)
			{
				bool bCanSkip = true;
				for (const float Density : Densities)
				{
					if (FMath::Abs(Density) < Size / 4.f * UE_SQRT_2 * (1.f + Tolerance))
					{
						bCanSkip = false;
					}
				}
				return bCanSkip;
			};
		};
		
		if (Query.IsGpu())
		{
			return VOXEL_ON_COMPLETE(RenderThread, ShouldSkip, LOD, ChunkSize, DataSize, ScaledVoxelSize, Bounds)
			{
				if (ShouldSkip)
				{
					return {};
				}

				FVoxelQuery DensityQuery = Query;
				DensityQuery.Add<FVoxelGradientStepQueryData>().Step = ScaledVoxelSize;
				DensityQuery.Add<FVoxelDensePositionQueryData>().Initialize(FVector3f(Bounds.Min), ScaledVoxelSize, FIntVector(DataSize));

				const TValue<TVoxelBuffer<float>> Densities = Get(DensityPin, DensityQuery);

				return VOXEL_ON_COMPLETE(RenderThread, Densities, LOD, ChunkSize, DataSize, ScaledVoxelSize)
				{
					if (Densities.IsConstant())
					{
						return {};
					}

					const FVoxelFloatBuffer UnpackedDensities = FVoxelFloatBuffer::MakeGpu(Densities.Num());

					BEGIN_VOXEL_SHADER_CALL(UnpackDensities)
					{
						ensure(DataSize % 2 == 0);
						const FIntVector BlockSize = FIntVector(DataSize) / 2;

						Parameters.BlockSize = BlockSize;
						Parameters.InDensities = Densities.GetGpuBuffer();
						Parameters.OutDensities = UnpackedDensities.GetGpuBuffer();

						Execute(FComputeShaderUtils::GetGroupCount(BlockSize, 4));
					}
					END_VOXEL_SHADER_CALL()

					const FVoxelInt32Buffer Nums = FVoxelInt32Buffer::MakeGpu(3);
					AddClearUAVPass(GraphBuilder, Nums.GetGpuBuffer(), 0);

					static FVoxelRDGExternalBufferRef RegularCellClass;
					static FVoxelRDGExternalBufferRef RegularCellData;
					static FVoxelRDGExternalBufferRef RegularVertexData;

					static bool bTransvoxelDataInitialized = false;
					if (!bTransvoxelDataInitialized)
					{
						bTransvoxelDataInitialized = true;

						RegularCellClass = FVoxelRDGExternalBuffer::CreateTyped<uint8>(
							Transvoxel::RegularCellClass,
							PF_R8_UINT,
							TEXT("RegularCellClass"));

						RegularCellData = FVoxelRDGExternalBuffer::CreateTyped<uint8>(
							MakeArrayView<const uint8>(
								reinterpret_cast<const uint8*>(Transvoxel::RegularCellData),
								sizeof(Transvoxel::RegularCellData)),
							PF_R8_UINT,
							TEXT("RegularCellData"));

						RegularVertexData = FVoxelRDGExternalBuffer::CreateTyped<uint16>(
							MakeArrayView<const uint16>(
								reinterpret_cast<const uint16*>(Transvoxel::RegularVertexData),
								sizeof(Transvoxel::RegularVertexData) / sizeof(uint16)),
							PF_R16_UINT,
							TEXT("RegularVertexData"));
					}

					BEGIN_VOXEL_SHADER_CALL(GenerateMarchingCubeMesh)
					{
						PermutationDomain.Set<FComputeNums>(true);

						Parameters.ChunkSize = ChunkSize;
						Parameters.DataSize = DataSize;
						Parameters.Densities = UnpackedDensities.GetGpuBuffer();

						Parameters.Nums = Nums.GetGpuBuffer();

						Parameters.RegularCellClass = FVoxelRDGBuffer(RegularCellClass);
						Parameters.RegularCellData = FVoxelRDGBuffer(RegularCellData);
						Parameters.RegularVertexData = FVoxelRDGBuffer(RegularVertexData);

						Execute(FComputeShaderUtils::GetGroupCount(FIntVector(ChunkSize), 4));
					}
					END_VOXEL_SHADER_CALL()

					const TValue<TBufferView<int32>> NumsView = Nums.MakeView();

					return VOXEL_ON_COMPLETE(RenderThread, Densities, LOD, ChunkSize, DataSize, ScaledVoxelSize, UnpackedDensities, NumsView)
					{
						if (NumsView[0] == 0 ||
							NumsView[1] == 0 ||
							NumsView[2] == 0)
						{
							return {};
						}

						const FVoxelInt4Buffer Cells = FVoxelInt4Buffer::MakeGpu(NumsView[0]);
						const FVoxelInt32Buffer Indices = FVoxelInt32Buffer::MakeGpu(NumsView[1]);
						const FVoxelFloatBuffer VerticesX = FVoxelFloatBuffer::MakeGpu(NumsView[2]);
						const FVoxelFloatBuffer VerticesY = FVoxelFloatBuffer::MakeGpu(NumsView[2]);
						const FVoxelFloatBuffer VerticesZ = FVoxelFloatBuffer::MakeGpu(NumsView[2]);
						
						const FVoxelInt32Buffer TempNums = FVoxelInt32Buffer::MakeGpu(3);
						AddClearUAVPass(GraphBuilder, TempNums.GetGpuBuffer(), 0);

						BEGIN_VOXEL_SHADER_CALL(GenerateMarchingCubeMesh)
						{
							PermutationDomain.Set<FComputeNums>(false);

							Parameters.ChunkSize = ChunkSize;
							Parameters.DataSize = DataSize;
							Parameters.Densities = UnpackedDensities.GetGpuBuffer();

							Parameters.OutCells = Cells.GetGpuBuffer();
							Parameters.OutIndices = Indices.GetGpuBuffer();
							Parameters.OutVerticesX = VerticesX.GetGpuBuffer();
							Parameters.OutVerticesY = VerticesY.GetGpuBuffer();
							Parameters.OutVerticesZ = VerticesZ.GetGpuBuffer();

							Parameters.Nums = TempNums.GetGpuBuffer();

							Parameters.RegularCellClass = FVoxelRDGBuffer(RegularCellClass);
							Parameters.RegularCellData = FVoxelRDGBuffer(RegularCellData);
							Parameters.RegularVertexData = FVoxelRDGBuffer(RegularVertexData);

							Execute(FComputeShaderUtils::GetGroupCount(FIntVector(ChunkSize), 4));
						}
						END_VOXEL_SHADER_CALL()
						
						const TSharedRef<FVoxelMarchingCubeSurface> Surface = MakeShared<FVoxelMarchingCubeSurface>();
						Surface->LOD = LOD;
						Surface->ChunkSize = ChunkSize;
						Surface->ScaledVoxelSize = ScaledVoxelSize;
						Surface->Cells = Cells;
						Surface->Indices = Indices;
						Surface->Vertices.X = VerticesX;
						Surface->Vertices.Y = VerticesY;
						Surface->Vertices.Z = VerticesZ;
						return Surface;
					};
				};
			};
		}
		else
		{
			return VOXEL_ON_COMPLETE(AsyncThread, ShouldSkip, LOD, ChunkSize, DataSize, ScaledVoxelSize, Bounds)
			{
				if (ShouldSkip)
				{
					return {};
				}

				FVoxelQuery DensityQuery = Query;
				DensityQuery.Add<FVoxelGradientStepQueryData>().Step = ScaledVoxelSize;
				DensityQuery.Add<FVoxelDensePositionQueryData>().Initialize(FVector3f(Bounds.Min), ScaledVoxelSize, FIntVector(DataSize));

				const TValue<TBufferView<float>> Densities = GetBufferView(DensityPin, DensityQuery);

				return VOXEL_ON_COMPLETE(AsyncThread, Densities, LOD, ChunkSize, DataSize, ScaledVoxelSize)
				{
					if (Densities.IsConstant() ||
						!ensure(Densities.Num() == DataSize * DataSize * DataSize))
					{
						return {};
					}

					TVoxelArray<float> UnpackedDensities;
					FVoxelUtilities::SetNumFast(UnpackedDensities, FIntVector(DataSize));

					FRuntimeUtilities::UnpackData<float>(
						Densities.GetRawView(),
						UnpackedDensities,
						FIntVector(DataSize));

					TVoxelArray<FVoxelInt4> Cells;
					TVoxelArray<int32> Indices;
					TVoxelArray<float> VerticesX;
					TVoxelArray<float> VerticesY;
					TVoxelArray<float> VerticesZ;
					FVoxelMarchingCubeProcessor(ChunkSize, DataSize).MainPass(
						UnpackedDensities,
						Cells,
						Indices,
						VerticesX,
						VerticesY,
						VerticesZ);

					FVoxelBuffer::Shrink(Cells);
					FVoxelBuffer::Shrink(Indices);
					FVoxelBuffer::Shrink(VerticesX);
					FVoxelBuffer::Shrink(VerticesY);
					FVoxelBuffer::Shrink(VerticesZ);

					if (VerticesX.Num() == 0 ||
						VerticesY.Num() == 0 ||
						VerticesZ.Num() == 0)
					{
						return {};
					}
					
					const TSharedRef<FVoxelMarchingCubeSurface> Surface = MakeShared<FVoxelMarchingCubeSurface>();
					Surface->LOD = LOD;
					Surface->ChunkSize = ChunkSize;
					Surface->ScaledVoxelSize = ScaledVoxelSize;
					Surface->Cells = FVoxelInt4Buffer::MakeCpu(Cells);
					Surface->Indices = FVoxelInt32Buffer::MakeCpu(Indices);
					Surface->Vertices = FVoxelVectorBuffer::MakeCpu(VerticesX, VerticesY, VerticesZ);
					return Surface;
				};
			};
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_FVoxelMarchingCubeSurface_CreateCollider, Collider)
{
	FindVoxelQueryData(FVoxelBoundsQueryData, BoundsQueryData);

	const TValue<FVoxelMarchingCubeSurface> Surface = Get(SurfacePin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, Surface)
	{
		if (Surface->Vertices.Num() == 0)
		{
			return {};
		}

		const TValue<TBufferView<int32>> Indices = Surface->Indices.MakeView();
		const TValue<TBufferView<FVector>> Vertices = Surface->Vertices.MakeView();

		return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, Surface, Indices, Vertices)
		{
			check(Surface->Indices.Num() % 3 == 0);
			const int32 NumTriangles = Surface->Indices.Num() / 3;

			TValue<TBufferView<FVoxelPhysicalMaterial>> Materials;
			if (IsDefaultValue(PhysicalMaterialPin))
			{
				Materials = GetBufferView(PhysicalMaterialPin, Query);
			}
			else
			{
				TVoxelArray<float> X = FVoxelFloatBuffer::Allocate(NumTriangles);
				TVoxelArray<float> Y = FVoxelFloatBuffer::Allocate(NumTriangles);
				TVoxelArray<float> Z = FVoxelFloatBuffer::Allocate(NumTriangles);

				for (int32 Index = 0; Index < NumTriangles; Index++)
				{
					const FVector3f A = Vertices[Indices[3 * Index + 0]] * Surface->ScaledVoxelSize;
					const FVector3f B = Vertices[Indices[3 * Index + 1]] * Surface->ScaledVoxelSize;
					const FVector3f C = Vertices[Indices[3 * Index + 2]] * Surface->ScaledVoxelSize;

					const FVector3f Center = FVector3f(BoundsQueryData->Bounds.Min) + (A + B + C) / 3.f;

					X[Index] = Center.X;
					Y[Index] = Center.Y;
					Z[Index] = Center.Z;
				}

				FVoxelQuery MaterialQuery = Query;
				MaterialQuery.Add<FVoxelSparsePositionQueryData>().Initialize(FVoxelVectorBuffer::MakeCpu(X, Y, Z));

				Materials = GetBufferView(PhysicalMaterialPin, MaterialQuery);
			}

			return VOXEL_ON_COMPLETE(AsyncThread, Surface, Indices, Vertices, Materials)
			{
				TVoxelArray<FVector3f> Positions;
				FVoxelUtilities::SetNumFast(Positions, Vertices.Num());

				for (int32 Index = 0; Index < Positions.Num(); Index++)
				{
					Positions[Index] = Vertices[Index] * Surface->ScaledVoxelSize;
				}

				if (Materials.IsConstant())
				{
					const TSharedPtr<FVoxelTriangleMeshCollider> Collider = FVoxelCollisionCooker::CookTriangleMesh(Indices.GetRawView(), Positions, {});
					if (Collider)
					{
						Collider->PhysicalMaterials.Add(Materials.GetConstant().Material);
					}
					return Collider;
				}

				TVoxelArray<uint16> MaterialIndices;
				TMap<uint64, uint16> MaterialToIndex;
				TVoxelArray<TWeakObjectPtr<UPhysicalMaterial>> PhysicalMaterials;

				for (const FVoxelPhysicalMaterial& Material : Materials)
				{
					uint16& Index = MaterialToIndex.FindOrAdd(ReinterpretCastRef<uint64>(Material), MAX_uint16);
					if (Index == MAX_uint16)
					{
						Index = PhysicalMaterials.Add(Material.Material);
					}
					MaterialIndices.Add(Index);
				}

				const TSharedPtr<FVoxelTriangleMeshCollider> Collider = FVoxelCollisionCooker::CookTriangleMesh(Indices.GetRawView(), Positions, MaterialIndices);
				if (Collider)
				{
					Collider->PhysicalMaterials = MoveTemp(PhysicalMaterials);
				}
				return Collider;
			};
		};
	};
}

DEFINE_VOXEL_NODE(FVoxelNode_FVoxelMarchingCubeSurface_CreateNavmesh, Navmesh)
{
	const TValue<FVoxelMarchingCubeSurface> Surface = Get(SurfacePin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, Surface)
	{
		if (Surface->Vertices.Num() == 0)
		{
			return {};
		}

		const TValue<TBufferView<int32>> Indices = Surface->Indices.MakeView();
		const TValue<TBufferView<FVector>> Vertices = Surface->Vertices.MakeView();

		return VOXEL_ON_COMPLETE(AsyncThread, Surface, Indices, Vertices)
		{
			FBox3f Bounds(ForceInit);

			TVoxelArray<FVector3f> Positions;
			FVoxelUtilities::SetNumFast(Positions, Surface->Vertices.Num());

			for (int32 Index = 0; Index < Positions.Num(); Index++)
			{
				const FVector3f Position = Vertices[Index] * Surface->ScaledVoxelSize;
				Bounds += Position;
				Positions[Index] = Position;
			}

			const TSharedRef<FVoxelNavmesh> Navmesh = MakeShared<FVoxelNavmesh>();
			Navmesh->Bounds = FBox(Bounds);
			Navmesh->Indices = TVoxelArray<int32>(Indices.GetRawView());
			Navmesh->Vertices = MoveTemp(Positions);
			return Navmesh;
		};
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_FVoxelMarchingCubeSurface_CreateMesh, Mesh)
{
	FindVoxelQueryData(FVoxelBoundsQueryData, BoundsQueryData);
	FindVoxelQueryData(FVoxelLODQueryData, LODQueryData);
	
	const TValue<FVoxelMarchingCubeSurface> Surface = Get(SurfacePin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, LODQueryData, Surface)
	{
		if (Surface->Vertices.Num() == 0)
		{
			return {};
		}

		const TValue<TBufferView<int32>> Indices = Surface->Indices.MakeView();
		const TValue<TBufferView<FVector>> Vertices = Surface->Vertices.MakeView();

		return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, LODQueryData, Surface, Indices, Vertices)
		{
			FVoxelVectorBuffer QueryPositions;
			{
				VOXEL_SCOPE_COUNTER("Build QueryPositions");

				const FVoxelBox Bounds = BoundsQueryData->Bounds;

				TVoxelArray<float> WriteX = FVoxelFloatBuffer::Allocate(Vertices.Num());
				TVoxelArray<float> WriteY = FVoxelFloatBuffer::Allocate(Vertices.Num());
				TVoxelArray<float> WriteZ = FVoxelFloatBuffer::Allocate(Vertices.Num());

				for (int32 Index = 0; Index < Vertices.Num(); Index++)
				{
					const FVector3f Position = FVector3f(Bounds.Min) + Vertices[Index] * Surface->ScaledVoxelSize;

					WriteX[Index] = Position.X;
					WriteY[Index] = Position.Y;
					WriteZ[Index] = Position.Z;
				}

				QueryPositions = FVoxelVectorBuffer::MakeCpu(WriteX, WriteY, WriteZ);
				QueryPositions.SetBounds(Bounds);
			}

			FVoxelQuery VertexDataQuery = Query;
			VertexDataQuery.Add<FVoxelGradientStepQueryData>().Step = Surface->ScaledVoxelSize;
			VertexDataQuery.Add<FVoxelSparsePositionQueryData>().Initialize(QueryPositions);

			const TValue<FVoxelVertexData> VertexData = Get(VertexDataPin, VertexDataQuery);

			FVoxelQuery MaterialQuery = Query;
			MaterialQuery.Add<FVoxelSurfaceQueryData>().Surface = Surface;

			const TValue<FVoxelMeshMaterial> Material = Get(MaterialPin, MaterialQuery);

			return VOXEL_ON_COMPLETE(AsyncThread, LODQueryData, Surface, Indices, Vertices, VertexData, Material)
			{
				const TSharedRef<FVoxelMarchingCubeMesh_LocalVF> Mesh = MakeVoxelMesh<FVoxelMarchingCubeMesh_LocalVF>();
				Mesh->LOD = LODQueryData->LOD;
				Mesh->Bounds = FBox(ForceInit);
				Mesh->MeshMaterial = Material;

				{
					VOXEL_SCOPE_COUNTER("Indices");

					Mesh->IndexBuffer.SetIndices(
						TArray<uint32>(Indices.GetRawView()),
						Vertices.Num() > MAX_uint16
						? EIndexBufferStride::Force32Bit
						: EIndexBufferStride::Force16Bit);
				}

				{
					VOXEL_SCOPE_COUNTER("Positions");

					Mesh->PositionVertexBuffer.Init(Vertices.Num(), false);
					for (int32 Index = 0; Index < Vertices.Num(); Index++)
					{
						const FVector3f Position = FVector3f(Vertices[Index]) * Surface->ScaledVoxelSize;
						Mesh->Bounds += FVector(Position);
						Mesh->PositionVertexBuffer.VertexPosition(Index) = Position;
					}
				}

				const int32 NumTextureCoordinates = FMath::Max(1, VertexData->AllTextureCoordinates.Num());

				Mesh->StaticMeshVertexBuffer.SetUseFullPrecisionUVs(true);
				Mesh->StaticMeshVertexBuffer.Init(Vertices.Num(), NumTextureCoordinates, false);

				{
					VOXEL_SCOPE_COUNTER("Tangents");

					if (VertexData->Normals.Num() == Vertices.Num())
					{
						for (int32 Index = 0; Index < Vertices.Num(); Index++)
						{
							const FVector3f Normal = VertexData->Normals[Index].GetSafeNormal();

							Mesh->StaticMeshVertexBuffer.SetVertexTangents(
								Index,
								FVector3f(1, 0, 0),
								FVector3f::CrossProduct(Normal, FVector3f(1, 0, 0)),
								Normal);
						}
					}
					else
					{
						for (int32 Index = 0; Index < Vertices.Num(); Index++)
						{
							Mesh->StaticMeshVertexBuffer.SetVertexTangents(
								Index,
								FVector3f(1, 0, 0),
								FVector3f(0, 1, 0),
								FVector3f(0, 0, 1));
						}
					}
				}

				{
					VOXEL_SCOPE_COUNTER("TextureCoordinates");

					for (int32 TexCoord = 0; TexCoord < NumTextureCoordinates; TexCoord++)
					{
						if (VertexData->AllTextureCoordinates.IsValidIndex(TexCoord) &&
							VertexData->AllTextureCoordinates[TexCoord].Num() == Vertices.Num())
						{
							const TVoxelArray<FVector2f>& TextureCoordinates = VertexData->AllTextureCoordinates[TexCoord];
							for (int32 Index = 0; Index < Vertices.Num(); Index++)
							{
								Mesh->StaticMeshVertexBuffer.SetVertexUV(Index, TexCoord, TextureCoordinates[Index]);
							}
						}
						else
						{
							for (int32 Index = 0; Index < Vertices.Num(); Index++)
							{
								Mesh->StaticMeshVertexBuffer.SetVertexUV(Index, TexCoord, FVector2f(ForceInit));
							}
						}
					}
				}

				{
					VOXEL_SCOPE_COUNTER("Colors");

					Mesh->ColorVertexBuffer.Init(Vertices.Num(), false);

					if (VertexData->Colors.Num() == Vertices.Num())
					{
						for (int32 Index = 0; Index < Vertices.Num(); Index++)
						{
							Mesh->ColorVertexBuffer.VertexColor(Index) = VertexData->Colors[Index];
						}
					}
					else
					{
						for (int32 Index = 0; Index < Vertices.Num(); Index++)
						{
							Mesh->ColorVertexBuffer.VertexColor(Index) = FColor(ForceInit);
						}
					}
				}

				return Mesh;
			};
		};
	};
}