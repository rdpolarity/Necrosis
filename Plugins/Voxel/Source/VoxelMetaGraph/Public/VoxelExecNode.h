// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelExecNode.generated.h"

struct FVoxelChunkRef;

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelExecBase : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelExec : public FVoxelExecBase
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelChunkExec : public FVoxelExecBase
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelExecObject
	: public FVoxelVirtualStruct
	, public TSharedFromThis<FVoxelExecObject>
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	FVoxelExecObject() = default;
	virtual ~FVoxelExecObject() override;

	void Initialize(const FVoxelNode& Node);

	virtual void Create(FVoxelRuntime& Runtime);
	virtual void Destroy(FVoxelRuntime& Runtime);
	virtual void Tick(FVoxelRuntime& Runtime);
	virtual void OnChunksComplete(FVoxelRuntime& Runtime);

protected:
	const FVoxelNode& GetNode() const
	{
		check(PrivateNode);
		return *PrivateNode;
	}

	TSharedRef<FVoxelChunkRef> CreateChunk(const FVoxelQuery& Query) const;

private:
	bool bIsCreated = false;
	const FVoxelNode* PrivateNode = nullptr;
	FName ChunkExecPinName;
	FName ChunkOnCompletePinName;
	TSharedPtr<IVoxelNodeOuter> NodeOuter;
	TSharedPtr<FVoxelTaskStat> Stat;
	TSharedPtr<FVoxelChunkManager> ResultManager;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelChunkExecObject : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	FVoxelChunkExecObject() = default;
	virtual ~FVoxelChunkExecObject() override;

	void CallCreate(FVoxelRuntime& Runtime) const;
	void CallDestroy(FVoxelRuntime& Runtime) const;

protected:
	virtual void Create(FVoxelRuntime& Runtime) const {}
	virtual void Destroy(FVoxelRuntime& Runtime) const {}

private:
	mutable bool bIsCreated = false;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(Category = "Exec", meta = (Abstract, NodeColor = "Red", NodeIconColor = "White"))
struct VOXELMETAGRAPH_API FVoxelExecNode : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	virtual void Execute(TArray<TValue<FVoxelExecObject>>& OutObjects) const VOXEL_PURE_VIRTUAL();
};

USTRUCT(meta = (Abstract))
struct VOXELMETAGRAPH_API FVoxelExecNode_Default : public FVoxelExecNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
		
	VOXEL_INPUT_PIN(FVoxelExec, Exec, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelExec, Then);

	virtual void Execute(TArray<TValue<FVoxelExecObject>>& OutObjects) const final override;

public:
	VOXEL_SETUP_ON_COMPLETE_MANUAL(FVoxelExecObject, "ExecObject");

	virtual TValue<FVoxelExecObject> Execute(const FVoxelQuery& Query) const VOXEL_PURE_VIRTUAL({});
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(Category = "Exec", meta = (Abstract, NodeColor = "Red", NodeIconColor = "White"))
struct VOXELMETAGRAPH_API FVoxelChunkExecNode : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual void Execute(const FVoxelQuery& Query, TArray<TValue<FVoxelChunkExecObject>>& OutObjects) const VOXEL_PURE_VIRTUAL();
};

USTRUCT(meta = (Abstract))
struct VOXELMETAGRAPH_API FVoxelChunkExecNode_Default : public FVoxelChunkExecNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	VOXEL_INPUT_PIN(FVoxelChunkExec, Exec, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelChunkExec, Then);

	virtual void Execute(const FVoxelQuery& Query, TArray<TValue<FVoxelChunkExecObject>>& OutObjects) const final override;

public:
	VOXEL_SETUP_ON_COMPLETE_MANUAL(FVoxelChunkExecObject, "ChunkExecObject");

	virtual TValue<FVoxelChunkExecObject> Execute(const FVoxelQuery& Query) const VOXEL_PURE_VIRTUAL({});
};