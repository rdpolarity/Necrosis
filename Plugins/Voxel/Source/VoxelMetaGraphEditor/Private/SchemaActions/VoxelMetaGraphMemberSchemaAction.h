// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelMetaGraphMembersBaseSchemaAction.h"
#include "EdGraph/EdGraphSchema.h"
#include "VoxelMetaGraphMemberSchemaAction.generated.h"

struct FVoxelMetaGraphParameter;

USTRUCT()
struct FVoxelMetaGraphMemberSchemaAction : public FVoxelMetaGraphMembersBaseSchemaAction
{
	GENERATED_BODY();

public:
	using FVoxelMetaGraphMembersBaseSchemaAction::FVoxelMetaGraphMembersBaseSchemaAction;
	
	UPROPERTY()
	FGuid ParameterGuid;

	FVoxelMetaGraphParameter* GetParameter() const;
	
	//~ Begin FEdGraphSchemaAction Interface
	virtual void MovePersistentItemToCategory(const FText& NewCategoryName) override;
	
	virtual int32 GetReorderIndexInContainer() const override;
	virtual bool ReorderToBeforeAction(TSharedRef<FEdGraphSchemaAction> OtherAction) override;
	
	virtual FEdGraphSchemaActionDefiningObject GetPersistentItemDefiningObject() const override;

	virtual FVoxelPinType GetPinType() override;
	virtual TArray<FVoxelPinType> GetPinTypes() override;
	virtual void SetPinType(const FVoxelPinType& NewPinType) override;
	virtual FName GetName() override;
	virtual void SetName(const FString& NewName, const TSharedPtr<SVoxelMetaGraphMembers>& MembersWidget) override;
	//~ End FVoxelMetaGraphMembersBaseSchemaAction Interface
};