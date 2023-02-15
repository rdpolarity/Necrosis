// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMaterialLayer.h"
#include "VoxelMaterialParameters.h"
#include "VoxelMaterialLayerAsset.h"
#include "VoxelRuntime/VoxelSubsystem.h"
#include "VoxelMaterialLayerRenderer.generated.h"

UCLASS()
class VOXELMATERIAL_API UVoxelMaterialLayerRendererProxy : public UVoxelSubsystemProxy
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(FVoxelMaterialLayerRenderer);
};

class VOXELMATERIAL_API FVoxelMaterialLayerRenderer : public IVoxelSubsystem
{
public:
	GENERATED_VOXEL_SUBSYSTEM_BODY(UVoxelMaterialLayerRendererProxy);

	//~ Begin IVoxelSubsystem Interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual void Tick() override;
	//~ End IVoxelSubsystem Interface

	FVoxelMaterialLayer RegisterLayer_GameThread(const UVoxelMaterialLayerAsset* Layer);
	TSharedRef<FVoxelMaterialRef> GetMaterialInstance_AnyThread(uint8 ClassIndex) const;

private:
	struct FClassData
	{
		const TSubclassOf<UVoxelMaterialLayerAsset> Class;
		const TSharedRef<FVoxelMaterialRef> MaterialInstance;

		int32 LayerIndexCounter = 0;
		TMap<TWeakObjectPtr<const UVoxelMaterialLayerAsset>, uint8> LayerIndices;
		TMap<FName, TVoxelInstancedStruct<FVoxelMaterialParameterData>> Parameters;

		FClassData(const TSubclassOf<UVoxelMaterialLayerAsset>& Class, const TSharedRef<FVoxelMaterialRef>& MaterialInstance)
			: Class(Class)
			, MaterialInstance(MaterialInstance)
		{
		}
	};

	mutable FVoxelCriticalSection CriticalSection;

	bool bHasPendingChanges = false;
	TMap<TSubclassOf<UVoxelMaterialLayerAsset>, TSharedPtr<FClassData>> ClassDatas;

	int32 ClassIndexCounter = 0;
	TMap<TSubclassOf<UVoxelMaterialLayerAsset>, uint8> ClassToClassIndex;
	TMap<uint8, TSubclassOf<UVoxelMaterialLayerAsset>> ClassIndexToClass;
};