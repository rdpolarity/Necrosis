// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelFoliageInstanceTemplate.h"
#include "VoxelFoliageObjectVersion.h"
#include "VoxelFoliageRandomGenerator.h"

DEFINE_VOXEL_FACTORY(UVoxelFoliageInstanceTemplate);

void UVoxelFoliageInstanceTemplate::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FVoxelFoliageObjectVersion::GUID);

	if (Ar.CustomVer(FVoxelFoliageObjectVersion::GUID) >= FVoxelFoliageObjectVersion::FoliageInstanceStructureChanged)
	{
		return;
	}

	bNeedsMigration = true;
}

void UVoxelFoliageInstanceTemplate::PostLoad()
{
	Super::PostLoad();

	if (!bNeedsMigration)
	{
		return;
	}
	bNeedsMigration = false;

	Meshes = {};
}

FMatrix44f FVoxelFoliageInstanceTemplateProxy::GetTransform(
	const int32 MeshIndex,
	const FVoxelFoliageRandomGenerator& RandomGenerator,
	const FVector3f& Position,
	const FVector3f& Normal,
	const FVector3f& WorldUp,
	const float ScaleMultiplier) const
{
	const FVoxelFoliageMeshProxy& FoliageMesh = Meshes[MeshIndex];

	FMatrix44f Matrix = FRotationTranslationMatrix44f(
		FRotator3f(FoliageMesh.OffsetSettings.LocalRotationOffset),
		FVector3f(FoliageMesh.OffsetSettings.LocalPositionOffset));
	
	const FVector3f Scale = FoliageMesh.ScaleSettings.GetScale(RandomGenerator) * ScaleMultiplier;

	const float Yaw = FoliageMesh.RotationSettings.bRandomYaw ? RandomGenerator.GetYawRotationFraction() * 360.f : 0.f;
	const float Pitch = RandomGenerator.GetPitchRotationFraction() * FoliageMesh.RotationSettings.RandomPitchAngle;

	Matrix *= FScaleRotationTranslationMatrix44f(
		Scale,
		FRotator3f(Pitch, Yaw, 0.0f),
		FVector3f::ZeroVector);

	switch (FoliageMesh.RotationSettings.RotationAlignment)
	{
	default: check(false);
	case EVoxelFoliageRotation::AlignToSurface:
		Matrix *= FRotationMatrix44f::MakeFromZ(Normal);
		break;
	case EVoxelFoliageRotation::AlignToWorldUp:
		Matrix *= FRotationMatrix44f::MakeFromZ(WorldUp);
		break;
	case EVoxelFoliageRotation::RandomAlign:
		Matrix *= FRotationMatrix44f::MakeFromZ(RandomGenerator.GetRandomAlignUnitVector());
		break;
	}

	Matrix *= FTranslationMatrix44f(Position + FVector3f(FoliageMesh.OffsetSettings.GlobalPositionOffset));
	return Matrix;
}