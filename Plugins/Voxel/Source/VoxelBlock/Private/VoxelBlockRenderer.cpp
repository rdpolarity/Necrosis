// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelBlockRenderer.h"
#include "VoxelBlockRegistry.h"
#include "VoxelBlockUtilities.h"

DEFINE_VOXEL_SUBSYSTEM(FVoxelBlockRenderer);

void FVoxelBlockRenderer::Initialize()
{
	Super::Initialize();

	if (!FVoxelBlockUtilities::BuildTextures(
		GetSubsystem<FVoxelBlockRegistry>().GetBlockAssets(),
		ScalarIds,
		FacesIds,
		Textures,
		TextureArrays))
	{
		return;
	}
	ensure(ScalarIds.Num() == FacesIds.Num());

	InstanceParameters = FVoxelBlockUtilities::GetInstanceParameters(Textures, TextureArrays);

	for (const auto& It : GetSubsystem<FVoxelBlockRegistry>().GetBlockAssets())
	{
		TextureSize = It.Value->TextureSize;
		break;
	}
}

void FVoxelBlockRenderer::AddReferencedObjects(FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(Collector);

	Collector.AddReferencedObjects(Textures);
	Collector.AddReferencedObjects(TextureArrays);
}