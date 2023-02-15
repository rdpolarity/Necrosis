// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelDensityCanvasAsset.h"
#include "VoxelDensityCanvasData.h"

DEFINE_VOXEL_FACTORY(UVoxelDensityCanvasAsset);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelDensityCanvasAsset::ClearData()
{
	VOXEL_FUNCTION_COUNTER_LLM();

	GetData()->ClearData();
}

void UVoxelDensityCanvasAsset::AddSphere(
	const FTransform VoxelActorTransform,
	const FVector Position,
	const float Radius)
{
	VOXEL_FUNCTION_COUNTER_LLM();

	GetData()->SphereEdit(
		FVector3f(VoxelActorTransform.InverseTransformPosition(Position) / VoxelSize),
		Radius / VoxelSize,
		0.f,
		0.f,
		false,
		true);
}

void UVoxelDensityCanvasAsset::RemoveSphere(
	const FTransform VoxelActorTransform,
	const FVector Position,
	const float Radius)
{
	VOXEL_FUNCTION_COUNTER_LLM();

	GetData()->SphereEdit(
		FVector3f(VoxelActorTransform.InverseTransformPosition(Position) / VoxelSize),
		Radius / VoxelSize,
		0.f,
		0.f,
		false,
		false);
}

void UVoxelDensityCanvasAsset::SmoothAdd(
	const FTransform VoxelActorTransform,
	const FVector Position,
	const float Radius,
	const float Falloff,
	const float Strength)
{
	VOXEL_FUNCTION_COUNTER_LLM();

	GetData()->SphereEdit(
		FVector3f(VoxelActorTransform.InverseTransformPosition(Position) / VoxelSize),
		Radius / VoxelSize,
		Falloff,
		Strength,
		true,
		true);
}

void UVoxelDensityCanvasAsset::SmoothRemove(
	const FTransform VoxelActorTransform,
	const FVector Position,
	const float Radius,
	const float Falloff,
	const float Strength)
{
	VOXEL_FUNCTION_COUNTER_LLM();

	GetData()->SphereEdit(
		FVector3f(VoxelActorTransform.InverseTransformPosition(Position) / VoxelSize),
		Radius / VoxelSize,
		Falloff,
		Strength,
		true,
		false);
}

UVoxelDensityCanvasAsset* UVoxelDensityCanvasAsset::MakeDensityCanvasAsset(float CanvasVoxelSize)
{
	VOXEL_FUNCTION_COUNTER_LLM();

	UVoxelDensityCanvasAsset* Asset = NewObject<UVoxelDensityCanvasAsset>();
	Asset->VoxelSize = CanvasVoxelSize;
	return Asset;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelDensityCanvasAsset::PostLoad()
{
	VOXEL_FUNCTION_COUNTER_LLM();

	Super::PostLoad();

	if (!Data)
	{
		Data = MakeShared<FData>();
	}
}

void UVoxelDensityCanvasAsset::Serialize(FArchive& Ar)
{
	VOXEL_FUNCTION_COUNTER_LLM();
	
	Super::Serialize(Ar);

	if (!Data)
	{
		Data = MakeShared<FData>();
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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<UVoxelDensityCanvasAsset::FData> UVoxelDensityCanvasAsset::GetData() const
{
	if (!ensure(Data))
	{
		VOXEL_CONST_CAST(this)->Data = MakeShared<FData>();
	}
	Data->VoxelSize = VoxelSize;
	return Data.ToSharedRef();
}