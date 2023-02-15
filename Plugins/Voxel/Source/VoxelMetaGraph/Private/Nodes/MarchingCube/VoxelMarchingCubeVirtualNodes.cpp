// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/MarchingCube/VoxelMarchingCubeVirtualNodes.h"
#include "Nodes/MarchingCube/VoxelMarchingCubeMesh_VoxelVF.h"
#include "Nodes/VoxelPositionNodes.h"
#include "Nodes/VoxelDetailTextureNodes.h"
#include "MeshOptimizer.h"
#include "MaterialCompiler.h"

DEFINE_VOXEL_NODE(FVoxelNode_FVoxelMarchingCubeSurface_CreateVirtualMesh, Mesh)
{
	FindVoxelQueryData(FVoxelLODQueryData, LODQueryData);
	FindVoxelQueryData(FVoxelBoundsQueryData, BoundsQueryData);
	
	const TValue<FVoxelMarchingCubeSurface> Surface = Get(SurfacePin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, LODQueryData, BoundsQueryData, Surface)
	{
		if (Surface->Vertices.Num() == 0)
		{
			return {};
		}

		FVoxelQuery MaterialQuery = Query;
		MaterialQuery.Add<FVoxelSurfaceQueryData>().Surface = Surface;

		const TValue<FVoxelMeshMaterial> Material = Get(MaterialPin, MaterialQuery);

		return VOXEL_ON_COMPLETE(RenderThread, LODQueryData, BoundsQueryData, Surface, Material)
		{
			const FVoxelRDGExternalBufferRef Indices = FVoxelRDGExternalBuffer::Create(
				sizeof(uint32), 
				Surface->Indices.Num(), 
				PF_R32_UINT, 
				TEXT("Indices"));
			
			const FVoxelRDGExternalBufferRef VerticesX = FVoxelRDGExternalBuffer::Create(
				sizeof(float), 
				Surface->Vertices.X.Num(), 
				PF_R32_FLOAT, 
				TEXT("VerticesX"));
			
			const FVoxelRDGExternalBufferRef VerticesY = FVoxelRDGExternalBuffer::Create(
				sizeof(float), 
				Surface->Vertices.Y.Num(), 
				PF_R32_FLOAT, 
				TEXT("VerticesY"));
			
			const FVoxelRDGExternalBufferRef VerticesZ = FVoxelRDGExternalBuffer::Create(
				sizeof(float), 
				Surface->Vertices.Z.Num(), 
				PF_R32_FLOAT, 
				TEXT("VerticesZ"));

			AddCopyBufferPass(
				GraphBuilder,
				FVoxelRDGBuffer(Indices),
				Surface->Indices.GetGpuBuffer());

			AddCopyBufferPass(
				GraphBuilder,
				FVoxelRDGBuffer(VerticesX),
				Surface->Vertices.X.GetGpuBuffer());

			AddCopyBufferPass(
				GraphBuilder,
				FVoxelRDGBuffer(VerticesY),
				Surface->Vertices.Y.GetGpuBuffer());

			AddCopyBufferPass(
				GraphBuilder,
				FVoxelRDGBuffer(VerticesZ),
				Surface->Vertices.Z.GetGpuBuffer());

			const TSharedRef<FVoxelMarchingCubeMesh_VoxelVF> Mesh = MakeVoxelMesh<FVoxelMarchingCubeMesh_VoxelVF>();
			Mesh->LOD = LODQueryData->LOD;
			Mesh->Bounds = BoundsQueryData->Bounds.ShiftBy(-BoundsQueryData->Bounds.Min).ToFBox();
			Mesh->VoxelSize = Surface->ScaledVoxelSize;
			Mesh->MeshMaterial = Material;
			Mesh->Indices = Indices;
			Mesh->VerticesX = VerticesX;
			Mesh->VerticesY = VerticesY;
			Mesh->VerticesZ = VerticesZ;
			return Mesh;
		};
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UMaterialExpressionSampleVoxelDetailTexture::UMaterialExpressionSampleVoxelDetailTexture()
{
	Group = "Voxel Internal Parameters";
#if VOXEL_ENGINE_VERSION >= 501
	bHidePreviewWindow = true;
#endif
}

#if WITH_EDITOR
FExpressionInput* UMaterialExpressionSampleVoxelDetailTexture::GetInput(int32 InputIndex)
{
	return nullptr;
}

FName UMaterialExpressionSampleVoxelDetailTexture::GetInputName(int32 InputIndex) const
{
	return {};
}

FString UMaterialExpressionSampleVoxelDetailTexture::GetEditableName() const
{
	FString Name = Super::GetEditableName();
	Name.RemoveFromStart("VoxelDetailTextures_");
	Name.RemoveFromEnd("Texture");
	return Name;
}

void UMaterialExpressionSampleVoxelDetailTexture::SetEditableName(const FString& NewName)
{
	FString Name = NewName;
	Name.RemoveFromStart("VoxelDetailTextures_");
	Name.RemoveFromEnd("Texture");

	Super::SetEditableName("VoxelDetailTextures_" + Name + "Texture");
}

void UMaterialExpressionSampleVoxelDetailTexture::SetParameterName(const FName& InName)
{
	FString Name = InName.ToString();
	Name.RemoveFromStart("VoxelDetailTextures_");
	Name.RemoveFromEnd("Texture");

	Super::SetParameterName(FName("VoxelDetailTextures_" + Name + "Texture"));
}

int32 UMaterialExpressionSampleVoxelDetailTexture::Compile(FMaterialCompiler* Compiler, int32 OutputIndex)
{
	const int32 TextureArg = Compiler->TextureParameter(ParameterName, Texture, SamplerType);
	const int32 CoordinateArg = Compiler->TextureCoordinate(1, false, false);

	return Compiler->TextureSample(TextureArg, CoordinateArg, SAMPLERTYPE_Color);
}

void UMaterialExpressionSampleVoxelDetailTexture::GetCaption(TArray<FString>& OutCaptions) const
{
	FString Name = ParameterName.ToString();
	Name.RemoveFromStart("VoxelDetailTextures_");
	Name.RemoveFromEnd("Texture");

	OutCaptions.Add(TEXT("Voxel Detail Texture")); 
	OutCaptions.Add("'" + Name + "'");
}
#endif