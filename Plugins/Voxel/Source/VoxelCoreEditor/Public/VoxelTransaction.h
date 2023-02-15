// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "ScopedTransaction.h"

struct VOXELCOREEDITOR_API FVoxelTransaction
{
public:
	explicit FVoxelTransaction(const TWeakObjectPtr<UObject>& Object, const FString& Text = {})
		: Object(Object)
		, Transaction(FText::FromString(Text), !GIsTransacting)
	{
		if (ensure(Object.IsValid()))
		{
			Object->PreEditChange(nullptr);
		}
	}
	explicit FVoxelTransaction(UObject& Object, const FString& Text = {})
		: FVoxelTransaction(&Object, Text)
	{
	}
	explicit FVoxelTransaction(const UEdGraphPin* Pin, const FString& Text = {})
		: FVoxelTransaction(Pin ? Pin->GetOwningNode() : nullptr, Text)
	{
	}
	~FVoxelTransaction()
	{
		if (ensure(Object.IsValid()))
		{
			Object->PostEditChange();
		}
	}

private:
	const TWeakObjectPtr<UObject> Object;
	const FScopedTransaction Transaction;
};