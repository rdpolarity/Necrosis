// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelPointCanvasAsset.h"
#include "VoxelPointCanvasData.h"
#include "VoxelMeshVoxelizerLibrary.h"

DEFINE_VOXEL_FACTORY(UVoxelPointCanvasAsset);

void UVoxelPointCanvasAsset::InitializeDefault()
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_USE_NAMESPACE(PointCanvas);

	MarkPackageDirty();

	Data = MakeShared<FVoxelPointCanvasData>();

	const UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/EngineMeshes/Cube.Cube"));
	if (!ensure(Mesh))
	{
		return;
	}

	FVoxelMeshVoxelizerInputData MeshData = UVoxelMeshVoxelizerLibrary::CreateMeshDataFromStaticMesh(*Mesh);

	for (int32 LOD = 0; LOD < 10; LOD++)
	{
		const TSharedRef<FLODData> LODData = MakeShared<FLODData>();
		Data->LODs.Add(LODData);

		FVoxelScopeLock_Write Lock(LODData->CriticalSection);

		TVoxelArray<FVector3f> Vertices;
		for (const int32 Index : MeshData.Indices)
		{
			Vertices.Add(MeshData.Vertices[Index] / 10.f / (1 << LOD));
		}
		LODData->AddPoints(Vertices);
	}
}

void UVoxelPointCanvasAsset::Edit()
{
	VOXEL_FUNCTION_COUNTER();

	Data->GetFirstLOD().EditPoints(
		[](const FVoxelIntBox& Bounds)
		{
			return true;
		},
		[=](const TVoxelArrayView<FVector3f> Points)
		{
			for (FVector3f& Point : Points)
			{
				Point = FRotationMatrix44f(FRotator3f(0, Point.Z, 0)).TransformPosition(Point);
			}
		});
}

void UVoxelPointCanvasAsset::PostLoad()
{
	VOXEL_FUNCTION_COUNTER_LLM();

	Super::PostLoad();

	if (!Data)
	{
		Data = MakeShared<FVoxelPointCanvasData>();
	}
}

void UVoxelPointCanvasAsset::Serialize(FArchive& Ar)
{
	VOXEL_FUNCTION_COUNTER_LLM();
	
	Super::Serialize(Ar);

	if (!Data)
	{
		Data = MakeShared<FVoxelPointCanvasData>();
	}

	if (bCompress)
	{
		BulkData.SetBulkDataFlags(BULKDATA_SerializeCompressed);
	}
	else
	{
		BulkData.ClearBulkDataFlags(BULKDATA_SerializeCompressed);
	}

	FVoxelObjectUtilities::SerializeBulkData(this, BulkData, Ar, *Data);
}