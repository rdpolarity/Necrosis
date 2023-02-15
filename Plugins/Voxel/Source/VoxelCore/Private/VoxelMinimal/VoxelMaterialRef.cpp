// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMinimal.h"

DECLARE_VOXEL_COUNTER(VOXELCORE_API, STAT_VoxelNumMaterialInstancesPooled, "Num Material Instances Pooled");
DECLARE_VOXEL_COUNTER(VOXELCORE_API, STAT_VoxelNumMaterialInstancesUsed, "Num Material Instances Used");

DEFINE_VOXEL_COUNTER(STAT_VoxelNumMaterialInstancesPooled);
DEFINE_VOXEL_COUNTER(STAT_VoxelNumMaterialInstancesUsed);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct FVoxelInstanceRef
{
	UMaterialInstanceDynamic* Instance = nullptr;
};

struct FVoxelInstancePool
{
	double LastAccessTime = 0;
	TArray<UMaterialInstanceDynamic*> Instances;
};

class FVoxelMaterialRefManager : public FGCObject
{
public:
	TSharedPtr<FVoxelMaterialRef> DefaultMaterial;

	TArray<TWeakPtr<FVoxelMaterialRef>> MaterialRefs;
	TSet<TSharedPtr<FVoxelInstanceRef>> InstanceRefs;

	// We don't want to keep the parents alive
	TMap<TWeakObjectPtr<UMaterialInterface>, FVoxelInstancePool> InstancePools;

	TMap<FVoxelMaterialParameters, TWeakPtr<FVoxelMaterialRef>> ParametersToInstance;
	
public:
	//~ Begin FGCObject Interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		VOXEL_FUNCTION_COUNTER();

		MaterialRefs.RemoveAllSwap([](const TWeakPtr<FVoxelMaterialRef>& MaterialRef)
		{
			return !MaterialRef.IsValid();
		});

		// Cleanup pools before tracking refs
		{
			const double AccessTimeThreshold = FPlatformTime::Seconds() - 30.f;
			for (auto It = InstancePools.CreateIterator(); It; ++It)
			{
				if (!It.Key().IsValid() || It.Value().LastAccessTime < AccessTimeThreshold)
				{
					DEC_VOXEL_COUNTER_BY(STAT_VoxelNumMaterialInstancesPooled, It.Value().Instances.Num());
					It.RemoveCurrent();
				}
			}
		}

		for (const TWeakPtr<FVoxelMaterialRef>& WeakMaterial : MaterialRefs)
		{
			if (const TSharedPtr<FVoxelMaterialRef> Material = WeakMaterial.Pin())
			{
				Collector.AddReferencedObject(Material->Material);
			}
		}
		for (const TSharedPtr<FVoxelInstanceRef>& InstanceRef : InstanceRefs)
		{
			Collector.AddReferencedObject(InstanceRef->Instance);
		}

		for (auto& It : InstancePools)
		{
			Collector.AddReferencedObjects(It.Value.Instances);
		}
	}
	virtual FString GetReferencerName() const override
	{
		return "FVoxelMaterialRefManager";
	}
	//~ End FGCObject Interface
	
