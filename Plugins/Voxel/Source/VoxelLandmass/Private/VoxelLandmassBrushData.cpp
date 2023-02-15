// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelLandmassBrushData.h"
#include "VoxelMeshVoxelizer.h"
#include "VoxelMeshVoxelizerLibrary.h"
#include "VoxelDistanceFieldUtilities_Old.h"

DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelLandmassBrushData);

int64 FVoxelLandmassBrushData::GetAllocatedSize() const
{
	int64 AllocatedSize = sizeof(*this);
	AllocatedSize += DistanceField.GetAllocatedSize();
	return AllocatedSize;
}

void FVoxelLandmassBrushData::Serialize(FArchive& Ar)
{
	VOXEL_FUNCTION_COUNTER();
		
	using FVersion = DECLARE_VOXEL_VERSION
	(
		FirstVersion,
		RemoveMaxSmoothness,
		AddRanges,
		AddRangeMips,
		RemoveRangeMips,
		RemoveSerializedRangeChunkSize,
		AddMaxSmoothness,
		AddNormals
	);

	int32 Version = FVersion::LatestVersion;
	Ar << Version;

	if (Version != FVersion::LatestVersion)
	{
		return;
	}

	Ar << MeshBounds;
	Ar << VoxelSize;
	Ar << MaxSmoothness;
	Ar << VoxelizerSettings;
		
	Ar << Origin;
	Ar << Size;
	DistanceField.BulkSerialize(Ar);
	Normals.BulkSerialize(Ar);

	UpdateStats();
}

TSharedRef<FVoxelRDGExternalBuffer> FVoxelLandmassBrushData::GetDistanceField_RenderThread() const
{
	check(IsInRenderingThread());

	if (!DistanceField_RenderThread)
	{
		DistanceField_RenderThread = FVoxelRDGExternalBuffer::CreateTyped<float>(DistanceField, PF_R32_FLOAT, TEXT("DistanceField"));
	}

	return DistanceField_RenderThread.ToSharedRef();
}

TSharedRef<FVoxelRDGExternalBuffer> FVoxelLandmassBrushData::GetNormals_RenderThread() const
{
	check(IsInRenderingThread());

	if (!Normals_RenderThread)
	{
		Normals_RenderThread = FVoxelRDGExternalBuffer::CreateTyped<FVoxelOctahedron>(Normals, PF_R8G8, TEXT("Normals"));
	}

	return Normals_RenderThread.ToSharedRef();
}

