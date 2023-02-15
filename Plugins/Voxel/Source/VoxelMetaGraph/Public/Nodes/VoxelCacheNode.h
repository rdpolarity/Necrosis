// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelQueryMap.h"
#include "VoxelRuntime/VoxelSubsystem.h"
#include "VoxelCacheNode.generated.h"

DECLARE_UNIQUE_VOXEL_ID(FVoxelCachedValueId);

UCLASS()
class VOXELMETAGRAPH_API UVoxelCacheNodeSubsystemProxy : public UVoxelSubsystemProxy
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(FVoxelCacheNodeSubsystem);
};

USTRUCT()
struct FVoxelCachedValue
{
	GENERATED_BODY()

	FVoxelSharedPinValue Value;
	TSet<TSharedPtr<FVoxelDependency>> Dependencies;
};

struct FVoxelCachedValueRef
{
	const FVoxelCachedValueId Id = FVoxelCachedValueId::New();
	double LastAccessTime = 0;
	TUniquePtr<TVoxelFutureValue<FVoxelCachedValue>> Value;
};

class VOXELMETAGRAPH_API FVoxelCacheNodeSubsystem : public IVoxelSubsystem
{
public:
	GENERATED_VOXEL_SUBSYSTEM_BODY(UVoxelCacheNodeSubsystemProxy);

	using FQueryCache = TVoxelQueryMap<TSharedPtr<FVoxelCachedValueRef>>;

	struct FNodeCache
	{
		const TSharedRef<IVoxelNodeOuter> Outer;
		FQueryCache QueryCache;

		explicit FNodeCache(const TSharedRef<IVoxelNodeOuter>& Outer)
			: Outer(Outer)
		{
		}
	};
	
	TVoxelKeyedCriticalSection<FVoxelCachedValueId> ValueCriticalSection;
	FVoxelCriticalSection CriticalSection;
	TMap<const FVoxelNode*, TSharedPtr<FNodeCache>> NodeCaches;

	void Cleanup(const TSharedRef<FNodeCache>& NodeCache);
};

USTRUCT(Category = "Misc")
struct VOXELMETAGRAPH_API FVoxelNode_Cache : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_GENERIC_INPUT_PIN(Data);
	VOXEL_GENERIC_OUTPUT_PIN(OutData);

	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;
};