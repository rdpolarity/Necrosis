// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Collision/VoxelFoliageCollisionComponent.h"

VOXEL_CONSOLE_VARIABLE(
	VOXELFOLIAGE_API, bool, GVoxelFoliageShowInstancesCollisions, true,
	"voxel.foliage.ShowInstancesCollisions",
	"");

bool UVoxelFoliageCollisionComponent::ShouldCreatePhysicsState() const
{
	return Instances.Num() > 0;
}

FBoxSphereBounds UVoxelFoliageCollisionComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	if (!StaticMesh ||
		Instances.Num() == 0)
	{
		return FBoxSphereBounds(LocalToWorld.GetLocation(), FVector::ZeroVector, 0.f);
	}

	const FMatrix BoundTransformMatrix = LocalToWorld.ToMatrixWithScale();

	const FBoxSphereBounds RenderBounds = StaticMesh->GetBounds();
	FBoxSphereBounds NewBounds = RenderBounds.TransformBy(Instances[0] * BoundTransformMatrix);

	for (int32 InstanceIndex = 1; InstanceIndex < Instances.Num(); InstanceIndex++)
	{
		NewBounds = NewBounds + RenderBounds.TransformBy(Instances[InstanceIndex] * BoundTransformMatrix);
	}

	return NewBounds;
}

void UVoxelFoliageCollisionComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	VOXEL_FUNCTION_COUNTER();

	Super::OnComponentDestroyed(bDestroyingHierarchy);

	ClearAllInstanceBodies();
}

void UVoxelFoliageCollisionComponent::OnCreatePhysicsState()
{
	if (Instances.Num() != InstanceBodies.Num())
	{
		CreateAllInstanceBodies();
	}

	// We want to avoid PrimitiveComponent base body instance at component location
	USceneComponent::OnCreatePhysicsState();

	UE_501_ONLY(OnComponentPhysicsStateChanged.Broadcast(this, EComponentPhysicsStateChange::Created);)
}

