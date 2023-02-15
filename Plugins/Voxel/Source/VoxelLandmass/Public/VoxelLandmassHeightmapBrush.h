// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBrush.h"
#include "VoxelHeightmap.h"
#include "Nodes/VoxelDistanceFieldNodes.h"
#include "VoxelLandmassHeightmapBrush.generated.h"

class FVoxelHeightmap;

USTRUCT()
struct VOXELLANDMASS_API FVoxelLandmassHeightmapBrush : public FVoxelBrush
{
	GENERATED_BODY()
	GENERATED_VOXEL_BRUSH_BODY();
		
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config")
	TObjectPtr<UVoxelHeightmap> Heightmap;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config")
	bool bSubtractive = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config")
	float Smoothness = 100.f;
	
	// Higher priority assets will be processed last
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config")
	int32 Priority = 0;

public:
	TSharedPtr<const FVoxelHeightmap> HeightmapPtr;
	FVoxelHeightmapConfig HeightmapConfig;

	virtual bool IsValid() const override { return HeightmapPtr.IsValid(); }
	virtual void CacheData_GameThread() override;
};

struct VOXELLANDMASS_API FVoxelLandmassHeightmapBrushImpl : TVoxelBrushImpl<FVoxelLandmassHeightmapBrush>
{
	const TSharedRef<const FVoxelHeightmap> Heightmap;

	explicit FVoxelLandmassHeightmapBrushImpl(const FVoxelLandmassHeightmapBrush& Brush, const FMatrix& WorldToLocal);

	FVector2D Scale = FVector2D(ForceInit);
	FQuat2D Rotation = FQuat2D(ForceInit);
	FVector2D Rotation3D = FVector2D(ForceInit);
	FVector2D Position = FVector2D(ForceInit);

	float InnerScaleZ = 0;
	float InnerOffsetZ = 0;

	float ScaleZ = 0;
	float OffsetZ = 0;
	
	virtual float GetDistance(const FVector& LocalPosition) const override;
	virtual TSharedPtr<FVoxelDistanceField> GetDistanceField() const override;
};

USTRUCT(meta = (Internal))
struct FVoxelNode_FVoxelHeightmapDistanceField_GetDistances : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_CALL_PARAM(TSharedPtr<const FVoxelLandmassHeightmapBrushImpl>, Brush);
	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Distance);
};

USTRUCT()
struct FVoxelHeightmapDistanceField : public FVoxelDistanceField
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	TSharedPtr<const FVoxelLandmassHeightmapBrushImpl> Brush;

	virtual TVoxelFutureValue<FVoxelFloatBuffer> GetDistances(const FVoxelQuery& Query) const override;
};