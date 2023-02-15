// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Toolkits/VoxelFoliageClusterTemplateEditorToolkit.h"

class UVoxelFoliageClusterTemplate;
class UVoxelFoliageInstanceTemplate;

class FVoxelFoliageEditorUtilities
{
public:
	struct FClusterRenderData
	{
		uint64 Seed = 0;
		EPreviewInstancesCount PreviewInstancesCount = EPreviewInstancesCount::Random;

		int32 MinOverallInstances = 0;
		int32 MaxOverallInstances = 0;

		int32 TotalNumInstances = 0;

		float FloorScale = 0.f;
		int32 MeshComponentsCount = 0;
	};

	static void RenderInstanceTemplate(
		const UVoxelFoliageInstanceTemplate& InstanceTemplate,
		FPreviewScene& PreviewScene,
		TArray<TWeakObjectPtr<UStaticMeshComponent>>& OutStaticMeshComponents);

	static void RenderClusterTemplate(
		const UVoxelFoliageClusterTemplate& InstanceTemplate,
		FPreviewScene& PreviewScene,
		FClusterRenderData& Data,
		TArray<TWeakObjectPtr<UInstancedStaticMeshComponent>>& OutMeshComponents);
};