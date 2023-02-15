// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/MarchingCube/VoxelMarchingCubeMesh_VoxelVF.h"
#include "Nodes/VoxelMeshMaterialNodes.h"

void FVoxelMarchingCubeVertexFactoryShaderParameters::Bind(const FShaderParameterMap& ParameterMap)
{
#define BIND(Name) Name.Bind(ParameterMap, TEXT(#Name))
	BIND(ChunkSize);
	BIND(NumCellsPerSide);
	BIND(TextureSize);
	BIND(AtlasTextureSize);
	BIND(TextureSampler);
	BIND(IndirectionTexture);
	BIND(ColorTexture);
	BIND(NormalTexture);
#undef BIND
}

void FVoxelMarchingCubeVertexFactoryShaderParameters::GetElementShaderBindings(
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
	const FVoxelMarchingCubeVertexFactory& VoxelVertexFactory = static_cast<const FVoxelMarchingCubeVertexFactory&>(*VertexFactory);

	ShaderBindings.Add(ChunkSize, VoxelVertexFactory.ChunkSize);
	ShaderBindings.Add(NumCellsPerSide, VoxelVertexFactory.NumCellsPerSide);
	ShaderBindings.Add(TextureSize, VoxelVertexFactory.TextureSize);
	ShaderBindings.Add(AtlasTextureSize, VoxelVertexFactory.AtlasTextureSize);

	ShaderBindings.AddTexture(
		IndirectionTexture,
		TextureSampler,
		TStaticSamplerState<SF_Bilinear>::GetRHI(),
		VoxelVertexFactory.IndirectionTexture ? VoxelVertexFactory.IndirectionTexture : GBlackTexture->TextureRHI);

	ShaderBindings.AddTexture(
		ColorTexture,
		TextureSampler,
		TStaticSamplerState<SF_Bilinear>::GetRHI(),
		VoxelVertexFactory.ColorTexture ? VoxelVertexFactory.ColorTexture : GWhiteTexture->TextureRHI);

	ShaderBindings.AddTexture(
		NormalTexture,
		TextureSampler,
		TStaticSamplerState<SF_Bilinear>::GetRHI(),
		VoxelVertexFactory.NormalTexture ? VoxelVertexFactory.NormalTexture : GBlackTexture->TextureRHI);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelMarchingCubeVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
{
	if (Parameters.MaterialParameters.bIsSpecialEngineMaterial)
	{
		return true;
	}

	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) &&
		Parameters.MaterialParameters.MaterialDomain == MD_Surface;
}

void FVoxelMarchingCubeVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	OutEnvironment.SetDefine(TEXT("VOXEL_ENGINE_VERSION"), VOXEL_ENGINE_VERSION);
}

void FVoxelMarchingCubeVertexFactory::InitRHI()
{
	FVertexFactory::InitRHI();

	const auto AddDeclaration = [&](EVertexInputStreamType Type)
	{
		FVertexDeclarationElementList Elements;
		Elements.Add(AccessStreamComponent(PositionComponentX, 0, Type));
		Elements.Add(AccessStreamComponent(PositionComponentY, 1, Type));
		Elements.Add(AccessStreamComponent(PositionComponentZ, 2, Type));
		InitDeclaration(Elements, Type);
	};

	AddDeclaration(EVertexInputStreamType::Default);
	AddDeclaration(EVertexInputStreamType::PositionOnly);
	AddDeclaration(EVertexInputStreamType::PositionAndNormalOnly);
}

void FVoxelMarchingCubeVertexFactory::ReleaseRHI()
{
	FVertexFactory::ReleaseRHI();
}

IMPLEMENT_TYPE_LAYOUT(FVoxelMarchingCubeVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelMarchingCubeVertexFactory, SF_Pixel, FVoxelMarchingCubeVertexFactoryShaderParameters);

IMPLEMENT_VERTEX_FACTORY_TYPE(FVoxelMarchingCubeVertexFactory, "/Engine/Plugins/Voxel/Private/VoxelMarchingCubeVertexFactory.ush",
	EVertexFactoryFlags::UsedWithMaterials |
	EVertexFactoryFlags::SupportsDynamicLighting |
	EVertexFactoryFlags::SupportsPositionOnly |
	EVertexFactoryFlags::SupportsCachingMeshDrawCommands);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int64 FVoxelMarchingCubeMesh_VoxelVF::GetAllocatedSize() const
{
	return
		Indices->GetNumBytes() +
		VerticesX->GetNumBytes() +
		VerticesY->GetNumBytes() +
		VerticesZ->GetNumBytes();
}

FVector FVoxelMarchingCubeMesh_VoxelVF::GetScale() const
{
	return FVector(VoxelSize);
}

