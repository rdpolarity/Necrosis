// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFoliage.h"
#include "VoxelFoliageTemplatesData.h"
#include "VoxelFoliageInstanceTemplate.generated.h"

UCLASS()
class VOXELFOLIAGE_API UVoxelFoliageMesh_New : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	TObjectPtr<UStaticMesh> StaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance Settings", meta = (ShowOnlyInnerProperties, Override))
	FVoxelFoliageSettings InstanceSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision Settings")
	bool bOverrideCollisionSettings = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision Settings", meta = (EditCondition = "bOverrideCollisionSettings"))
	FBodyInstance Collision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale", meta = (InlineEditConditionToggle))
	bool bOverrideScaleSettings = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale", meta = (EditCondition = "bOverrideScaleSettings", Override))
	FVoxelFoliageScaleSettings ScaleSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation", meta = (ShowOnlyInnerProperties, Override))
	FVoxelFoliageRotationSettings RotationSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offset", meta = (ShowOnlyInnerProperties, Override))
	FVoxelFoliageOffsetSettings OffsetSettings;

	// Relative to the other strength in this array - they will be normalized
	// Has no impact if the array has only one element
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base", meta = (UIMin = 0, UIMax = 1, ClampMin = 0, ClampMax = 1))
	float Strength = 1.f;
};

struct VOXELFOLIAGE_API FVoxelFoliageMeshProxy
{
	const TWeakObjectPtr<UStaticMesh> StaticMesh;
	const FVoxelFoliageSettings InstanceSettings;
	const FBodyInstance BodyInstance;
	const FVoxelFoliageScaleSettings ScaleSettings;
	const FVoxelFoliageRotationSettings RotationSettings;
	const FVoxelFoliageOffsetSettings OffsetSettings;
	const float Strength;

	explicit FVoxelFoliageMeshProxy(
		const UVoxelFoliageMesh_New& FoliageMesh,
		const FVoxelFoliageSettings& InstanceSettings,
		const FBodyInstance& BodyInstance,
		const FVoxelFoliageScaleSettings& ScaleSettings,
		const FVoxelFoliageRotationSettings& RotationSettings,
		const FVoxelFoliageOffsetSettings& OffsetSettings)
		: StaticMesh(FoliageMesh.StaticMesh)
		, InstanceSettings(InstanceSettings, FoliageMesh.InstanceSettings)
		, BodyInstance(FoliageMesh.bOverrideCollisionSettings ? FoliageMesh.Collision : BodyInstance)
		, ScaleSettings(FoliageMesh.bOverrideScaleSettings ? FoliageMesh.ScaleSettings : ScaleSettings)
		, RotationSettings(RotationSettings, FoliageMesh.RotationSettings)
		, OffsetSettings(OffsetSettings, FoliageMesh.OffsetSettings)
		, Strength(FoliageMesh.Strength)
	{
	}
};

USTRUCT(BlueprintType)
struct VOXELFOLIAGE_API FVoxelFoliageMesh
{
	GENERATED_BODY()

	UPROPERTY()
	TSoftObjectPtr<UStaticMesh> Mesh_DEPRECATED;

	UPROPERTY()
	FVoxelFoliageSettings InstanceSettings_DEPRECATED;

	UPROPERTY()
	float Strength_DEPRECATED;
};

UCLASS(BlueprintType, meta = (VoxelAssetType, AssetSubMenu=Foliage, AssetColor=LightGreen))
class VOXELFOLIAGE_API UVoxelFoliageInstanceTemplate : public UObject
{
	GENERATED_BODY()

public:
	// The meshes to use - if you use multiple ones, the hits will be split among them based on their strength
	UPROPERTY()
	TArray<TObjectPtr<UVoxelFoliageMesh_New>> Meshes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel Foliage")
	float DistanceBetweenPoints = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Restrictions", meta = (ShowOnlyInnerProperties, NoOverrideCustomization))
	FVoxelFoliageSpawnRestriction SpawnRestriction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instances Settings", meta = (ShowOnlyInnerProperties))
	FVoxelFoliageSettings InstanceSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision Settings")
	FBodyInstance Collision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale")
	FVoxelFoliageScaleSettings ScaleSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation", meta = (ShowOnlyInnerProperties))
	FVoxelFoliageRotationSettings RotationSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offset", meta = (ShowOnlyInnerProperties))
	FVoxelFoliageOffsetSettings OffsetSettings;

	TArray<FVoxelFoliageMeshProxy> MakeMeshProxies() const
	{
		TArray<FVoxelFoliageMeshProxy> Result;
		for (const UVoxelFoliageMesh_New* Mesh : Meshes)
		{
			if (!Mesh)
			{
				continue;
			}

			Result.Add(FVoxelFoliageMeshProxy(*Mesh, InstanceSettings, Collision, ScaleSettings, RotationSettings, OffsetSettings));
		}
		return Result;
	}

	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;

private:
	bool bNeedsMigration = false;
};

struct VOXELFOLIAGE_API FVoxelFoliageInstanceTemplateProxy
{
	const TArray<FVoxelFoliageMeshProxy> Meshes;
	const float DistanceBetweenPoints;
	const FVoxelFoliageSpawnRestriction SpawnRestriction;
	const FVoxelFoliageScaleSettings ScaleSettings;
	const FVoxelFoliageRotationSettings RotationSettings;
	const FVoxelFoliageOffsetSettings OffsetSettings;

public:
	explicit FVoxelFoliageInstanceTemplateProxy(const UVoxelFoliageInstanceTemplate& Template)
		: Meshes(Template.MakeMeshProxies())
		, DistanceBetweenPoints(Template.DistanceBetweenPoints)
		, SpawnRestriction(Template.SpawnRestriction)
		, ScaleSettings(Template.ScaleSettings)
		, RotationSettings(Template.RotationSettings)
		, OffsetSettings(Template.OffsetSettings)
	{
	}

	FMatrix44f GetTransform(
		const int32 MeshIndex,
		const FVoxelFoliageRandomGenerator& RandomGenerator,
		const FVector3f& Position,
		const FVector3f& Normal,
		const FVector3f& WorldUp,
		const float ScaleMultiplier) const;
};