// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMetaGraphGraph.h"
#include "VoxelGraphEditorToolkit.h"

class IMessageLogListing;

VOXEL_FWD_NAMESPACE_CLASS(FVoxelMetaGraphDebugGraph, MetaGraph, FGraph);
VOXEL_FWD_NAMESPACE_CLASS(SVoxelMetaGraphMembers, MetaGraph, SMembers);
VOXEL_FWD_NAMESPACE_CLASS(SVoxelMetaGraphPreview, MetaGraph, SPreview);
VOXEL_FWD_NAMESPACE_CLASS(SVoxelMetaGraphPreviewStats, MetaGraph, SPreviewStats);

class FVoxelMetaGraphEditorToolkit : public FVoxelGraphEditorToolkit, public FVoxelTicker
{
public:
	FVoxelMetaGraphEditorToolkit()
		: FVoxelGraphEditorToolkit("FVoxelMetaGraphEditorToolkit")
	{
	}
	
public:
	FName FindGraphId(const UEdGraph* Graph) const;
	UEdGraph* GetGraph(FName GraphId) const;
	TSharedPtr<SGraphEditor> GetGraphEditor(FName GraphId) const;
	FName GetActiveGraphId() const;
	void CreateGraph(FName GraphId);

	UEdGraph* GetActiveGraph() const
	{
		return GetGraph(GetActiveGraphId());
	}
	virtual TSharedPtr<SGraphEditor> GetActiveGraphEditor() const override
	{
		return GetGraphEditor(GetActiveGraphId());
	}

public:
	void QueueRefresh()
	{
		bRefreshQueued = true;
	}
	TSharedRef<SVoxelMetaGraphMembers> GetGraphMembers() const
	{
		return GraphMembers.ToSharedRef();
	}

	//~ Begin FVoxelGraphEditorToolkit Interface
	virtual TArray<TSharedPtr<SGraphEditor>> GetGraphEditors() const override;

	virtual void BindToolkitCommands() override;
	virtual void BuildToolbar(FToolBarBuilder& ToolbarBuilder) override;
	virtual TSharedRef<FTabManager::FLayout> GetLayout() const override;
	virtual void RegisterTabs(FRegisterTab RegisterTab) override;

	virtual void SetupObjectToEdit() override;
	virtual void CreateInternalWidgets() override;
	virtual void OnGraphChangedImpl() override;
	virtual void FixupGraphProperties() override;
	virtual void OnSelectedNodesChanged(const TSet<UObject*>& NewSelection) override;
	//~ End FVoxelGraphEditorToolkit Interface

	//~ Begin FVoxelTicker Interface
	virtual void Tick() override;
	//~ End FVoxelTicker Interface

	bool CompileGraph();
	void OnDebugGraph(const FVoxelMetaGraphDebugGraph& Graph);

	FVector2D FindLocationInGraph() const;

	void SelectParameter(FGuid ParameterId, bool bForceRefresh);

private:
	static constexpr const TCHAR* ViewportTabId = TEXT("FVoxelMetaGraphEditorToolkit_Viewport");
	static constexpr const TCHAR* DetailsTabId = TEXT("FVoxelMetaGraphEditorToolkit_Details");
	static constexpr const TCHAR* MessagesTabId = TEXT("FVoxelMetaGraphEditorToolkit_Messages");
	static constexpr const TCHAR* GraphTabId = TEXT("FVoxelMetaGraphEditorToolkit_Graph");
	static constexpr const TCHAR* DebugGraphTabId = TEXT("FVoxelMetaGraphEditorToolkit_DebugGraph");
	static constexpr const TCHAR* MembersTabId = TEXT("FVoxelMetaGraphEditorToolkit_Members");
	static constexpr const TCHAR* PreviewStatsTabId = TEXT("FVoxelMetaGraphEditorToolkit_PreviewStats");

	static constexpr const TCHAR* MainGraphId = TEXT("Main");
	static constexpr const TCHAR* DebugGraphId = TEXT("Debug");

	TSharedPtr<SWidget> MessagesWidget;
	TSharedPtr<IMessageLogListing> MessagesListing;
	TSharedPtr<SVoxelMetaGraphPreview> GraphPreview;
	TSharedPtr<SVoxelMetaGraphPreviewStats> GraphPreviewStats;
	TSharedPtr<SVoxelMetaGraphMembers> GraphMembers;
	TMap<FName, TSharedPtr<SGraphEditor>> GraphEditorsMap;

	bool bRefreshQueued = false;
	bool bIsInOnDebugGraph = false;

	bool bUpdateOnChange = true;

	FGuid TargetParameterId;
};