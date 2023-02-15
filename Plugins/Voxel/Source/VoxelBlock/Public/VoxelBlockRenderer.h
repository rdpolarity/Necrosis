// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBlockTypes.h"
#include "VoxelRuntime/VoxelSubsystem.h"
#include "VoxelBlockRenderer.generated.h"

class FVoxelCookedPhysicsMesh;

UCLASS()
class VOXELBLOCK_API UVoxelBlockRendererProxy : public UVoxelSubsystemProxy
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(FVoxelBlockRenderer);
};

class VOXELBLOCK_API FVoxelBlockRenderer : public IVoxelSubsystem
{
public:
	GENERATED_VOXEL_SUBSYSTEM_BODY(UVoxelBlockRendererProxy);

	//~ Begin IVoxelSubsystem Interface
	virtual void Initialize() override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	//~ End IVoxelSubsystem Interface

	FORCEINLINE const FVoxelMaterialParameters& GetInstanceParameters() const
	{
		return InstanceParameters;
	}
	FORCEINLINE int32 GetTextureSize() const
	{
		return TextureSize;
	}
	FORCEINLINE FColor GetPackedColor(FVoxelBlockId Id, EVoxelBlockFace Face) const
	{
		ensureVoxelSlow(ScalarIds.Contains(Id));
		ensureVoxelSlow(FacesIds.Contains(Id));

		int32 ScalarId = ScalarIds.FindRef(Id);
		int32 FaceId = FacesIds.FindRef(Id).GetId(Face);

		ScalarId = FMath::Clamp(ScalarId, 0, 0xFFFF);
		FaceId = FMath::Clamp(FaceId, 0, 0xFFFF);

		FColor Color;
		Color.R = uint16(ScalarId) & 0xFF;
		Color.G = uint16(ScalarId) >> 8;
		Color.B = uint16(FaceId) & 0xFF;
		Color.A = uint16(FaceId) >> 8;

		return Color;
	}

private:
	FVoxelMaterialParameters InstanceParameters;

	int32 TextureSize = 1;
	TMap<FVoxelBlockId, int32> ScalarIds;
	TMap<FVoxelBlockId, FVoxelBlockFaceIds> FacesIds;

	TMap<FName, TObjectPtr<UTexture2D>> Textures;
	TMap<FName, TObjectPtr<UTexture2DArray>> TextureArrays;
};