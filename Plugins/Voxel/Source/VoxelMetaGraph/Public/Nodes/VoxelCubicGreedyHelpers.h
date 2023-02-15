// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#if 0 // TODO
#include "VoxelMinimal.h"
#include "VoxelMesh/VoxelMesh.h"
#include "Nodes/VoxelCubicGreedyNodes.h"
#include "VoxelCubicGreedyHelpers.generated.h"

class VOXELMETAGRAPH_API FVoxelCubicGreedyVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
	DECLARE_TYPE_LAYOUT(FVoxelCubicGreedyVertexFactoryShaderParameters, NonVirtual);

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

	LAYOUT_FIELD(FShaderParameter, ScaledVoxelSize);
	LAYOUT_FIELD(FShaderResourceParameter, Quads);
};

class VOXELMETAGRAPH_API FVoxelCubicGreedyQuadsRenderBuffer : public FVertexBufferWithSRV
{
public:
	TArray<uint32> Quads;

	virtual void InitRHI() override;
};

class VOXELMETAGRAPH_API FVoxelCubicGreedyVertexFactory : public FVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FVoxelCubicGreedyVertexFactory);

public:
	float ScaledVoxelSize = 0.f;
	FVoxelCubicGreedyQuadsRenderBuffer* QuadsBuffer = nullptr;

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
struct VOXELMETAGRAPH_API FVoxelCubicGreedyMesh : public FVoxelMesh
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	int32 LOD = 0;
	TFunction<TSharedRef<FVoxelMaterialRef>()> GetMaterial_GameThread;

	FBox Bounds = FBox(ForceInit);
	int32 NumQuads = 0;
	float ScaledVoxelSize = 0;
	TMap<FName, TSharedPtr<const FVoxelTextureAtlasTextureData>> TextureDatas;

	FVoxelCubicGreedyQuadsRenderBuffer QuadsBuffer;

	virtual FBox GetBounds() const override { return Bounds; }
	virtual int64 GetAllocatedSize() const override;
	virtual TSharedPtr<FVoxelMaterialRef> GetMaterial() const override { return MaterialInstance; }

	virtual void Initialize_GameThread() override;

	virtual void Initialize_RenderThread(ERHIFeatureLevel::Type FeatureLevel) override;
	virtual void Destroy_RenderThread() override;

	virtual bool Draw_RenderThread(const FPrimitiveSceneProxy& Proxy, FMeshBatch& MeshBatch) override;

private:
	TSharedPtr<FVoxelCubicGreedyVertexFactory> VertexFactory;
	TSharedPtr<FVoxelMaterialRef> MaterialInstance;
};
#endif