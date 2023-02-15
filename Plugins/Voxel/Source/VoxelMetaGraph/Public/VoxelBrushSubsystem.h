// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBrush.h"
#include "VoxelQuery.h"
#include "VoxelRuntime/VoxelSubsystem.h"
#include "VoxelBrushSubsystem.generated.h"

UCLASS()
class VOXELMETAGRAPH_API UVoxelBrushSubsystemProxy : public UVoxelSubsystemProxy
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(FVoxelBrushSubsystem);
};

class VOXELMETAGRAPH_API FVoxelBrushSubsystem : public IVoxelSubsystem
{
public:
	GENERATED_VOXEL_SUBSYSTEM_BODY(UVoxelBrushSubsystemProxy);

	//~ Begin IVoxelSubsystem Interface
	virtual void Tick() override;
	//~ End IVoxelSubsystem Interface

public:
	TArray<const UScriptStruct*> GetAllStructs() const;

	template<typename T>
	TVoxelArray<TSharedPtr<const T>> GetBrushes(
		const FName LayerName,
		const FVoxelBox& Bounds,
		const TSharedRef<FVoxelDependency>& Dependency)
	{
		return ::ReinterpretCastVoxelArray<TSharedPtr<const T>>(this->GetBrushes(
			T::BrushType::StaticStruct(),
			LayerName,
			Bounds,
			Dependency));
	}
	TVoxelArray<TSharedPtr<const FVoxelBrushImpl>> GetBrushes(
		const UScriptStruct* Struct,
		FName LayerName,
		const FVoxelBox& Bounds,
		const TSharedRef<FVoxelDependency>& Dependency);

public:
	struct FFindResult
	{
		TSharedPtr<const FVoxelBrushImpl> Brush;
		float Distance = 0;
	};
	bool FindClosestBrush(FFindResult& OutResult, const FVector& WorldPosition) const;

private:
	FMatrix LastWorldToLocal = FMatrix::Identity;

	mutable FVoxelCriticalSection CriticalSection_BrushesMap;
	
	struct FDependencyRef
	{
		FVoxelBox Bounds;
		TWeakPtr<FVoxelDependency> Dependency;
	};
	struct FBrushes
	{
		mutable FVoxelCriticalSection CriticalSection;
		TVoxelArray<TSharedPtr<const FVoxelBrushImpl>> Brushes;
		TMap<FName, TVoxelArray<FDependencyRef>> LayerNameToDependencyRefs;
	};
	TMap<const UScriptStruct*, TSharedPtr<FBrushes>> BrushesMap;

	void ForAllBrushes(TFunctionRef<void(const FBrushes&)> Lambda) const;
	TSharedRef<FBrushes> FindOrAddBrushes(const UScriptStruct* Struct);
	void UpdateBrushes(const UScriptStruct* Struct);
};