// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

class SPreviewScale : public SCompoundWidget
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_ATTRIBUTE(double, Value)
		SLATE_ATTRIBUTE(int32, Resolution)
		SLATE_ATTRIBUTE(TWeakPtr<SWidget>, SizeWidget)
	};

	void Construct(const FArguments& InArgs);

	void Enable() const;

private:
	TAttribute<double> Value;
	TAttribute<int32> Resolution;
	TAttribute<TWeakPtr<SWidget>> SizeWidget;

	TSharedPtr<SWidget> BoxWidget;
};

END_VOXEL_NAMESPACE(MetaGraph)