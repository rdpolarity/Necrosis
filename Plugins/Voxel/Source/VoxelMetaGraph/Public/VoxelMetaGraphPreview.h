// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelMetaGraphPreview.generated.h"

class FDeferredCleanupSlateBrush;

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelMetaGraphPreview : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	virtual TSharedPtr<SWidget> MakeWidget() VOXEL_PURE_VIRTUAL({});

	virtual FString GetValue(const FVector2D& Position) const VOXEL_PURE_VIRTUAL({});
	virtual FString GetMinValue() const VOXEL_PURE_VIRTUAL({});
	virtual FString GetMaxValue() const VOXEL_PURE_VIRTUAL({});
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelMetaGraphPreviewFactory : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	static const FVoxelMetaGraphPreviewFactory* FindFactory(const FVoxelPinType& Type);

public:
	virtual bool CanCreate(const FVoxelPinType& Type) const;

	struct FParameters
	{
		int32 PreviewSize = 0;
		FMatrix PixelToLocal;
		bool bNormalize = true;
		TFunction<FVoxelFutureValue(const FVoxelQuery& Query)> ExecuteQuery;
		TSharedPtr<FVoxelMetaGraphPreview> PreviousPreview;
		TFunction<void(const TSharedRef<FVoxelMetaGraphPreview>&)> Finalize;
	};
	virtual void Create(const FParameters& Parameters) const VOXEL_PURE_VIRTUAL();

public:
	static FVoxelQuery MakeDefaultQuery(const FParameters& Parameters);
	static FVoxelVectorBuffer ComputePositions(const FParameters& Parameters, FVoxelBox* OutBounds = nullptr);
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelMetaGraphPreview_Texture : public FVoxelMetaGraphPreview
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	using FGenerate = TFunction<void(bool bNormalize, int32 PreviewSize, TVoxelArrayView<FLinearColor> Colors)>;
	using FGetTooltip = TFunction<FString(int32 Index)>;
	
	int32 PreviewSize = 0;
	FMatrix PixelToLocal;
	bool bNormalize = true;

	TWeakObjectPtr<UTexture2D> Texture;
	TSharedPtr<FDeferredCleanupSlateBrush> Brush;

	FGenerate GenerateColors;
	FGetTooltip GetTooltip;

	FString MinValue;
	FString MaxValue;

	void UpdateTexture(const FSimpleDelegate& OnComplete);
	virtual FString GetValue(const FVector2D& Position) const override;
	virtual FString GetMinValue() const override;
	virtual FString GetMaxValue() const override;

	virtual TSharedPtr<SWidget> MakeWidget() override;
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelMetaGraphPreviewFactory_Texture : public FVoxelMetaGraphPreviewFactory
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	static void RenderTexture(
		const FParameters& Parameters,
		const TSharedRef<FVoxelMetaGraphPreview_Texture>& Preview);
};