void UVoxelFoliageCollisionComponent::OnDestroyPhysicsState()
{
	ClearAllInstanceBodies();

#if UE_ENABLE_DEBUG_DRAWING
	SendRenderDebugPhysics();
#endif

	// We want to avoid PrimitiveComponent base body instance at component location
	USceneComponent::OnDestroyPhysicsState();

	UE_501_ONLY(OnComponentPhysicsStateChanged.Broadcast(this, EComponentPhysicsStateChange::Destroyed);)
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelFoliageCollisionComponent::SetStaticMesh(UStaticMesh* Mesh)
{
	StaticMesh = Mesh;
}

void UVoxelFoliageCollisionComponent::AssignInstances(const TVoxelArray<FTransform3f>& Transforms)
{
	check(InstanceBodies.Num() == 0);

	Instances.Reserve(Transforms.Num());
	Instances.AddUninitialized(Transforms.Num());

	for (int32 Index = 0; Index < Transforms.Num(); Index++)
	{
		Instances[Index] = FTransform(Transforms[Index]).ToMatrixWithScale();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelFoliageCollisionComponent::CreateAllInstanceBodies()
{
	VOXEL_FUNCTION_COUNTER();
	check(InstanceBodies.Num() == 0);

	FPhysScene* PhysicsScene = GetWorld()->GetPhysicsScene();
	if (!PhysicsScene)
	{
		return;
	}

	UBodySetup* BodySetup = GetBodySetup();
	if (!BodySetup)
	{
		InstanceBodies.AddZeroed(Instances.Num());
		return;
	}

	if (!BodyInstance.GetOverrideWalkableSlopeOnInstance())
	{
		BodyInstance.SetWalkableSlopeOverride(BodySetup->WalkableSlopeOverride, false);
	}

	InstanceBodies.Reserve(Instances.Num());
	InstanceBodies.SetNumUninitialized(Instances.Num());

	TArray<FBodyInstance*> ValidBodyInstances;
	ValidBodyInstances.Reserve(Instances.Num());
	TArray<FTransform> ValidTransforms;
	ValidTransforms.Reserve(Instances.Num());

	for (int32 Index = 0; Index < Instances.Num(); Index++)
	{
		const FTransform InstanceTransform = FTransform(Instances[Index]) * GetComponentTransform();

		if (InstanceTransform.GetScale3D().IsNearlyZero())
		{
			InstanceBodies[Index] = nullptr;
			continue;
		}

		FBodyInstance* Instance = new FBodyInstance();

		InstanceBodies[Index] = Instance;
		Instance->CopyRuntimeBodyInstancePropertiesFrom(&BodyInstance);
		Instance->InstanceBodyIndex = Index;
		Instance->bAutoWeld = false;

		Instance->bSimulatePhysics = false;

		ValidBodyInstances.Add(Instance);
		ValidTransforms.Add(InstanceTransform);
	}

	FBodyInstance::InitStaticBodies(ValidBodyInstances, ValidTransforms, BodySetup, this, PhysicsScene);
	RecreatePhysicsState();
	MarkRenderStateDirty();
}

void UVoxelFoliageCollisionComponent::ClearAllInstanceBodies()
{
	VOXEL_FUNCTION_COUNTER();

	for (int32 i = 0; i < InstanceBodies.Num(); i++)
	{
		if (InstanceBodies[i])
		{
			InstanceBodies[i]->TermBody();
			delete InstanceBodies[i];
		}
	}

	InstanceBodies.Reset();
	Instances.Reset();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class FVoxelFoliageCollisionSceneProxy : public FPrimitiveSceneProxy
{
	const TArray<FMatrix> Instances;
	const UBodySetup* BodySetup;
	const FTransform ComponentTransform;

public:
	explicit FVoxelFoliageCollisionSceneProxy(UVoxelFoliageCollisionComponent& Component)
		: FPrimitiveSceneProxy(&Component)
		, Instances(Component.Instances)
		, BodySetup(Component.GetBodySetup())
		, ComponentTransform(Component.GetComponentTransform())
	{
	}

	//~ Begin FPrimitiveSceneProxy Interface
	virtual void GetDynamicMeshElements(
		const TArray<const FSceneView*>& Views,
		const FSceneViewFamily& ViewFamily,
		uint32 VisibilityMap,
		FMeshElementCollector& Collector) const override
	{
		VOXEL_FUNCTION_COUNTER_LLM();

		if (!GVoxelFoliageShowInstancesCollisions)
		{
			return;
		}

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (!(VisibilityMap & (1 << ViewIndex)))
			{
				continue;
			}

			ensure(
				Views[ViewIndex]->Family->EngineShowFlags.Collision ||
				Views[ViewIndex]->Family->EngineShowFlags.CollisionPawn ||
				Views[ViewIndex]->Family->EngineShowFlags.CollisionVisibility);

			FColoredMaterialRenderProxy* MaterialProxy = new FColoredMaterialRenderProxy(
				GEngine->ShadedLevelColorationUnlitMaterial->GetRenderProxy(),
				FColor(157, 149, 223, 255));

			Collector.RegisterOneFrameMaterialProxy(MaterialProxy);

			for (const FMatrix& Instance : Instances)
			{
				BodySetup->AggGeom.GetAggGeom(
					FTransform(Instance) * ComponentTransform,
					FColor(157, 149, 223, 255),
					MaterialProxy,
					false,
					true,
					DrawsVelocity(),
					ViewIndex,
					Collector);
			}
		}
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = true;
		Result.bRenderInMainPass = true;
		Result.bDynamicRelevance =
			View->Family->EngineShowFlags.Collision ||
			View->Family->EngineShowFlags.CollisionPawn ||
			View->Family->EngineShowFlags.CollisionVisibility;
		return Result;
	}

	virtual uint32 GetMemoryFootprint() const override
	{
		return sizeof(*this) + GetAllocatedSize();
	}

	virtual SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}
	//~ End FPrimitiveSceneProxy Interface
};

FPrimitiveSceneProxy* UVoxelFoliageCollisionComponent::CreateSceneProxy()
{
	if (!GIsEditor ||
		Instances.Num() == 0 ||
		!GVoxelFoliageShowInstancesCollisions)
	{
		return nullptr;
	}

	return new FVoxelFoliageCollisionSceneProxy(*this);
}