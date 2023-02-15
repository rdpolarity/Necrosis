// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelExecCodeGenNode.generated.h"

USTRUCT(meta = (Internal))
struct VOXELMETAGRAPH_API FVoxelNode_ExecCodeGen : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()
	VOXEL_USE_NAMESPACE_TYPES(MetaGraph, FGraph, FPin);

public:
	using Super::CreateInputPin;
	using Super::CreateOutputPin;

	TSharedPtr<FGraph> Graph;

	TArray<const FPin*> GraphInputPins;
	const FPin* GraphOutputPin = nullptr;
	
	TArray<FVoxelPinRef> InputPinRefs;
	FVoxelPinRef OutputPinRef;

	//~ Begin FVoxelNode Interface
	virtual TVoxelFunction<FVoxelFutureValue(const FVoxelQuery&)> Compile(FName PinName) const override;
	//~ End FVoxelNode Interface

private:
	struct FBuffer
	{
		FVoxelPinType InnerType;
		int32 Num = 0;
		TSharedPtr<const TVoxelArray<uint8>> Data;

		FVoxelRDGBuffer RDGBuffer;
		TSharedPtr<FVoxelRDGExternalBuffer> GpuBuffer;
	};
	struct FStep
	{
		int32 NodeId = 0;
		bool bIsPassthrough = false;
		TVoxelArray<int32> InputRegisters;
		TVoxelArray<int32> OutputRegisters;
	};
	struct FState
	{
		TVoxelArray<FStep> Steps;
		TVoxelArray<FVoxelPinType> RegisterTypes;
		TMap<int32, FBuffer> DefaultBuffers;
		TVoxelArray<int32> OutputRegisters;
	};

	FVoxelFutureValue ExecuteGpu(
		const FVoxelQuery& Query,
		const TSharedRef<const FState>& State) const;

	FVoxelSharedPinValue ExecuteGpu(
		FRDGBuilder& GraphBuilder,
		const TVoxelArray<TSharedPtr<const FVoxelBuffer>>& InputValues,
		const TSharedRef<const FState>& State) const;

	FVoxelFutureValue ExecuteCpu(
		const FVoxelQuery& Query,
		const bool bIsBuffer,
		const TSharedRef<const FState>& State) const;

	FVoxelSharedPinValue ExecuteCpu(
		const TVoxelArray<TSharedPtr<const FVoxelBufferView>>& InputValues,
		const bool bIsBuffer,
		const TSharedRef<const FState>& State) const;

	template<typename T>
	bool CheckBufferSizes(
		const TVoxelArray<TSharedPtr<const T>>& InputValues,
		int32& Num) const;
};