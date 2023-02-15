// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelQuery.h"
#include "VoxelRuntime/VoxelSubsystem.h"
#include "VoxelDependencyManager.generated.h"

UCLASS()
class VOXELMETAGRAPH_API UVoxelDependencyManagerProxy : public UVoxelSubsystemProxy
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(FVoxelDependencyManager);
};

class VOXELMETAGRAPH_API FVoxelDependencyManager : public IVoxelSubsystem
{
public:
	GENERATED_VOXEL_SUBSYSTEM_BODY(UVoxelDependencyManagerProxy);

	FVoxelDependencyManager() = default;
	
	void AddDependency2D(const void* Category, FName Element, const FVoxelBox2D& Bounds, const TSharedRef<FVoxelDependency>& Dependency);
	void AddDependency3D(const void* Category, FName Element, const FVoxelBox& Bounds, const TSharedRef<FVoxelDependency>& Dependency);

	void Update2D(const void* Category, TMap<FName, TVoxelArray<FVoxelBox2D>>&& Updates);
	void Update3D(const void* Category, TMap<FName, TVoxelArray<FVoxelBox>>&& Updates);

private:
	struct FKey
	{
		const void* Category;
		FName Element;

		friend uint32 GetTypeHash(const FKey& Key)
		{
			return GetTypeHash(Key.Category) ^ GetTypeHash(Key.Element);
		}
		bool operator==(const FKey& Other) const
		{
			return Category == Other.Category && Element == Other.Element;
		}
	};
	struct FValue
	{
		TVoxelArray<TPair<FVoxelBox2D, TWeakPtr<FVoxelDependency>>> Dependencies_2D;
		TVoxelArray<TPair<FVoxelBox, TWeakPtr<FVoxelDependency>>> Dependencies_3D;
	};

	FVoxelCriticalSection CriticalSection;
	TMap<FKey, FValue> Map;
};