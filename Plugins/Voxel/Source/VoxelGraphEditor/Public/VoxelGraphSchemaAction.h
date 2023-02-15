// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "VoxelGraphSchemaAction.generated.h"

USTRUCT()
struct VOXELGRAPHEDITOR_API FVoxelGraphSchemaAction : public FEdGraphSchemaAction
{
	GENERATED_BODY();

public:
	using FEdGraphSchemaAction::FEdGraphSchemaAction;

	virtual FName GetTypeId() const final override
	{
		return StaticGetTypeId();
	}

	static FName StaticGetTypeId()
	{
		static const FName TypeId("FVoxelGraphSchemaAction");
		return TypeId;
	}

	virtual void GetIcon(FSlateIcon& Icon, FLinearColor& Color)
	{
		static const FSlateIcon DefaultIcon("EditorStyle", "NoBrush");
		Icon = DefaultIcon;
		Color = FLinearColor::White;
	}
};

USTRUCT()
struct VOXELGRAPHEDITOR_API FVoxelGraphSchemaAction_NewComment : public FVoxelGraphSchemaAction
{
	GENERATED_BODY();

public:
	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	virtual void GetIcon(FSlateIcon& Icon, FLinearColor& Color) override;
};

USTRUCT()
struct VOXELGRAPHEDITOR_API FVoxelGraphSchemaAction_Paste : public FVoxelGraphSchemaAction
{
	GENERATED_BODY();

public:
	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
};