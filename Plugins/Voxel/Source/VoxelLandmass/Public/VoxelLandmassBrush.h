// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBrush.h"
#include "Nodes/VoxelDistanceFieldNodes.h"
#include "VoxelLandmassBrush.generated.h"

class FVoxelLandmassBrushData;

USTRUCT()
struct VOXELLANDMASS_API FVoxelLandmassBrush : public FVoxelBrush
{
	GENERATED_BODY()
	GENERATED_VOXEL_BRUSH_BODY()
		
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config")
	TObjectPtr<UStaticMesh> Mesh;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config")
	bool bInvert = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config")
	bool bHermiteInterpolation = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config")
	float Smoothness = 0.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config", meta = (UIMin = -10, UIMax = 10), AdvancedDisplay)
	float DistanceOffset = 0;

	// Higher priority assets will be processed last
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config")
	int32 Priority = 0;

public:
	TSharedPtr<const FVoxelLandmassBrushData> Data;

	virtual bool IsValid() const override { return Data.IsValid(); }
	virtual void CacheData_GameThread() override;
};

struct VOXELLANDMASS_API FVoxelLandmassBrushImpl : TVoxelBrushImpl<FVoxelLandmassBrush>
{
	const TSharedRef<const FVoxelLandmassBrushData> Data;

	FMatrix44f LocalToData = FMatrix44f::Identity;
	float DataToLocalScale = 0.f;

	FVoxelLandmassBrushImpl(const FVoxelLandmassBrush& Brush, const FMatrix& WorldToLocal);

	virtual float GetDistance(const FVector& LocalPosition) const override;
	virtual TSharedPtr<FVoxelDistanceField> GetDistanceField() const override;
};

USTRUCT(meta = (Internal))
struct FVoxelNode_FVoxelMeshDistanceField_GetDistances : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_CALL_PARAM(TSharedPtr<const FVoxelLandmassBrushImpl>, Brush);
	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Distance);
};

USTRUCT()
struct FVoxelMeshDistanceField : public FVoxelDistanceField
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	TSharedPtr<const FVoxelLandmassBrushImpl> Brush;

	virtual TVoxelFutureValue<FVoxelFloatBuffer> GetDistances(const FVoxelQuery& Query) const override;
};