// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "EdGraph/EdGraph.h"

struct FVoxelGraphMessages;

extern VOXELCORE_API FVoxelGraphMessages* GVoxelGraphMessages;

struct VOXELCORE_API FVoxelGraphMessages
{
public:
	const TWeakObjectPtr<const UEdGraph> Graph;

	explicit FVoxelGraphMessages(const UEdGraph* Graph)
		: Graph(Graph)
	{
		FVoxelMessages::OnClearNodeMessages.Broadcast(Graph);

		ensure(!GVoxelGraphMessages);
		GVoxelGraphMessages = this;
	}
	~FVoxelGraphMessages()
	{
		ensure(GVoxelGraphMessages == this);
		GVoxelGraphMessages = nullptr;
	}

	bool HasError() const
	{
		return *HasErrorPtr;
	}

private:
	const TSharedRef<bool> HasErrorPtr = MakeSharedCopy(false);

	class FMessageConsumer : public IVoxelMessageConsumer
	{
	public:
		const TSharedRef<bool> HasErrorPtr;
		const TWeakObjectPtr<const UEdGraph> Graph;

		explicit FMessageConsumer(const FVoxelGraphMessages& Messages)
			: HasErrorPtr(Messages.HasErrorPtr)
			, Graph(Messages.Graph)
		{
		}

		virtual void LogMessage(const TSharedRef<FTokenizedMessage>& Message) override;
	};
	const TSharedRef<FMessageConsumer> MessageConsumer = MakeShared<FMessageConsumer>(*this);
	const FVoxelScopedMessageConsumer ScopedMessageConsumer = FVoxelScopedMessageConsumer(MessageConsumer);
};