public:
	UMaterialInstanceDynamic* GetInstanceFromPool(UMaterialInterface* Parent)
	{
		VOXEL_FUNCTION_COUNTER();
		check(IsInGameThread());

		INC_VOXEL_COUNTER(STAT_VoxelNumMaterialInstancesUsed);

		UMaterialInstanceDynamic* Instance = nullptr;

		// Create the pool if needed so that LastAccessTime is properly set
		// Otherwise instances returned to the pool can get incorrectly immediately GCed
		FVoxelInstancePool& Pool = InstancePools.FindOrAdd(Parent);
		Pool.LastAccessTime = FPlatformTime::Seconds();

		while (!Instance && Pool.Instances.Num() > 0)
		{
			Instance = Pool.Instances.Pop(false);
			DEC_VOXEL_COUNTER(STAT_VoxelNumMaterialInstancesPooled);
		}

		if (!Instance)
		{
			Instance = UMaterialInstanceDynamic::Create(Parent, GetTransientPackage());
		}

		check(Instance);
		return Instance;
	}
	void ReturnInstanceToPool(UMaterialInstanceDynamic* Instance)
	{
		VOXEL_FUNCTION_COUNTER();
		check(IsInGameThread());

		DEC_VOXEL_COUNTER(STAT_VoxelNumMaterialInstancesUsed);

		if (!Instance || !Instance->Parent)
		{
			return;
		}

		Instance->ClearParameterValues();

		InstancePools.FindOrAdd(Instance->Parent).Instances.Add(Instance);
		INC_VOXEL_COUNTER(STAT_VoxelNumMaterialInstancesPooled);
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelMaterialRefManager* GVoxelMaterialRefManager = nullptr;

VOXEL_RUN_ON_STARTUP_GAME(CreateGVoxelMaterialInterfaceManager)
{
	if (!FApp::CanEverRender())
	{
		return;
	}

	GVoxelMaterialRefManager = new FVoxelMaterialRefManager();
	
	UMaterial* DefaultMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
	check(DefaultMaterial);
	GVoxelMaterialRefManager->DefaultMaterial = FVoxelMaterialRef::Make(DefaultMaterial);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMaterialParameters::ApplyTo(UMaterialInstanceDynamic* Instance) const
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	if (!Instance)
	{
		return;
	}

	for (auto& It : ScalarParameters)
	{
		Instance->SetScalarParameterValue(It.Key, It.Value);
	}
	for (auto& It : VectorParameters)
	{
		Instance->SetVectorParameterValue(It.Key, It.Value);
	}
	for (auto& It : TextureParameters)
	{
		Instance->SetTextureParameterValue(It.Key, It.Value.Get());
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<FVoxelMaterialRef> FVoxelMaterialRef::Default()
{
	return GVoxelMaterialRefManager->DefaultMaterial.ToSharedRef();
}

TSharedRef<FVoxelMaterialRef> FVoxelMaterialRef::Make(UMaterialInterface* Material)
{
	check(IsInGameThread());

	if (!Material)
	{
		return Default();
	}

	const TSharedRef<FVoxelMaterialRef> MaterialRef = MakeShareable(new FVoxelMaterialRef());
	MaterialRef->Material = Material;
	GVoxelMaterialRefManager->MaterialRefs.Add(MaterialRef);

	return MaterialRef;
}

TSharedRef<FVoxelMaterialRef> FVoxelMaterialRef::MakeInstance(UMaterialInterface* Parent)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	if (!IsValid(Parent) ||
		!ensure(!Parent->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad | RF_NeedPostLoadSubobjects)))
	{
		Parent = UMaterial::GetDefaultMaterial(MD_Surface);
	}

	UMaterialInstanceDynamic* ParentInstance = Cast<UMaterialInstanceDynamic>(Parent);
	if (ParentInstance)
	{
		Parent = ParentInstance->Parent;
	}

	UMaterialInstanceDynamic* Instance = GVoxelMaterialRefManager->GetInstanceFromPool(Parent);
	check(Instance);

	if (ParentInstance)
	{
		VOXEL_SCOPE_COUNTER("CopyParameterOverrides");
		Instance->CopyParameterOverrides(ParentInstance);
	}

	const TSharedRef<FVoxelInstanceRef> InstanceRef = MakeShared<FVoxelInstanceRef>();
	GVoxelMaterialRefManager->InstanceRefs.Add(InstanceRef);

	const TSharedRef<FVoxelMaterialRef> MaterialRef = MakeShareable(new FVoxelMaterialRef());
	MaterialRef->Material = Instance;
	MaterialRef->InstanceRef = InstanceRef;
	GVoxelMaterialRefManager->MaterialRefs.Add(MaterialRef);

	return MaterialRef;
}

TSharedRef<FVoxelMaterialRef> FVoxelMaterialRef::MakeInstance(UMaterialInterface* Parent, const FVoxelMaterialParameters& Parameters)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	TWeakPtr<FVoxelMaterialRef>& WeakInstance = VOXEL_INLINE_COUNTER("FindOrAdd", GVoxelMaterialRefManager->ParametersToInstance.FindOrAdd(Parameters));
	TSharedPtr<FVoxelMaterialRef> Instance = WeakInstance.Pin();
	if (Instance.IsValid())
	{
		return Instance.ToSharedRef();
	}

	Instance = MakeInstance(Parent);
	WeakInstance = Instance;

	Parameters.ApplyTo(Instance->GetMaterialInstance());

	for (const TSharedPtr<FVirtualDestructor>& Resource : Parameters.Resources)
	{
		Instance->Resources.Enqueue(Resource);
	}

	{
		VOXEL_SCOPE_COUNTER("Delete old instances");

		// Cleanup map AFTER setting WeakInstance
		for (auto It = GVoxelMaterialRefManager->ParametersToInstance.CreateIterator(); It; ++It)
		{
			if (!It.Value().IsValid())
			{
				It.RemoveCurrent();
			}
		}
	}

	return Instance.ToSharedRef();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelMaterialRef::~FVoxelMaterialRef()
{
	if (!InstanceRef)
	{
		return;
	}

	FVoxelUtilities::RunOnGameThread([InstanceRef = InstanceRef]
	{
		check(IsInGameThread());

		ensure(GVoxelMaterialRefManager->InstanceRefs.Remove(InstanceRef));
		GVoxelMaterialRefManager->ReturnInstanceToPool(InstanceRef->Instance);
	});
}