// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelLoopNodes.h"

VOXEL_CONSOLE_VARIABLE(
	VOXELMETAGRAPH_API, int32, GVoxelMetaGraphLoopLimit, 100,
	"voxel.metagraph.LoopLimit",
	"Max loops a while node can do in a meta graph");

DEFINE_VOXEL_NODE(FVoxelNode_GetWhileLoopIndex, Index)
{
	FindVoxelQueryData(FVoxelWhileLoopQueryData, WhileLoopQueryData);

	return WhileLoopQueryData->Index;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_GetWhileLoopPreviousValue, Value)
{
	FindVoxelQueryData(FVoxelWhileLoopQueryData, WhileLoopQueryData);

	if (WhileLoopQueryData->Value.GetType() != ReturnPinType)
	{
		VOXEL_MESSAGE(Error, "{0}: Invalid pin type, should be {1}", this, WhileLoopQueryData->Value.GetType().ToString());
		return {};
	}

	return WhileLoopQueryData->Value;
}

FVoxelPinTypeSet FVoxelNode_GetWhileLoopPreviousValue::GetPromotionTypes(const FVoxelPin& Pin) const
{
	FVoxelPinTypeSet OutTypes;

	OutTypes.Add<float>();
	OutTypes.Add<FVector2D>();
	OutTypes.Add<FVector>();
	OutTypes.Add<FLinearColor>();

	OutTypes.Add<int32>();
	OutTypes.Add<FIntPoint>();
	OutTypes.Add<FIntVector>();

	return OutTypes;
}

void FVoxelNode_GetWhileLoopPreviousValue::PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	GetPin(ValuePin).SetType(NewType.GetBufferType());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_WhileLoop, Result)
{
	const FVoxelFutureValue FirstValue = Get(FirstValuePin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, FirstValue)
	{
		const TSharedRef<FVoxelWhileLoopQueryData> QueryData = MakeShared<FVoxelWhileLoopQueryData>();
		QueryData->Index = 0;
		QueryData->Value = FirstValue;

		FVoxelQuery ChildQuery = Query;
		ChildQuery.Add(QueryData);

		const TSharedRef<TFunction<FVoxelFutureValue()>> Function = MakeShared<TFunction<FVoxelFutureValue()>>();

		*Function = [VOXEL_ON_COMPLETE_CAPTURE, Function, QueryData, ChildQuery]
		{
			const TValue<bool> Continue = Get(ContinuePin, ChildQuery);

			return VOXEL_ON_COMPLETE(AnyThread, Function, QueryData, ChildQuery, Continue)
			{
				if (!Continue)
				{
					return QueryData->Value;
				}

				if (QueryData->Index > GVoxelMetaGraphLoopLimit)
				{
					VOXEL_MESSAGE(Error, "{0}: Loop limit of {1} reached", this, GVoxelMetaGraphLoopLimit);
					return {};
				}

				const FVoxelFutureValue NextValue = Get(NextValuePin, ChildQuery);

				return VOXEL_ON_COMPLETE(AnyThread, Function, QueryData, NextValue)
				{
					QueryData->Index++;
					QueryData->Value = NextValue;

					return (*Function)();
				};
			};
		};

		return (*Function)();
	};
}

FVoxelPinTypeSet FVoxelNode_WhileLoop::GetPromotionTypes(const FVoxelPin& Pin) const
{
	return FVoxelPinTypeSet::All();
}

void FVoxelNode_WhileLoop::PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	GetPin(FirstValuePin).SetType(NewType);
	GetPin(NextValuePin).SetType(NewType);
	GetPin(ResultPin).SetType(NewType);
}