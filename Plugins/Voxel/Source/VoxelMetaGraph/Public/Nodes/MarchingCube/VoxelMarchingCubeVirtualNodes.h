// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Nodes/MarchingCube/VoxelMarchingCubeNodes.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "VoxelMarchingCubeVirtualNodes.generated.h"

USTRUCT(Category = "Mesh|MarchingCube")
struct VOXELMETAGRAPH_API FVoxelNode_FVoxelMarchingCubeSurface_CreateVirtualMesh : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelMarchingCubeSurface, Surface, nullptr);
	VOXEL_INPUT_PIN(FVoxelMeshMaterial, Material, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelMesh, Mesh);
};

UCLASS()
class VOXELMETAGRAPH_API UMaterialExpressionSampleVoxelDetailTexture : public UMaterialExpressionTextureSampleParameter2D
{
	GENERATED_BODY()

public:
	UMaterialExpressionSampleVoxelDetailTexture();

#if WITH_EDITOR
	//~ Begin UMaterialExpression Interface
	virtual FExpressionInput* GetInput(int32 InputIndex) override;
	virtual FName GetInputName(int32 InputIndex) const override;
	
	virtual FString GetEditableName() const override;
	virtual void SetEditableName(const FString& NewName) override;

	virtual void SetParameterName(const FName& Name) override;

	virtual int32 Compile(FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	//~ End UMaterialExpression Interface
#endif
};