// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelFoliage.h"
#include "Render/VoxelFoliageComponent.h"

DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelFoliageDataMemory);
DEFINE_VOXEL_COUNTER(STAT_VoxelFoliageNumInstances);

FVoxelFoliageSettings::FVoxelFoliageSettings(const FVoxelFoliageSettings& GlobalSettings, const FVoxelFoliageSettings& OverridableSettings)
	: FVoxelFoliageSettings(GlobalSettings)
{
	MaterialOverrides = OverridableSettings.MaterialOverrides;
	CullDistance = FVoxelInt32Interval(OverridableSettings.CullDistance.Min * GlobalSettings.CullDistanceMultiplier, OverridableSettings.CullDistance.Max * GlobalSettings.CullDistanceMultiplier);
	CopyOverridenParameters(OverridableSettings);
}

void FVoxelFoliageSettings::ApplyToComponent(UVoxelFoliageComponent& Component) const
{
	VOXEL_FUNCTION_COUNTER();

	Component.OverrideMaterials = MaterialOverrides;
	Component.InstanceStartCullDistance = CullDistance.Min;
	Component.InstanceEndCullDistance = CullDistance.Max;
	Component.CastShadow = bCastShadow;
	Component.bAffectDynamicIndirectLighting = bAffectDynamicIndirectLighting;
	Component.bAffectDistanceFieldLighting = bAffectDistanceFieldLighting;
	Component.bCastShadowAsTwoSided = bCastShadowAsTwoSided;
	Component.bReceivesDecals = bReceivesDecals;
	Component.LightingChannels = LightingChannels;
	Component.bRenderCustomDepth = bRenderCustomDepth;
	Component.CustomDepthStencilValue = CustomDepthStencilValue;
}