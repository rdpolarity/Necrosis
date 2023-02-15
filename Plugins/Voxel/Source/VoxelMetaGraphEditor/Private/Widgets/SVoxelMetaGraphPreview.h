// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelMetaGraphPreview.h"

class AVoxelActor;
class UVoxelMetaGraph;
class AVoxelMetaGraphPreviewActor;

BEGIN_VOXEL_NAMESPACE(MetaGraph)

class SPreviewImage;
class SPreviewScale;
class SPreviewRuler;
class SPreviewStats;
class SDepthSlider;

class SPreview : public SCompoundWidget
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_ARGUMENT(UVoxelMetaGraph*, MetaGraph);
	};

	void Construct(const FArguments& Args);

	void QueueUpdate()
	{
		bUpdateQueued = true;
	}

	TSharedPtr<SPreviewStats> ConstructStats() const;

	//~ Begin SWidget Interface
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	//~ End SWidget Interface

private:
	TWeakObjectPtr<UVoxelMetaGraph> MetaGraph;

	FThreadSafeBool bUpdateQueued = false;
	FThreadSafeBool bUpdateProcessing = false;
	double ProcessingStartTime = 0;

	TSharedPtr<FVoxelMetaGraphPreview> Preview;
	TSharedPtr<SScaleBox> PreviewScaleBox;
	TSharedPtr<SPreviewImage> PreviewImage;
	TSharedPtr<SPreviewScale> PreviewScale;
	TSharedPtr<SPreviewRuler> PreviewRuler;
	TSharedPtr<STextBlock> PreviewMessage;
	TSharedPtr<SDepthSlider> DepthSlider;

	TWeakObjectPtr<AVoxelMetaGraphPreviewActor> PreviewActor;

	bool bNormalizePreviewColors = true;

	FVector2D CurrentMousePosition;

	FString CurrentLocation;
	FString CurrentValue;
	FString MinValue;
	FString MaxValue;

	bool bLockCoordinate = false;
	bool bIsCoordinateLocked = false;
	FVector LockedCoordinate;

	void Update();
	FMatrix GetPixelToWorld(int32 InPreviewSize) const;
	AVoxelActor* GetVoxelActor();

	void UpdateMessage(const FString& Message) const;
};

END_VOXEL_NAMESPACE(MetaGraph)