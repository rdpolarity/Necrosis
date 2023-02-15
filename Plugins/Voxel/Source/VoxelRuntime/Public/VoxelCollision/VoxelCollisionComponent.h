// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "VoxelCollisionComponent.generated.h"

struct FVoxelCollider;

UCLASS()
class VOXELRUNTIME_API UVoxelCollisionComponent final : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	UVoxelCollisionComponent() = default;
	
	void SetCollider(const TSharedPtr<const FVoxelCollider>& NewCollider);
	
	//~ Begin UPrimitiveComponent Interface.
	virtual UBodySetup* GetBodySetup() override { return BodySetup; }
	virtual bool ShouldCreatePhysicsState() const override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual int32 GetNumMaterials() const override;
	virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const override;
	//~ End UPrimitiveComponent Interface.
	
private:
	UPROPERTY(Transient)
	TObjectPtr<UBodySetup> BodySetup;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> Materials;

	TSharedPtr<const FVoxelCollider> Collider;

	friend class FVoxelCollisionSceneProxy;
};