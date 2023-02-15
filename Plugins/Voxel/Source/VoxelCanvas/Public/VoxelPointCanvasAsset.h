// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelPointCanvasAsset.generated.h"

struct FVoxelPointCanvasData;

UCLASS(meta = (VoxelAssetType, AssetSubMenu=Canvas, AssetColor=Red))
class VOXELCANVAS_API UVoxelPointCanvasAsset : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	bool bCompress = false;

	UFUNCTION(meta = (ShowEditorButton))
	void InitializeDefault();

	UFUNCTION(meta = (ShowEditorButton))
	void Edit();

	//~ Begin UObject Interface
	virtual void PostLoad() override;
	virtual void Serialize(FArchive& Ar) override;
	//~ End UObject Interface

	TSharedRef<FVoxelPointCanvasData> GetData() const
	{
		return Data.ToSharedRef();
	}

private:
	FByteBulkData BulkData;
	TSharedPtr<FVoxelPointCanvasData> Data;
};