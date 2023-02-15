// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMaterialLayerNodes.h"
#include "VoxelMaterialLayerRenderer.h"
#include "Nodes/VoxelPositionNodes.h"

DEFINE_VOXEL_NODE_CPU(FVoxelNode_MakeMaterialLayerDetailTexture, DetailTexture)
{
	FindVoxelQueryData(FVoxelDetailTextureQueryData, DetailTextureQueryData);

	const TValue<TBufferView<FVoxelMaterialLayer>> Layers = GetBufferView(LayerPin, Query);
	const TValue<FName> Name = Get(NamePin, Query);
	
	return VOXEL_ON_COMPLETE(AsyncThread, DetailTextureQueryData, Layers, Name)
	{
		FindVoxelQueryData(FVoxelPositionQueryData, PositionQueryData);
		CheckVoxelBuffersNum(Layers, PositionQueryData->GetPositions());

		const uint8 Class = Layers[0].Class;

		if (OutClass)
		{
			*OutClass = Class;
		}

		const TSharedRef<FVoxelDetailTexture> DetailTexture = MakeShared<FVoxelDetailTexture>();
		DetailTexture->Name = Name;
		DetailTexture->SizeX = DetailTextureQueryData->AtlasTextureSize;
		DetailTexture->SizeY = DetailTextureQueryData->AtlasTextureSize;
		DetailTexture->Format = PF_R8_UINT;
		DetailTexture->SetCpuData(*DetailTextureQueryData, [&](TVoxelArray<uint8>& Data, int32 QueryIndex, int32 DataIndex, int32 CellIndex)
		{
			const FVoxelMaterialLayer Layer = Layers[QueryIndex];
			if (Layer.Class != Class)
			{
				VOXEL_MESSAGE(Error, "{0}: Material layers with different classes rendered in the same chunk", this);
			}

			Data[DataIndex] = Layer.Layer;
		});
		
		return DetailTexture;
	};
}

BEGIN_VOXEL_NAMESPACE(MetaGraph)

BEGIN_VOXEL_COMPUTE_SHADER(MakeMaterialLayerDetailTexture)
	VOXEL_SHADER_PARAMETER_CST(int32, TextureSize)
	VOXEL_SHADER_PARAMETER_CST(int32, NumCells)
	VOXEL_SHADER_PARAMETER_CST(int32, NumCellsPerSide)

	VOXEL_SHADER_PARAMETER_SRV(Buffer<uint2>, Layer)
	
	VOXEL_SHADER_PARAMETER_TEXTURE_UAV(Texture2D<uint>, Texture)
END_VOXEL_SHADER()

END_VOXEL_NAMESPACE(MetaGraph)

DEFINE_VOXEL_NODE_GPU(FVoxelNode_MakeMaterialLayerDetailTexture, DetailTexture)
{
	FindVoxelQueryData(FVoxelDetailTextureQueryData, DetailTextureQueryData);
	
	const TValue<FVoxelMaterialLayerBuffer> Layers = Get(LayerPin, Query);
	const TValue<FName> Name = Get(NamePin, Query);

	return VOXEL_ON_COMPLETE(RenderThread, DetailTextureQueryData, Layers, Name)
	{
		VOXEL_USE_NAMESPACE(MetaGraph);

		FindVoxelQueryData(FVoxelPositionQueryData, PositionQueryData);
		CheckVoxelBuffersNum(Layers, PositionQueryData->GetPositions());

		const FRDGTextureRef Texture = GraphBuilder.CreateTexture(
			FRDGTextureDesc::Create2D(
				FIntPoint(DetailTextureQueryData->AtlasTextureSize, DetailTextureQueryData->AtlasTextureSize),
				PF_R8_UINT,
				FClearValueBinding::Black,
				TexCreate_ShaderResource | TexCreate_UAV),
			TEXT("MaterialLayerTexture"));

		BEGIN_VOXEL_SHADER_CALL(MakeMaterialLayerDetailTexture)
		{
			Parameters.TextureSize = DetailTextureQueryData->TextureSize;
			Parameters.NumCells = DetailTextureQueryData->NumCells;
			Parameters.NumCellsPerSide = DetailTextureQueryData->NumCellsPerSide;

			Parameters.Layer = Layers.GetGpuBuffer();

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
		DetailTexture->Format = PF_R8_UINT;
		DetailTexture->GpuTexture = ExtractedTexture;
		return DetailTexture;
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_CreateLayeredMaterial, Material)
{
	return VOXEL_CALL_NODE(FVoxelNode_CreateDetailTextureMaterial_Internal, MaterialPin)
	{
		VOXEL_CALL_NODE_BIND(TextureSizeBiasPin)
		{
			return Get(TextureSizeBiasPin, Query);
		};
		VOXEL_CALL_NODE_BIND(FallbackMaterialPin)
		{
			return {};
		};
		VOXEL_CALL_NODE_BIND(InputPin)
		{
			const TArray<TValue<FVoxelDetailTexture>> DetailTextures = Get(DetailTexturesPins, Query);

			const TSharedRef<uint8> Class = MakeShared<uint8>(0xFF);
			const TValue<FVoxelDetailTexture> DetailTexture = VOXEL_CALL_NODE(FVoxelNode_MakeMaterialLayerDetailTexture, DetailTexturePin)
			{
				Node.OutClass = Class;

				VOXEL_CALL_NODE_BIND(LayerPin)
				{
					return Get(LayerPin, Query);
				};
				VOXEL_CALL_NODE_BIND(NamePin)
				{
					return STATIC_FNAME("MaterialLayer");
				};
			};

			return VOXEL_ON_COMPLETE(AnyThread, DetailTextures, Class, DetailTexture)
			{
				const TSharedRef<FVoxelMeshMaterial> Material = MakeShared<FVoxelMeshMaterial>();
				Material->Material = GetSubsystem<FVoxelMaterialLayerRenderer>().GetMaterialInstance_AnyThread(*Class);

				const TSharedRef<FVoxelNode_CreateDetailTextureMaterial_Internal_Input> Input = MakeShared<FVoxelNode_CreateDetailTextureMaterial_Internal_Input>();
				Input->Material = Material;
				Input->DetailTextures = DetailTextures;
				Input->DetailTextures.Add(DetailTexture);
				return Input;
			};
		};
	};
}