// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMesh/VoxelMesh.h"
#include "VoxelMarchingCubeMesh_VoxelVF.generated.h"

struct FVoxelMeshMaterial;

class VOXELMETAGRAPH_API FVoxelMarchingCubeVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
	DECLARE_TYPE_LAYOUT(FVoxelMarchingCubeVertexFactoryShaderParameters, NonVirtual);

public:
	void Bind(const FShaderParameterMap& ParameterMap);
	void GetElementShaderBindings(
		const FSceneInterface* Scene,
		const FSceneView* View,
		const FMeshMaterialShader* Shader,
		const EVertexInputStreamType InputStreamType,
		ERHIFeatureLevel::Type FeatureLevel,
		const FVertexFactory* VertexFactory,
		const FMeshBatchElement& BatchElement,
		FMeshDrawSingleShaderBindings& ShaderBindings,
		FVertexInputStreamArray& VertexStreams) const;

	LAYOUT_FIELD(FShaderParameter, ChunkSize);
	LAYOUT_FIELD(FShaderParameter, NumCellsPerSide);
	LAYOUT_FIELD(FShaderParameter, TextureSize);
	LAYOUT_FIELD(FShaderParameter, AtlasTextureSize);
	LAYOUT_FIELD(FShaderResourceParameter, TextureSampler);
	LAYOUT_FIELD(FShaderResourceParameter, IndirectionTexture);
	LAYOUT_FIELD(FShaderResourceParameter, ColorTexture);
	LAYOUT_FIELD(FShaderResourceParameter, NormalTexture);
};

class VOXELMETAGRAPH_API FVoxelMarchingCubeVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FVoxelMarchingCubeVertexFactory);

public:
	int32 ChunkSize = 0;
	int32 NumCellsPerSide = 0;
	int32 TextureSize = 0;
	int32 AtlasTextureSize = 0;
	FTextureRHIRef IndirectionTexture;
	FTextureRHIRef ColorTexture;
	FTextureRHIRef NormalTexture;

	FVertexStreamComponent PositionComponentX;
	FVertexStreamComponent PositionComponentY;
	FVertexStreamComponent PositionComponentZ;

	using FVertexFactory::FVertexFactory;

	static bool ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);
	static void ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

	virtual bool SupportsPositionOnlyStream() const override { return true; }
	virtual bool SupportsPositionAndNormalOnlyStream() const override { return true; }

	//~ Begin FRenderResource Interface
	virtual void InitRHI() override;
	virtual void ReleaseRHI() override;
	//~ End FRenderResource Interface
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelMarchingCubeMesh_VoxelVF : public FVoxelMesh
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()
		
	int32 LOD = 0;
	FBox Bounds = FBox(ForceInit);
	float VoxelSize = 0.f;
	TSharedPtr<const FVoxelMeshMaterial> MeshMaterial;

	FVoxelRDGExternalBufferRef Indices;
	FVoxelRDGExternalBufferRef VerticesX;
	FVoxelRDGExternalBufferRef VerticesY;
	FVoxelRDGExternalBufferRef VerticesZ;

	virtual FBox GetBounds() const override { return Bounds; }
	virtual int64 GetAllocatedSize() const override;
	virtual TSharedPtr<FVoxelMaterialRef> GetMaterial() const override { return Material; }
	virtual FVector GetScale() const override;

	virtual void Initialize_GameThread() override;

	virtual void Initialize_RenderThread(ERHIFeatureLevel::Type FeatureLevel) override;
	virtual void Destroy_RenderThread() override;

	virtual bool Draw_RenderThread(const FPrimitiveSceneProxy& Proxy, FMeshBatch& MeshBatch) const override;

private:
	FIndexBuffer Indices_Buffer;
	FVertexBuffer VerticesX_Buffer;
	FVertexBuffer VerticesY_Buffer;
	FVertexBuffer VerticesZ_Buffer;

	TSharedPtr<FVoxelMarchingCubeVertexFactory> VertexFactory;
	TSharedPtr<FVoxelMaterialRef> Material;
};