// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelPinType.h"
#include "EdGraph/EdGraphSchema.h"
#include "VoxelMetaGraphMembersBaseSchemaAction.generated.h"

class UVoxelMetaGraph;
class FVoxelMetaGraphEditorToolkit;
VOXEL_FWD_NAMESPACE_CLASS(SVoxelMetaGraphMembers, MetaGraph, SMembers);

USTRUCT()
struct FVoxelMetaGraphMembersBaseSchemaAction : public FEdGraphSchemaAction
{
	GENERATED_BODY();

public:
	using FEdGraphSchemaAction::FEdGraphSchemaAction;

	TWeakPtr<FVoxelMetaGraphEditorToolkit> WeakToolkit;

	UPROPERTY()
	TWeakObjectPtr<UVoxelMetaGraph> MetaGraph;

	virtual FVoxelPinType GetPinType() VOXEL_PURE_VIRTUAL({});
	virtual TArray<FVoxelPinType> GetPinTypes() VOXEL_PURE_VIRTUAL({});
	virtual void SetPinType(const FVoxelPinType& NewPinType) VOXEL_PURE_VIRTUAL();

	virtual FName GetName() VOXEL_PURE_VIRTUAL({});
	virtual void SetName(const FString& NewName, const TSharedPtr<SVoxelMetaGraphMembers>& MembersWidget) VOXEL_PURE_VIRTUAL();
};