void FVoxelMarchingCubeMesh_VoxelVF::Initialize_GameThread()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	check(MeshMaterial);

	ensure(!Material);
	Material = MeshMaterial->GetMaterial_GameThread();

	const UMaterial* MaterialObject = Material->GetMaterial()->GetMaterial();
	if (ensure(MaterialObject) &&
		MaterialObject->bTangentSpaceNormal &&
		!MaterialObject->bUsedAsSpecialEngineMaterial)
	{
		VOXEL_MESSAGE(Error, "{0} should have bTangentSpaceNormal set to false to be used on voxel terrain", MaterialObject);
	}

	if (!Material->IsInstance())
	{
		Material = FVoxelMaterialRef::MakeInstance(Material->GetMaterial());
	}

	check(!VertexFactory);
	VertexFactory = MakeShared<FVoxelMarchingCubeVertexFactory>(GMaxRHIFeatureLevel);

	const UMaterialInstanceDynamic* MaterialInstance = Material->GetMaterialInstance();
	if (!ensure(MaterialInstance))
	{
		return;
	}

	{
		float ChunkSize = 0.f;
		MaterialInstance->GetScalarParameterValue(STATIC_FNAME("VoxelDetailTextures_ChunkSize"), ChunkSize, true);
		VertexFactory->ChunkSize = FMath::RoundToInt(ChunkSize);
	}

	{
		float NumCellsPerSide = 0.f;
		MaterialInstance->GetScalarParameterValue(STATIC_FNAME("VoxelDetailTextures_NumCellsPerSide"), NumCellsPerSide, true);
		VertexFactory->NumCellsPerSide = FMath::RoundToInt(NumCellsPerSide);
	}

	{
		float TextureSize = 0.f;
		MaterialInstance->GetScalarParameterValue(STATIC_FNAME("VoxelDetailTextures_TextureSize"), TextureSize, true);
		VertexFactory->TextureSize = FMath::RoundToInt(TextureSize);
	}

	{
		float AtlasTextureSize = 0.f;
		MaterialInstance->GetScalarParameterValue(STATIC_FNAME("VoxelDetailTextures_AtlasTextureSize"), AtlasTextureSize, true);
		VertexFactory->AtlasTextureSize = FMath::RoundToInt(AtlasTextureSize);
	}

	{
		UTexture* IndirectionTexture = nullptr;
		MaterialInstance->GetTextureParameterValue(STATIC_FNAME("VoxelDetailTextures_IndirectionTexture"), IndirectionTexture, true);

		if (IndirectionTexture && ensure(IndirectionTexture->GetResource()))
		{
			VertexFactory->IndirectionTexture = IndirectionTexture->GetResource()->GetTexture2DRHI();
		}
	}

	{
		UTexture* ColorTexture = nullptr;
		MaterialInstance->GetTextureParameterValue(STATIC_FNAME("VoxelDetailTextures_ColorTexture"), ColorTexture, true);

		if (ColorTexture && ensure(ColorTexture->GetResource()))
		{
			VertexFactory->ColorTexture = ColorTexture->GetResource()->GetTexture2DRHI();
		}
	}

	{
		UTexture* NormalTexture = nullptr;
		MaterialInstance->GetTextureParameterValue(STATIC_FNAME("VoxelDetailTextures_NormalTexture"), NormalTexture, true);

		if (NormalTexture && ensure(NormalTexture->GetResource()))
		{
			VertexFactory->NormalTexture = NormalTexture->GetResource()->GetTexture2DRHI();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void FVoxelMarchingCubeMesh_VoxelVF::Initialize_RenderThread(ERHIFeatureLevel::Type FeatureLevel)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInRenderingThread());

	Indices_Buffer.IndexBufferRHI = Indices->GetBuffer();
	Indices_Buffer.InitResource();

	VerticesX_Buffer.VertexBufferRHI = VerticesX->GetBuffer();
	VerticesX_Buffer.InitResource();

	VerticesY_Buffer.VertexBufferRHI = VerticesY->GetBuffer();
	VerticesY_Buffer.InitResource();

	VerticesZ_Buffer.VertexBufferRHI = VerticesZ->GetBuffer();
	VerticesZ_Buffer.InitResource();

	VertexFactory->PositionComponentX = FVertexStreamComponent(&VerticesX_Buffer, 0, sizeof(float), VET_Float1);
	VertexFactory->PositionComponentY = FVertexStreamComponent(&VerticesY_Buffer, 0, sizeof(float), VET_Float1);
	VertexFactory->PositionComponentZ = FVertexStreamComponent(&VerticesZ_Buffer, 0, sizeof(float), VET_Float1);

	VOXEL_INLINE_COUNTER("VertexFactory", VertexFactory->InitResource());
}

void FVoxelMarchingCubeMesh_VoxelVF::Destroy_RenderThread()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInRenderingThread());

	Indices_Buffer.ReleaseResource();
	VerticesX_Buffer.ReleaseResource();
	VerticesY_Buffer.ReleaseResource();
	VerticesZ_Buffer.ReleaseResource();

	check(VertexFactory);
	VertexFactory->ReleaseResource();
	VertexFactory.Reset();
}

///////////////////////////////////////////////////////////////////////////////

bool FVoxelMarchingCubeMesh_VoxelVF::Draw_RenderThread(const FPrimitiveSceneProxy& Proxy, FMeshBatch& MeshBatch) const
{
	if (!ensure(MeshBatch.MaterialRenderProxy))
	{
		return false;
	}

	MeshBatch.Type = PT_TriangleList;
	MeshBatch.VertexFactory = VertexFactory.Get();

	FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
	BatchElement.PrimitiveUniformBuffer = Proxy.GetUniformBuffer();
	BatchElement.IndexBuffer = &Indices_Buffer;
	BatchElement.FirstIndex = 0;
	BatchElement.NumPrimitives = Indices->GetNumElements() / 3;
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = VerticesX->GetNumElements() - 1;
	
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	MeshBatch.VisualizeLODIndex = LOD % GEngine->LODColorationColors.Num();
#endif

	return true;
}