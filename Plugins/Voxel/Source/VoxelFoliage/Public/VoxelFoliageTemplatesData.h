// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFoliageTemplatesData.generated.h"

class FVoxelFoliageRandomGenerator;

UENUM(BlueprintType)
enum class EVoxelFoliageScaling : uint8
{
	/** Instances will have uniform X, Y and Z scales */
	Uniform,
	/** Instances will have random X, Y and Z scales */
	Free,
	/** X and Y will be the same random scale, Z will be another */
	LockXY
};

USTRUCT(BlueprintType)
struct VOXELFOLIAGE_API FVoxelFoliageScaleSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale")
	EVoxelFoliageScaling Scaling = EVoxelFoliageScaling::Uniform;

	/** Specifies the range of scale, from minimum to maximum, to apply to an actor instance's X Scale property */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale")
	FVoxelFloatInterval ScaleX = { 1.0f, 1.0f };

	/** Specifies the range of scale, from minimum to maximum, to apply to an actor instance's Y Scale property */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale")
	FVoxelFloatInterval ScaleY = { 1.0f, 1.0f };

	/** Specifies the range of scale, from minimum to maximum, to apply to an actor instance's Z Scale property */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale")
	FVoxelFloatInterval ScaleZ = { 1.0f, 1.0f };

	FVector3f GetScale(const FVoxelFoliageRandomGenerator& RandomGenerator) const;
};

UENUM(BlueprintType)
enum class EVoxelFoliageRotation : uint8
{
	AlignToSurface,
	AlignToWorldUp,
	RandomAlign
};

USTRUCT(BlueprintType)
struct VOXELFOLIAGE_API FVoxelFoliageRotationSettings : public FVoxelOverridableSettings
{
	GENERATED_BODY()

	// Vertical to use for the instances
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation", meta = (Overridable))
	EVoxelFoliageRotation RotationAlignment = EVoxelFoliageRotation::AlignToWorldUp;

	// If selected, foliage instances will have a random yaw rotation around their vertical axis applied
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation", meta = (Overridable))
	bool bRandomYaw = true;

	// A random pitch adjustment can be applied to each instance, up to the specified angle in degrees, from the original vertical
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation", meta = (UIMin = 0, ClampMin = 0, UIMax = 180, ClampMax = 180, Units = "Degrees", Overridable))
	float RandomPitchAngle = 3;

	FVoxelFoliageRotationSettings() = default;
	FVoxelFoliageRotationSettings(const FVoxelFoliageRotationSettings& GlobalSettings, const FVoxelFoliageRotationSettings& OverridableSettings)
		: FVoxelFoliageRotationSettings(GlobalSettings)
	{
		CopyOverridenParameters(OverridableSettings);
	}
};

USTRUCT(BlueprintType)
struct VOXELFOLIAGE_API FVoxelFoliageOffsetSettings : public FVoxelOverridableSettings
{
	GENERATED_BODY()

	// Apply an offset to the instance position. Applied before the rotation. In cm
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offset", meta = (Overridable))
	FVector LocalPositionOffset = FVector::ZeroVector;

	// Apply an offset to the instance rotation. Applied after the local position offset, and before the rotation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offset", meta = (Overridable))
	FRotator LocalRotationOffset = FRotator::ZeroRotator;

	// Apply an offset to the instance position. Applied after the rotation. In cm
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Offset", meta = (Overridable))
	FVector GlobalPositionOffset = FVector::ZeroVector;

	FVoxelFoliageOffsetSettings() = default;
	FVoxelFoliageOffsetSettings(const FVoxelFoliageOffsetSettings& GlobalSettings, const FVoxelFoliageOffsetSettings& OverridableSettings)
		: FVoxelFoliageOffsetSettings(GlobalSettings)
	{
		CopyOverridenParameters(OverridableSettings);
	}
};

USTRUCT(BlueprintType)
struct VOXELFOLIAGE_API FVoxelFoliageSpawnRestriction : public FVoxelOverridableSettings
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Restrictions|Slope", meta = (Overridable))
	bool bEnableSlopeRestriction = false;
	
	// Min/max angle between object up vector and generator up vector in degrees
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Restrictions|Slope", meta = (EditCondition = "bEnableSlopeRestriction", UIMin = 0, ClampMin = 0, UIMax = 180, ClampMax = 180, Units = "Degrees", Overridable))
	FVoxelFloatInterval GroundSlopeAngle = { 0, 90 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Restrictions|Height", meta = (Overridable))
	bool bEnableHeightRestriction = false;
	
	// In voxels. Only spawn instances if the instance voxel Z position is in this interval.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Restrictions|Height", meta = (EditCondition = "bEnableHeightRestriction", Overridable))
	FVoxelFloatInterval HeightRestriction = { -100.f, 100.f };

	// In voxels, the size of the fade on the edges of HeightRestriction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Restrictions|Height", meta = (EditCondition = "bEnableHeightRestriction", UIMin = 0, ClampMin = 0, Overridable))
	float HeightRestrictionFalloff = 0.f;

	FVoxelFoliageSpawnRestriction() = default;
	FVoxelFoliageSpawnRestriction(const FVoxelFoliageSpawnRestriction& GlobalSettings, const FVoxelFoliageSpawnRestriction& OverridableSettings)
		: FVoxelFoliageSpawnRestriction(GlobalSettings)
	{
		CopyOverridenParameters(OverridableSettings);
	}

	bool CanSpawn(
		const FVoxelFoliageRandomGenerator& RandomGenerator,
		const FVector3f& Position,
		const FVector3f& Normal,
		const FVector3f& WorldUp) const;
};