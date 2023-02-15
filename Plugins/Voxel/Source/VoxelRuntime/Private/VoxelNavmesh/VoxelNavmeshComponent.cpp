// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelNavmesh/VoxelNavmeshComponent.h"
#include "VoxelNavmesh/VoxelNavmesh.h"
#include "AI/NavigationSystemBase.h"
#include "AI/NavigationSystemHelpers.h"

UVoxelNavmeshComponent::UVoxelNavmeshComponent()
{
	bCanEverAffectNavigation = true;
	bHasCustomNavigableGeometry = EHasCustomNavigableGeometry::EvenIfNotCollidable;
}

void UVoxelNavmeshComponent::SetNavigationMesh(const TSharedPtr<const FVoxelNavmesh>& NewNavigationMesh)
{
	VOXEL_FUNCTION_COUNTER();

	NavigationMesh = NewNavigationMesh;

	if (NavigationMesh)
	{
		NavigationMesh->UpdateStats();
	}

	if (IsRegistered() && 
		GetWorld() && 
		GetWorld()->GetNavigationSystem() && 
		FNavigationSystem::WantsComponentChangeNotifies())
	{
		VOXEL_SCOPE_COUNTER("UpdateComponentData");

		bNavigationRelevant = IsNavigationRelevant();
		FNavigationSystem::UpdateComponentData(*this);
	}
}

bool UVoxelNavmeshComponent::IsNavigationRelevant() const
{
	return NavigationMesh.IsValid();
}

bool UVoxelNavmeshComponent::DoCustomNavigableGeometryExport(FNavigableGeometryExport& GeomExport) const
{
	VOXEL_FUNCTION_COUNTER();

	if (NavigationMesh)
	{
		const TArray<FVector> DoubleVertices(NavigationMesh->Vertices);

		GeomExport.ExportCustomMesh(
			DoubleVertices.GetData(),
			NavigationMesh->Vertices.Num(),
			NavigationMesh->Indices.GetData(),
			NavigationMesh->Indices.Num(),
			GetComponentTransform());
	}

	return false;
}

FBoxSphereBounds UVoxelNavmeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	const FBox LocalBounds =
		NavigationMesh
		? NavigationMesh->Bounds
		: FBox(FVector::ZeroVector, FVector::ZeroVector);

	ensure(!LocalBounds.Min.ContainsNaN());
	ensure(!LocalBounds.Max.ContainsNaN());

	return LocalBounds.TransformBy(LocalToWorld);
}

void UVoxelNavmeshComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	VOXEL_FUNCTION_COUNTER();

	Super::OnComponentDestroyed(bDestroyingHierarchy);

	// Clear memory
	NavigationMesh.Reset();
}