TSharedPtr<FVoxelLandmassBrushData> FVoxelLandmassBrushData::VoxelizeMesh(
	const UStaticMesh& Mesh,
	float VoxelSize,
	float MaxSmoothness,
	const FVoxelMeshVoxelizerSettings& VoxelizerSettings)
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelMeshVoxelizerInputData MeshData = UVoxelMeshVoxelizerLibrary::CreateMeshDataFromStaticMesh(Mesh);

	FBox3f MeshBounds(ForceInit);
	for (FVector3f& Vertex : MeshData.Vertices)
	{
		Vertex /= VoxelSize;
		MeshBounds += Vertex;
	}

	const FBox3f MeshBoundsWithSmoothness = MeshBounds.ExpandBy(MeshBounds.GetSize().GetMax() * MaxSmoothness);

	const FIntVector Size = FVoxelUtilities::CeilToInt(MeshBoundsWithSmoothness.GetSize());
	const FVector3f Origin = MeshBoundsWithSmoothness.Min;

	if (int64(Size.X) * int64(Size.Y) * int64(Size.Z) * sizeof(float) >= MAX_int32 / 2)
	{
		VOXEL_MESSAGE(Error, "{0}: Voxelized mesh would have more than 1GB of data", Mesh);
		return nullptr;
	}
	if (Size.X * Size.Y * Size.Z == 0)
	{
		VOXEL_MESSAGE(Error, "{0}: Size = 0", Mesh);
		return nullptr;
	}

	TVoxelArray<float> Distances;
	TVoxelArray<FVector3f> SurfacePositions;
	TVoxelArray<FVector3f> VoxelNormals;
	int32 NumLeaks = 0;
	Voxel::MeshVoxelizer::Voxelize(
		VoxelizerSettings,
		MeshData.Vertices,
		MeshData.Indices,
		Origin,
		Size,
		Distances,
		SurfacePositions,
		&MeshData.VertexNormals,
		&VoxelNormals,
		&NumLeaks);

	// Propagate distances
	FVoxelDistanceFieldUtilities::JumpFlood(Size, SurfacePositions, EVoxelComputeDevice::CPU, true);
	FVoxelDistanceFieldUtilities::GetDistancesFromSurfacePositions(Size, SurfacePositions, Distances);
	
	// Propagate normals
	for (int32 Index = 0; Index < VoxelNormals.Num(); Index++)
	{
		if (!VoxelNormals[Index].IsZero())
		{
			continue;
		}

		const FVector3f SurfacePosition = SurfacePositions[Index];

		const FVector3f Alpha = SurfacePosition - FVector3f(FVoxelUtilities::FloorToInt(SurfacePosition));

		const FIntVector Min = FVoxelUtilities::Clamp(FVoxelUtilities::FloorToInt(SurfacePosition), FIntVector::ZeroValue, Size - 1);
		const FIntVector Max = FVoxelUtilities::Clamp(FVoxelUtilities::CeilToInt(SurfacePosition), FIntVector::ZeroValue, Size - 1);

		const FVector3f Normal000 = VoxelNormals[FVoxelUtilities::Get3DIndex(Size, Min.X, Min.Y, Min.Z)];
		const FVector3f Normal001 = VoxelNormals[FVoxelUtilities::Get3DIndex(Size, Max.X, Min.Y, Min.Z)];
		const FVector3f Normal010 = VoxelNormals[FVoxelUtilities::Get3DIndex(Size, Min.X, Max.Y, Min.Z)];
		const FVector3f Normal011 = VoxelNormals[FVoxelUtilities::Get3DIndex(Size, Max.X, Max.Y, Min.Z)];
		const FVector3f Normal100 = VoxelNormals[FVoxelUtilities::Get3DIndex(Size, Min.X, Min.Y, Max.Z)];
		const FVector3f Normal101 = VoxelNormals[FVoxelUtilities::Get3DIndex(Size, Max.X, Min.Y, Max.Z)];
		const FVector3f Normal110 = VoxelNormals[FVoxelUtilities::Get3DIndex(Size, Min.X, Max.Y, Max.Z)];
		const FVector3f Normal111 = VoxelNormals[FVoxelUtilities::Get3DIndex(Size, Max.X, Max.Y, Max.Z)];

		ensure(!Normal000.IsZero());
		ensure(!Normal001.IsZero());
		ensure(!Normal010.IsZero());
		ensure(!Normal011.IsZero());
		ensure(!Normal100.IsZero());
		ensure(!Normal101.IsZero());
		ensure(!Normal110.IsZero());
		ensure(!Normal111.IsZero());

		VoxelNormals[Index] = FVoxelUtilities::TrilinearInterpolation(
			Normal000,
			Normal001,
			Normal010,
			Normal011,
			Normal100,
			Normal101,
			Normal110,
			Normal111,
			Alpha.X,
			Alpha.Y,
			Alpha.Z).GetSafeNormal();
	}

	const TSharedRef<FVoxelLandmassBrushData> BrushData = MakeShared<FVoxelLandmassBrushData>();
	BrushData->MeshBounds = MeshBounds;
	BrushData->VoxelSize = VoxelSize;
	BrushData->MaxSmoothness = MaxSmoothness;
	BrushData->VoxelizerSettings = VoxelizerSettings;

	BrushData->Origin = Origin;
	BrushData->Size = Size;
	BrushData->DistanceField = MoveTemp(Distances);
	BrushData->Normals = TVoxelArray<FVoxelOctahedron>(VoxelNormals);
	BrushData->UpdateStats();

	LOG_VOXEL(Log, "%s voxelized, %d leaks", *Mesh.GetName(), NumLeaks);

	return BrushData;
}