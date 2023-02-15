// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMeshVoxelizerLibrary.h"

DECLARE_VOXEL_MEMORY_STAT(VOXELLANDMASS_API, STAT_VoxelLandmassBrushData, "Voxel Landmass Brush Data Memory");

class VOXELLANDMASS_API FVoxelLandmassBrushData
{
public:
	FBox3f MeshBounds = FBox3f(FVector3f(0.f), FVector3f(0.f));
	float VoxelSize = 0;
	float MaxSmoothness = 0;
	FVoxelMeshVoxelizerSettings VoxelizerSettings;

	FVector3f Origin = FVector3f::ZeroVector;
	FIntVector Size = FIntVector::ZeroValue;
	TVoxelArray<float> DistanceField;
	TVoxelArray<FVoxelOctahedron> Normals;

	FVoxelLandmassBrushData() = default;

	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelLandmassBrushData);

	int64 GetAllocatedSize() const;
	void Serialize(FArchive& Ar);

	TSharedRef<FVoxelRDGExternalBuffer> GetDistanceField_RenderThread() const;
	TSharedRef<FVoxelRDGExternalBuffer> GetNormals_RenderThread() const;

	static TSharedPtr<FVoxelLandmassBrushData> VoxelizeMesh(
		const UStaticMesh& Mesh,
		float VoxelSize,
		float MaxSmoothness,
		const FVoxelMeshVoxelizerSettings& VoxelizerSettings);

private:
	mutable TSharedPtr<FVoxelRDGExternalBuffer> DistanceField_RenderThread;
	mutable TSharedPtr<FVoxelRDGExternalBuffer> Normals_RenderThread;
};