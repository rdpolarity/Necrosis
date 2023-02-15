// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelDependencyManager.h"

DEFINE_VOXEL_SUBSYSTEM(FVoxelDependencyManager);

void FVoxelDependencyManager::AddDependency2D(const void* Category, FName Element, const FVoxelBox2D& Bounds, const TSharedRef<FVoxelDependency>& Dependency)
{
	VOXEL_SCOPE_LOCK(CriticalSection);
	Map.FindOrAdd({ Category, Element }).Dependencies_2D.Add({ Bounds, Dependency });
}

void FVoxelDependencyManager::AddDependency3D(const void* Category, FName Element, const FVoxelBox& Bounds, const TSharedRef<FVoxelDependency>& Dependency)
{
	VOXEL_SCOPE_LOCK(CriticalSection);
	Map.FindOrAdd({ Category, Element }).Dependencies_3D.Add({ Bounds, Dependency });
}

void FVoxelDependencyManager::Update2D(const void* Category, TMap<FName, TVoxelArray<FVoxelBox2D>>&& InUpdates)
{
	GetRuntimeChecked().AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [=, Updates = MoveTemp(InUpdates)]
	{
		VOXEL_FUNCTION_COUNTER();

		TSet<TSharedPtr<FVoxelDependency>> DependenciesToInvalidate;
		{
			FVoxelScopeLock Lock(CriticalSection);

			for (const auto& It : Updates)
			{
				FValue* Value = Map.Find({ Category, It.Key });
				if (!Value)
				{
					continue;
				}

				TVoxelArray<TPair<FVoxelBox2D, TWeakPtr<FVoxelDependency>>>& Dependencies = Value->Dependencies_2D;

				for (int32 Index = 0; Index < Dependencies.Num(); Index++)
				{
					const auto& Pair = Dependencies[Index];

					const TSharedPtr<FVoxelDependency> Dependency = Pair.Value.Pin();
					if (!Dependency.IsValid())
					{
						Dependencies.RemoveAtSwap(Index);
						Index--;
						continue;
					}

					for (const FVoxelBox2D& Bounds : It.Value)
					{
						if (!Pair.Key.Intersect(Bounds))
						{
							continue;
						}

						DependenciesToInvalidate.Add(Dependency);
						Dependencies.RemoveAtSwap(Index);
						Index--;
						break;
					}
				}
			}
		}

		FVoxelDependency::InvalidateDependencies(DependenciesToInvalidate);
	});
}

void FVoxelDependencyManager::Update3D(const void* Category, TMap<FName, TVoxelArray<FVoxelBox>>&& InUpdates)
{
	GetRuntimeChecked().AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [=, Updates = MoveTemp(InUpdates)]
	{
		VOXEL_FUNCTION_COUNTER();

		TSet<TSharedPtr<FVoxelDependency>> DependenciesToInvalidate;
		{
			FVoxelScopeLock Lock(CriticalSection);

			for (const auto& It : Updates)
			{
				FValue* Value = Map.Find({ Category, It.Key });
				if (!Value)
				{
					continue;
				}

				TVoxelArray<TPair<FVoxelBox, TWeakPtr<FVoxelDependency>>>& Dependencies = Value->Dependencies_3D;

				for (int32 Index = 0; Index < Dependencies.Num(); Index++)
				{
					const auto& Pair = Dependencies[Index];

					const TSharedPtr<FVoxelDependency> Dependency = Pair.Value.Pin();
					if (!Dependency.IsValid())
					{
						Dependencies.RemoveAtSwap(Index);
						Index--;
						continue;
					}

					for (const FVoxelBox& Bounds : It.Value)
					{
						if (!Pair.Key.Intersect(Bounds))
						{
							continue;
						}

						DependenciesToInvalidate.Add(Dependency);
						Dependencies.RemoveAtSwap(Index);
						Index--;
						break;
					}
				}
			}
		}
		FVoxelDependency::InvalidateDependencies(DependenciesToInvalidate);
	});
}