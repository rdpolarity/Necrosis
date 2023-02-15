// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelDeveloperSettings.h"
#include "VoxelLandmassSettings.generated.h"

UCLASS(config = Engine, defaultconfig, meta = (DisplayName = "Voxel Landmass"))
class VOXELLANDMASS_API UVoxelLandmassSettings : public UVoxelDeveloperSettings
{
	GENERATED_BODY()

public:
	UVoxelLandmassSettings()
	{
		SectionName = "Voxel Landmass";
	}
	
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Mesh Brush")
	float BaseVoxelSize = 100.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	TSoftObjectPtr<UMaterialInterface> MeshMaterial = MakeSoftObjectPtr<UMaterialInterface>("/Engine/Engine_MI_Shaders/Instances/M_Shader_SimpleTranslucent_Invis.M_Shader_SimpleTranslucent_Invis");

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	TSoftObjectPtr<UMaterialInterface> InvertedMeshMaterial = MakeSoftObjectPtr<UMaterialInterface>("/Engine/EngineDebugMaterials/M_SimpleTranslucent.M_SimpleTranslucent");

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	TSoftObjectPtr<UMaterialInterface> HeightmapMaterial = MakeSoftObjectPtr<UMaterialInterface>("/Engine/EngineDebugMaterials/M_SimpleTranslucent.M_SimpleTranslucent");
};