// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "VoxelMeshComponent.generated.h"

struct FVoxelMesh;

UCLASS(BlueprintType, Blueprintable, ClassGroup = (Voxel), meta = (BlueprintSpawnableComponent))
class VOXELRUNTIME_API UVoxelMeshComponent final : public UPrimitiveComponent
{
	GENERATED_BODY()
	
public:
	UVoxelMeshComponent();

	void SetMesh(const TSharedPtr<const FVoxelMesh>& NewMesh);
	
public:
	//~ Begin UPrimitiveComponent Interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual bool ShouldCreatePhysicsState() const override { return false; }

	virtual int32 GetNumMaterials() const override;
	virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const override;
	
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	//~ End UPrimitiveComponent Interface.

	FMaterialRelevance GetMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const;	
	
private:
	TSharedPtr<const FVoxelMesh> Mesh;
	
	friend class FVoxelMeshSceneProxy;
};