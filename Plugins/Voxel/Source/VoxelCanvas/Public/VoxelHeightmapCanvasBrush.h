// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

#if 0 // TODO use shared impl with landmass height brushes
#include "VoxelBrush.h"
#include "Nodes/VoxelDistanceFieldNodes.h"
#include "VoxelHeightmapCanvasBrush.generated.h"

class FVoxelHeightmapCanvasData;
class UVoxelHeightmapCanvasAsset;

USTRUCT()
struct VOXELCANVAS_API FVoxelHeightmapCanvasBrush : public FVoxelBrush
{
	GENERATED_BODY()
	GENERATED_VOXEL_BRUSH_BODY();
		
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config")
	TObjectPtr<UVoxelHeightmapCanvasAsset> Canvas;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config")
	bool bSubtractive = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config")
	float Smoothness = 100.f;
	
	// Higher priority assets will be processed last
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config")
	int32 Priority = 0;

public:
	TSharedPtr<const FVoxelHeightmapCanvasData> CanvasData;

	virtual bool IsValid() const override { return CanvasData.IsValid(); }
	virtual void CacheData_GameThread() override;
};

struct VOXELCANVAS_API FVoxelHeightmapCanvasBrushImpl : TVoxelBrushImpl<FVoxelHeightmapCanvasBrush>
{
	const TSharedRef<const FVoxelHeightmapCanvasData> CanvasData;

	explicit FVoxelHeightmapCanvasBrushImpl(const FVoxelHeightmapCanvasBrush& Brush, const FMatrix& WorldToLocal);
};

USTRUCT(meta = (Internal))
struct FVoxelNode_FVoxelHeightmapCanvasDistanceField_GetDistances : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_CALL_PARAM(TSharedPtr<const FVoxelHeightmapCanvasBrushImpl>, Brush);
	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Distance);
};

USTRUCT()
struct FVoxelHeightmapCanvasDistanceField : public FVoxelDistanceField
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	TSharedPtr<const FVoxelHeightmapCanvasBrushImpl> Brush;

	virtual TVoxelFutureValue<FVoxelFloatBuffer> GetDistances(const FVoxelQuery& Query) const override;
};
#endif