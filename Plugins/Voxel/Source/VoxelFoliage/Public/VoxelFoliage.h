// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFoliage.generated.h"

class UVoxelFoliageComponent;

DECLARE_VOXEL_MEMORY_STAT(VOXELFOLIAGE_API, STAT_VoxelFoliageDataMemory, "Voxel Foliage Data Memory");
DECLARE_VOXEL_COUNTER(VOXELFOLIAGE_API, STAT_VoxelFoliageNumInstances, "Num Foliage Instances");

struct FVoxelFoliageBuiltData;

struct VOXELFOLIAGE_API FVoxelFoliageData
{
	TSharedPtr<TVoxelArray<FTransform3f>> Transforms;
	TVoxelArray<TVoxelArray<float>> CustomDatas;
	mutable TSharedPtr<FVoxelFoliageBuiltData> BuiltData;

	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelFoliageDataMemory);
	VOXEL_COUNTER_HELPER(STAT_VoxelFoliageNumInstances, NumInstances);

	int64 GetAllocatedSize() const
	{
		VOXEL_CONST_CAST(NumInstances) = Transforms->Num();

		int64 AllocatedSize = Transforms->GetAllocatedSize();
		for (const TVoxelArray<float>& CustomData : CustomDatas)
		{
			AllocatedSize += CustomData.GetAllocatedSize();
		}
		return AllocatedSize;
	}
};

USTRUCT(BlueprintType)
struct VOXELFOLIAGE_API FVoxelFoliageSettings : public FVoxelOverridableSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ShowOverride = IfOverriden))
	TArray<TObjectPtr<UMaterialInterface>> MaterialOverrides;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ShowOverride = IfGlobal))
	float CullDistanceMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ShowOverride = IfOverriden))
	FVoxelInt32Interval CullDistance = { 100000, 200000 };

	// Controls whether the foliage should cast a shadow or not
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (Overridable))
	bool bCastShadow = true;

	// This flag is only used if CastShadow is true
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Settings", meta = (EditCondition = "bCastShadow", Overridable))
	bool bAffectDynamicIndirectLighting = false;

	// Controls whether the primitive should affect dynamic distance field lighting methods.  This flag is only used if CastShadow is true
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Settings", meta = (EditCondition = "bCastShadow", Overridable))
	bool bAffectDistanceFieldLighting = false;

	// Whether this foliage should cast dynamic shadows as if it were a two sided material
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Settings", meta = (EditCondition = "bCastShadow", Overridable))
	bool bCastShadowAsTwoSided = false;

	// Whether the foliage receives decals
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Settings", meta = (Overridable))
	bool bReceivesDecals = true;

	// Lighting channels that placed foliage will be assigned. Lights with matching channels will affect the foliage.
	// These channels only apply to opaque materials, direct lighting, and dynamic lighting and shadowing.
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Settings", meta = (Overridable))
	FLightingChannels LightingChannels;

	// If true, the foliage will be rendered in the CustomDepth pass (usually used for outlines)
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Settings", meta = (DisplayName = "Render CustomDepth Pass", Overridable))
	bool bRenderCustomDepth = false;

	// Optionally write this 0-255 value to the stencil buffer in CustomDepth pass (Requires project setting or r.CustomDepth == 3)
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Settings", meta = (UIMin = "0", UIMax = "255", EditCondition = "bRenderCustomDepth", DisplayName = "CustomDepth Stencil Value", Overridable))
	int32 CustomDepthStencilValue = 0;

	FVoxelFoliageSettings() = default;
	FVoxelFoliageSettings(const FVoxelFoliageSettings& GlobalSettings, const FVoxelFoliageSettings& OverridableSettings);

	void ApplyToComponent(UVoxelFoliageComponent& Component) const;
};