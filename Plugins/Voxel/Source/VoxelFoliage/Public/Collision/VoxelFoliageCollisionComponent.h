// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFoliageCollisionComponent.generated.h"

UCLASS()
class VOXELFOLIAGE_API UVoxelFoliageCollisionComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	UVoxelFoliageCollisionComponent() = default;

	//~ Begin UPrimitiveComponent Interface.
	virtual UBodySetup* GetBodySetup() override
	{
		if (StaticMesh)
		{
			return StaticMesh->GetBodySetup();
		}

		return nullptr;
	}
	virtual bool ShouldCreatePhysicsState() const override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	virtual void OnCreatePhysicsState() override;
	virtual void OnDestroyPhysicsState() override;
	//~ End UPrimitiveComponent Interface.

	void SetStaticMesh(UStaticMesh* Mesh);
	void AssignInstances(const TVoxelArray<FTransform3f>& Transforms);
	int32 GetInstancesCount() const { return Instances.Num(); }

public:
	void CreateAllInstanceBodies();
	void ClearAllInstanceBodies();

	//~ Begin UPrimitiveComponent Interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	//~ End UPrimitiveComponent Interface.

private:
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> StaticMesh = nullptr;

	TArray<FMatrix> Instances;
	TArray<FBodyInstance*> InstanceBodies;

	friend class FVoxelFoliageCollisionSceneProxy;
};