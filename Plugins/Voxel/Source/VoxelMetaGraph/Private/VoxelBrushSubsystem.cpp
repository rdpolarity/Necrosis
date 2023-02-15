// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelBrushSubsystem.h"

DEFINE_VOXEL_SUBSYSTEM(FVoxelBrushSubsystem);

void FVoxelBrushSubsystem::Tick()
{
	Super::Tick();

	if (LastWorldToLocal.Equals(WorldToLocal()))
	{
		return;
	}
	LastWorldToLocal = WorldToLocal();

	AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [=]
	{
		VOXEL_SCOPE_COUNTER("UpdateAll");
		
		for (const UScriptStruct* Struct : GetAllStructs())
		{
			UpdateBrushes(Struct);
		}
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


TArray<const UScriptStruct*> FVoxelBrushSubsystem::GetAllStructs() const
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelScopeLock Lock(CriticalSection_BrushesMap);

	TArray<const UScriptStruct*> Structs;
	BrushesMap.GenerateKeyArray(Structs);
	return Structs;
}

TVoxelArray<TSharedPtr<const FVoxelBrushImpl>> FVoxelBrushSubsystem::GetBrushes(
	const UScriptStruct* Struct,
	const FName LayerName,
	const FVoxelBox& Bounds,
	const TSharedRef<FVoxelDependency>& Dependency)
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedRef<FBrushes> Brushes = FindOrAddBrushes(Struct);

	FVoxelScopeLock Lock(Brushes->CriticalSection);

	TVoxelArray<TSharedPtr<const FVoxelBrushImpl>> OutBrushes;
	for (const TSharedPtr<const FVoxelBrushImpl>& Brush : Brushes->Brushes)
	{
		if (Brush->GetBrush().LayerName == LayerName &&
			Brush->GetBounds().Intersect(Bounds))
		{
			OutBrushes.Add(Brush);
		}
	}

	Brushes->LayerNameToDependencyRefs.FindOrAdd(LayerName).Add(FDependencyRef
	{
		Bounds,
		Dependency
	});

	return OutBrushes;
}

bool FVoxelBrushSubsystem::FindClosestBrush(FFindResult& OutResult, const FVector& WorldPosition) const
{
	VOXEL_FUNCTION_COUNTER();

	const FVector LocalPosition = WorldToLocal().TransformPosition(WorldPosition);
	
	float MinDistance = MAX_flt;
	ForAllBrushes([&](const FBrushes& Brushes)
	{
		FVoxelScopeLock Lock(Brushes.CriticalSection);
		for (const TSharedPtr<const FVoxelBrushImpl>& Brush : Brushes.Brushes)
		{
			const float Distance = Brush->GetDistance(LocalPosition);

			if (Distance > MinDistance)
			{
				continue;
			}
			MinDistance = Distance;

			OutResult.Brush = Brush;
			OutResult.Distance = Distance;
		}
	});
	return MinDistance != MAX_flt;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelBrushSubsystem::ForAllBrushes(TFunctionRef<void(const FBrushes&)> Lambda) const
{
	VOXEL_FUNCTION_COUNTER();

	TMap<const UScriptStruct*, TSharedPtr<FBrushes>> BrushesMapCopy;
	{
		FVoxelScopeLock Lock(CriticalSection_BrushesMap);
		BrushesMapCopy = BrushesMap;
	}

	for (const auto& It : BrushesMapCopy)
	{
		Lambda(*It.Value);
	}
}

TSharedRef<FVoxelBrushSubsystem::FBrushes> FVoxelBrushSubsystem::FindOrAddBrushes(const UScriptStruct* Struct)
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelScopeLock Lock(CriticalSection_BrushesMap);

	TSharedPtr<FBrushes>& Brushes = BrushesMap.FindOrAdd(Struct);
	if (!Brushes)
	{
		Brushes = MakeShared<FBrushes>();

		GVoxelBrushRegistry->AddListener(GetWorld(), Struct, MakeWeakSubsystemDelegate([=]
		{
			UpdateBrushes(Struct);
		}));
		
		for (const TSharedPtr<const FVoxelBrush>& Brush : GVoxelBrushRegistry->GetBrushes(GetWorld(), Struct))
		{
			const TSharedPtr<const FVoxelBrushImpl> BrushImpl = Brush->GetImpl(WorldToLocal());
			if (!BrushImpl)
			{
				continue;
			}

			Brushes->Brushes.Add(BrushImpl);
		}
		Brushes->Brushes.Sort(FVoxelBrushImpl::FLess());
	}
	return Brushes.ToSharedRef();
}

void FVoxelBrushSubsystem::UpdateBrushes(const UScriptStruct* Struct)
{
	VOXEL_FUNCTION_COUNTER();

	{
		FVoxelScopeLock Lock(CriticalSection_BrushesMap);
		if (!ensure(BrushesMap.Contains(Struct)))
		{
			return;
		}
	}

	TVoxelArray<TSharedPtr<const FVoxelBrushImpl>> NewBrushes;
	for (const TSharedPtr<const FVoxelBrush>& Brush : GVoxelBrushRegistry->GetBrushes(GetWorld(), Struct))
	{
		const TSharedPtr<const FVoxelBrushImpl> BrushImpl = Brush->GetImpl(WorldToLocal());
		if (!BrushImpl)
		{
			continue;
		}

		NewBrushes.Add(BrushImpl);
	}

	{
		VOXEL_SCOPE_COUNTER("Sort");
		NewBrushes.Sort(FVoxelBrushImpl::FLess());
	}

	const TSharedRef<FBrushes> Brushes = FindOrAddBrushes(Struct);

	TSet<TSharedPtr<FVoxelDependency>> Dependencies;
	{
		FVoxelScopeLock Lock(Brushes->CriticalSection);

		TVoxelArray<TSharedPtr<const FVoxelBrushImpl>> BrushesToUpdate;
		{
			VOXEL_SCOPE_COUNTER("Find BrushesToUpdate");

			FVoxelUtilities::DiffSortedArrays(
				Brushes->Brushes,
				NewBrushes,
				BrushesToUpdate,
				BrushesToUpdate,
				FVoxelBrushImpl::FLess());

			Brushes->Brushes = MoveTemp(NewBrushes);
		}

		VOXEL_SCOPE_COUNTER("Find Dependencies");
		for (const TSharedPtr<const FVoxelBrushImpl>& Brush : BrushesToUpdate)
		{
			TVoxelArray<FDependencyRef>* DependencyRefs = Brushes->LayerNameToDependencyRefs.Find(Brush->GetBrush().LayerName);
			if (!DependencyRefs)
			{
				continue;
			}

			DependencyRefs->RemoveAllSwap([&](const FDependencyRef& DependencyRef)
			{
				const TSharedPtr<FVoxelDependency> Dependency = DependencyRef.Dependency.Pin();
				if (!Dependency)
				{
					return true;
				}

				if (!DependencyRef.Bounds.Intersect(Brush->GetBounds()))
				{
					return false;
				}

				Dependencies.Add(Dependency);
				return true;
			});
		}
	}

	FVoxelDependency::InvalidateDependencies(Dependencies);
}