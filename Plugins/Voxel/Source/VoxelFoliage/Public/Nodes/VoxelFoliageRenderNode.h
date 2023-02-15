// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFoliage.h"
#include "VoxelExecNode.h"
#include "Nodes/VoxelFoliageNodes.h"
#include "Render/VoxelFoliageRenderer.h"
#include "VoxelFoliageRenderNode.generated.h"

USTRUCT()
struct VOXELFOLIAGE_API FVoxelChunkExecObject_CreateFoliageMeshComponent : public FVoxelChunkExecObject
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	struct FTemplateData
	{
		TWeakObjectPtr<UStaticMesh> StaticMesh;
		FVoxelFoliageSettings FoliageSettings;
		TSharedPtr<const FVoxelFoliageData> FoliageData;
	};

	FVector Position = FVector::Zero();
	TArray<FTemplateData> TemplatesData;

	virtual void Create(FVoxelRuntime& Runtime) const override;
	virtual void Destroy(FVoxelRuntime& Runtime) const override;

private:
	mutable TSet<FVoxelFoliageRendererId> FoliageIds;
};

USTRUCT(Category = "Foliage")
struct VOXELFOLIAGE_API FVoxelChunkExecNode_CreateFoliageMeshComponent : public FVoxelChunkExecNode_Default
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelFoliageChunkData, ChunkData, nullptr);
	VOXEL_INPUT_PIN_ARRAY(FVoxelFloatBuffer, CustomData, nullptr, 1);

	virtual TValue<FVoxelChunkExecObject> Execute(const FVoxelQuery& Query) const override;
	virtual TValue<FVoxelChunkExecObject> RenderFoliage(const FVoxelQuery& Query, const TSharedRef<FVoxelChunkExecObject_CreateFoliageMeshComponent>& Object) const;
};