// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMesh/VoxelMeshComponent.h"
#include "VoxelMesh/VoxelMeshComponentPool.h"
#include "VoxelMesh/VoxelMeshSceneProxy.h"
#include "VoxelMesh/VoxelMesh.h"

UVoxelMeshComponent::UVoxelMeshComponent()
{
	CastShadow = true;
	bUseAsOccluder = true;
}

void UVoxelMeshComponent::SetMesh(const TSharedPtr<const FVoxelMesh>& NewMesh)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	ensure(IsRegistered());

	Mesh = NewMesh;

	if (Mesh)
	{
		Mesh->CallInitialize_GameThread();
		Mesh->UpdateStats();

		SetRelativeScale3D(Mesh->GetScale());
	}
	
	MarkRenderStateDirty();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FPrimitiveSceneProxy* UVoxelMeshComponent::CreateSceneProxy()
{
	VOXEL_FUNCTION_COUNTER_LLM();

	if (!Mesh)
	{
		return nullptr;
	}

	return new FVoxelMeshSceneProxy(*this);
}

int32 UVoxelMeshComponent::GetNumMaterials() const
{
	return 1;
}

UMaterialInterface* UVoxelMeshComponent::GetMaterial(int32 Index) const
{
	if (!Mesh)
	{
		return nullptr;
	}

	return Mesh->GetMaterialSafe()->GetMaterial();
}

void UVoxelMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	if (!Mesh)
	{
		return;
	}

	OutMaterials.Add(Mesh->GetMaterialSafe()->GetMaterial());
}

FBoxSphereBounds UVoxelMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	const FBox LocalBounds = Mesh ? Mesh->GetBounds() : FBox(FVector::ZeroVector, FVector::ZeroVector);
	ensure(!LocalBounds.Min.ContainsNaN());
	ensure(!LocalBounds.Max.ContainsNaN());
	return LocalBounds.TransformBy(LocalToWorld);
}

void UVoxelMeshComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	VOXEL_FUNCTION_COUNTER();

	Super::OnComponentDestroyed(bDestroyingHierarchy);
	
	// Clear memory
	Mesh.Reset();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FMaterialRelevance UVoxelMeshComponent::GetMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const
{
	if (!Mesh)
	{
		return {};
	}

	const UMaterialInterface* Material = Mesh->GetMaterialSafe()->GetMaterial();
	if (!Material)
	{
		return {};
	}

	return Material->GetRelevance_Concurrent(InFeatureLevel);
}