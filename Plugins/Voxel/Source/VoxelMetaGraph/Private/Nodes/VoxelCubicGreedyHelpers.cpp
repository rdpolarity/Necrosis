// Copyright Voxel Plugin, Inc. All Rights Reserved.

#if 0 // TODO
#include "Nodes/VoxelCubicGreedyHelpers.h"
#include "Nodes/VoxelCubicGreedyNodes.h"
#include "VoxelTextureAtlas.h"

void FVoxelCubicGreedyVertexFactoryShaderParameters::Bind(const FShaderParameterMap& ParameterMap)
{
#define BIND(Name) Name.Bind(ParameterMap, TEXT(#Name))
	BIND(ScaledVoxelSize);
	BIND(Quads);
#undef BIND
}

void FVoxelCubicGreedyVertexFactoryShaderParameters::GetElementShaderBindings(
	const FSceneInterface* Scene, 
	const FSceneView* View, 
	const FMeshMaterialShader* Shader, 
	const EVertexInputStreamType InputStreamType, 
	ERHIFeatureLevel::Type FeatureLevel,
	const FVertexFactory* VertexFactory, 
	const FMeshBatchElement& BatchElement,
	FMeshDrawSingleShaderBindings& ShaderBindings, 
	FVertexInputStreamArray& VertexStreams) const
{
	const FVoxelCubicGreedyVertexFactory& VoxelVertexFactory = static_cast<const FVoxelCubicGreedyVertexFactory&>(*VertexFactory);

	ensure(VoxelVertexFactory.ScaledVoxelSize != 0);
	check(VoxelVertexFactory.QuadsBuffer);
	check(VoxelVertexFactory.QuadsBuffer->IsInitialized());

	ShaderBindings.Add(ScaledVoxelSize, VoxelVertexFactory.ScaledVoxelSize);
	ShaderBindings.Add(Quads, VoxelVertexFactory.QuadsBuffer->ShaderResourceViewRHI);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelCubicGreedyQuadsRenderBuffer::InitRHI()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInRenderingThread());
	ensure(Quads.Num() > 0);

	{
		FRHIResourceCreateInfo CreateInfo(TEXT("VoxelGreedyQuads"));
		VertexBufferRHI = RHICreateVertexBuffer(sizeof(uint32) * Quads.Num(), BUF_ShaderResource | BUF_Static, CreateInfo);

		FVoxelRenderUtilities::UpdateBuffer(VertexBufferRHI, Quads);
	}

	// Free up memory
	Quads.Empty();

	ShaderResourceViewRHI = RHICreateShaderResourceView(VertexBufferRHI, 2 * sizeof(uint32), PF_R32G32_UINT);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelCubicGreedyVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
{
	if (Parameters.MaterialParameters.bIsSpecialEngineMaterial)
	{
		return true;
	}

	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) &&
		Parameters.MaterialParameters.MaterialDomain == MD_Surface &&
		Parameters.MaterialParameters.bIsUsedWithLidarPointCloud;
}

void FVoxelCubicGreedyVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	OutEnvironment.SetDefine(TEXT("VOXEL_CUBIC_GREEDY"), true);
	OutEnvironment.SetDefine(TEXT("VOXEL_ENGINE_VERSION"), VOXEL_ENGINE_VERSION);
	OutEnvironment.SetDefine(TEXT("VOXEL_CHUNK_SIZE"), FVoxelPackedQuad::ChunkSize);
}

void FVoxelCubicGreedyVertexFactory::InitRHI()
{
	FVertexFactory::InitRHI();

	check(QuadsBuffer);
	check(QuadsBuffer->IsInitialized());

	InitDeclaration({}, EVertexInputStreamType::Default);
	InitDeclaration({}, EVertexInputStreamType::PositionOnly);
	InitDeclaration({}, EVertexInputStreamType::PositionAndNormalOnly);
}

void FVoxelCubicGreedyVertexFactory::ReleaseRHI()
{
	FVertexFactory::ReleaseRHI();
}

IMPLEMENT_TYPE_LAYOUT(FVoxelCubicGreedyVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelCubicGreedyVertexFactory, SF_Vertex, FVoxelCubicGreedyVertexFactoryShaderParameters);

