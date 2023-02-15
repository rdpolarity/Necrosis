// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBrush.h"
#include "Nodes/VoxelDistanceFieldNodes.h"
#include "VoxelPointCanvasBrush.generated.h"

class UVoxelPointCanvasAsset;
struct FVoxelPointCanvasData;

USTRUCT()
struct VOXELCANVAS_API FVoxelPointCanvasBrush : public FVoxelBrush
{
	GENERATED_BODY()
	GENERATED_VOXEL_BRUSH_BODY();
		
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config")
	TObjectPtr<UVoxelPointCanvasAsset> Canvas;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config")
	bool bSubtractive = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config")
	float Smoothness = 100.f;
	
	// Higher priority assets will be processed last
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config")
	int32 Priority = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config")
	float Scale = 100.f;

public:
	TSharedPtr<const FVoxelPointCanvasData> CanvasData;

	virtual bool IsValid() const override;
	virtual void CacheData_GameThread() override;
};

struct VOXELCANVAS_API FVoxelPointCanvasBrushImpl : TVoxelBrushImpl<FVoxelPointCanvasBrush>
{
	const TSharedRef<const FVoxelPointCanvasData> Canvas;

	FMatrix44f LocalToData = FMatrix44f::Identity;
	float DataToLocalScale = 0.f;

	explicit FVoxelPointCanvasBrushImpl(const FVoxelPointCanvasBrush& Brush, const FMatrix& WorldToLocal);

	virtual float GetDistance(const FVector& LocalPosition) const override;
	virtual TSharedPtr<FVoxelDistanceField> GetDistanceField() const override;
};

USTRUCT(meta = (Internal))
struct FVoxelNode_FVoxelPointCanvasDistanceField_GetDistances : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_CALL_PARAM(TSharedPtr<const FVoxelPointCanvasBrushImpl>, Brush);
	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Distance);
};

USTRUCT()
struct FVoxelPointCanvasDistanceField : public FVoxelDistanceField
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	TSharedPtr<const FVoxelPointCanvasBrushImpl> Brush;

	virtual TVoxelFutureValue<FVoxelFloatBuffer> GetDistances(const FVoxelQuery& Query) const override;
};