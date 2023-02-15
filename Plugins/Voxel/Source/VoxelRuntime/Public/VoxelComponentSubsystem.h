// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelRuntime/VoxelSubsystem.h"
#include "VoxelComponentSubsystem.generated.h"

UCLASS()
class VOXELRUNTIME_API UVoxelComponentSubsystemProxy : public UVoxelSubsystemProxy
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(FVoxelComponentSubsystem);
};

class VOXELRUNTIME_API FVoxelComponentSubsystem : public IVoxelSubsystem
{
public:
	GENERATED_VOXEL_SUBSYSTEM_BODY(UVoxelComponentSubsystemProxy);

	static bool bDisableModify;

	//~ Begin IVoxelSubsystem Interface
	virtual void Destroy() override;
	//~ End IVoxelSubsystem Interface

	UActorComponent* CreateComponent(const UClass* Class);
	void DestroyComponent(UActorComponent* Component);

	void SetupSceneComponent(USceneComponent& Component) const;
	void SetComponentPosition(USceneComponent& Component, const FVector3d& Position) const;
	
	template<typename T, typename = typename TEnableIf<TIsDerivedFrom<T, UActorComponent>::Value>::Type>
	T* CreateComponent(UClass* Class = nullptr)
	{
		if (!Class)
		{
			Class = T::StaticClass();
		}
		return CastChecked<T>(CreateComponent(Class), ECastCheckedType::NullAllowed);
	}

private:
	TSet<FObjectKey> Components;
};