IMPLEMENT_VERTEX_FACTORY_TYPE(FVoxelCubicGreedyVertexFactory, "/Engine/Plugins/Voxel/Private/VoxelCubicGreedyVertexFactory.ush",
	EVertexFactoryFlags::UsedWithMaterials |
	EVertexFactoryFlags::SupportsDynamicLighting |
	EVertexFactoryFlags::SupportsPositionOnly |
	EVertexFactoryFlags::SupportsCachingMeshDrawCommands);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int64 FVoxelCubicGreedyMesh::GetAllocatedSize() const
{
	return QuadsBuffer.Quads.GetAllocatedSize();
}

void FVoxelCubicGreedyMesh::Initialize_GameThread()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	if (!GetMaterial_GameThread)
	{
		return;
	}

	const TSharedPtr<FVoxelMaterialRef> Material = GetMaterial_GameThread();
	if (!Material)
	{
		return;
	}

	UMaterialInterface* MaterialInterface = Material->GetMaterial();
	if (!MaterialInterface)
	{
		return;
	}

	if (UMaterial* MaterialObject = MaterialInterface->GetMaterial())
	{
		if (!MaterialObject->bUsedWithLidarPointCloud &&
			!MaterialObject->bUsedAsSpecialEngineMaterial)
		{
			VOXEL_MESSAGE(Error, "Material {0} needs to have UsedWithLidarPointCloud on to be used a cubic greedy material", MaterialObject);
		}
	}

	if (TextureDatas.Num() == 0)
	{
		MaterialInstance = Material;
		return;
	}

	MaterialInstance = FVoxelMaterialRef::MakeInstance(MaterialInterface);

	UVoxelTextureAtlasSubsystem* Subsystem = GEngine->GetEngineSubsystem<UVoxelTextureAtlasSubsystem>();
	if (!ensure(Subsystem))
	{
		return;
	}

	for (auto& It : TextureDatas)
	{
		FVoxelTextureAtlas& Pool = Subsystem->GetAtlas(It.Value->PixelFormat);
		Pool.AddEntry(It.Value.ToSharedRef(), MaterialInstance.ToSharedRef(), It.Key);
	}
}

///////////////////////////////////////////////////////////////////////////////

void FVoxelCubicGreedyMesh::Initialize_RenderThread(ERHIFeatureLevel::Type FeatureLevel)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInRenderingThread());

	VOXEL_INLINE_COUNTER("Quads", QuadsBuffer.InitResource());

	check(!VertexFactory);
	VertexFactory = MakeShared<FVoxelCubicGreedyVertexFactory>(FeatureLevel);
	VertexFactory->ScaledVoxelSize = ScaledVoxelSize;
	VertexFactory->QuadsBuffer = &QuadsBuffer;
	VOXEL_INLINE_COUNTER("VertexFactory", VertexFactory->InitResource());
}

void FVoxelCubicGreedyMesh::Destroy_RenderThread()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInRenderingThread());

	QuadsBuffer.ReleaseResource();

	check(VertexFactory);
	VertexFactory->ReleaseResource();
	VertexFactory.Reset();
}
///////////////////////////////////////////////////////////////////////////////

bool FVoxelCubicGreedyMesh::Draw_RenderThread(const FPrimitiveSceneProxy& Proxy, FMeshBatch& MeshBatch)
{
	if (!ensure(MeshBatch.MaterialRenderProxy))
	{
		return false;
	}

	const FMaterial& Material = MeshBatch.MaterialRenderProxy->GetMaterialWithFallback(Proxy.GetScene().GetFeatureLevel(), MeshBatch.MaterialRenderProxy);
	if (!Material.IsUsedWithLidarPointCloud() && 
		!Material.IsSpecialEngineMaterial())
	{
		MeshBatch.MaterialRenderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
	}

	MeshBatch.Type = PT_TriangleList;
	MeshBatch.VertexFactory = VertexFactory.Get();

	FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
	BatchElement.PrimitiveUniformBuffer = Proxy.GetUniformBuffer();
	BatchElement.IndexBuffer = nullptr;
	BatchElement.FirstIndex = 0;
	BatchElement.NumPrimitives = 2 * NumQuads;
	// Should be unused by the current pipeline (see FMeshDrawCommand::SubmitDraw)
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = 0;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	MeshBatch.VisualizeLODIndex = LOD % GEngine->LODColorationColors.Num();
#endif

	return true;
}
#endif