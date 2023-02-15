// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMesh/VoxelMesh.h"
#include "VoxelRuntime/VoxelSubsystem.h"
#include "VoxelMeshRenderer.generated.h"

class UVoxelMeshComponent;

DECLARE_UNIQUE_VOXEL_ID(FVoxelMeshRendererId);

UCLASS()
class VOXELRUNTIME_API UVoxelMeshRendererProxy : public UVoxelSubsystemProxy
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(FVoxelMeshRenderer);
};

class VOXELRUNTIME_API FVoxelMeshRenderer : public IVoxelSubsystem
{
public:
	GENERATED_VOXEL_SUBSYSTEM_BODY(UVoxelMeshRendererProxy);

	FVoxelMeshRendererId CreateMesh(
		const FVector3d& Position,
		const TSharedRef<const FVoxelMesh>& Mesh);

	void DestroyMesh(FVoxelMeshRendererId Id);

private:
	TMap<FVoxelMeshRendererId, TWeakObjectPtr<UVoxelMeshComponent>> Components;
};