// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "VoxelNavmeshComponent.generated.h"

struct FVoxelNavmesh;

UCLASS(BlueprintType, Blueprintable, ClassGroup = (Voxel), meta = (BlueprintSpawnableComponent))
class VOXELRUNTIME_API UVoxelNavmeshComponent final : public UPrimitiveComponent
{
	GENERATED_BODY()
	
public:
	UVoxelNavmeshComponent();
	
	void SetNavigationMesh(const TSharedPtr<const FVoxelNavmesh>& NewNavigationMesh);

	//~ Begin UPrimitiveComponent Interface.
	virtual bool ShouldCreatePhysicsState() const override { return false; }
	virtual bool IsNavigationRelevant() const override;
	virtual bool DoCustomNavigableGeometryExport(FNavigableGeometryExport& GeomExport) const override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	//~ End UPrimitiveComponent Interface.
	
private:
	TSharedPtr<const FVoxelNavmesh> NavigationMesh;
};