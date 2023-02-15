// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelTemplateNode.generated.h"

struct VOXELMETAGRAPH_API FVoxelTemplateNodeUtilities
{
	VOXEL_USE_NAMESPACE_TYPES(MetaGraph, FGraph, FNode, FPin);

public:
	static TArray<FVoxelPinType> GetFloatTypes();
	static TArray<FVoxelPinType> GetIntTypes();

public:
	static bool IsFloat(const FVoxelPinType& PinType);
	static bool IsInt(const FVoxelPinType& PinType);
	static bool IsBool(const FVoxelPinType& PinType);

	static int32 GetDimension(const FVoxelPinType& PinType);

	static FVoxelPinType GetScalarType(const FVoxelPinType& PinType);
	static FVoxelPinType GetFloatType(const FVoxelPinType& Type);
	static FVoxelPinType GetIntType(const FVoxelPinType& Type);

	static TVoxelInstancedStruct<FVoxelNode> GetBreakNode(const FVoxelPinType& PinType);
	static TVoxelInstancedStruct<FVoxelNode> GetMakeNode(const FVoxelPinType& PinType);

public:
	static int32 GetMaxDimension(const TArray<FPin*>& Pins);

	static FPin* GetLinkedPin(FPin* Pin);
	static FPin* ConvertToFloat(FPin* Pin);

	static FPin* ScalarToVector(FPin* Pin, int32 HighestDimension);
	static FPin* ZeroExpandVector(FPin* Pin, int32 HighestDimension);

	static TArray<FPin*> BreakVector(FPin* Pin);
	static FPin* MakeVector(TArray<FPin*> Pins);

	static bool IsPinFloat(const FPin* Pin);
	static bool IsPinInt(const FPin* Pin);
	static bool IsPinBool(const FPin* Pin);

	static bool IsPinOfName(const FPin* Pin, TSet<FName> Names);

private:
	static UScriptStruct* GetConvertToFloatNode(const FPin* Pin, FVoxelPinType& ResultPinType);
	static TVoxelInstancedStruct<FVoxelNode> GetMakeNode(const FPin* Pin, const int32 Dimension);

	static FNode& CreateNode(const FPin* Pin, TVoxelInstancedStruct<FVoxelNode> NodeStruct);

public:
	static bool Any(const TArray<FPin*>& Pins, const TFunctionRef<bool(FPin*)> Lambda);
	static bool All(const TArray<FPin*>& Pins, const TFunctionRef<bool(FPin*)> Lambda);
	static TArray<FPin*> Filter(const TArray<FPin*>& Pins, const TFunctionRef<bool(const FPin*)> Lambda);

	static FPin* Reduce(TArray<FPin*> Pins, TFunctionRef<FPin*(FPin*, FPin*)> Lambda);

	template<typename T, typename = typename TEnableIf<TIsDerivedFrom<T, FVoxelNode>::Value>::Type>
	static FPin* Call_Single(const TArray<FPin*>& Pins)
	{
		return Call_Single(T::StaticStruct(), Pins);
	}
	template<typename... TArg, typename = typename TEnableIf<TAnd<TIsSame<TArg, FPin*>...>::Value>::Type>
	static FPin* Call_Single(UScriptStruct* NodeStruct, TArg... Args)
	{
		return Call_Single(NodeStruct, { Args... });
	}
	template<typename T, typename... TArg, typename = typename TEnableIf<TAnd<TIsSame<TArg, FPin*>..., TIsDerivedFrom<T, FVoxelNode>>::Value>::Type>
	static FPin* Call_Single(TArg... Args)
	{
		return Call_Single(T::StaticStruct(), { Args... });
	}
	static FPin* Call_Single(UScriptStruct* NodeStruct, const TArray<FPin*>& Pins);
	
	template<typename T, typename = typename TEnableIf<TIsDerivedFrom<T, FVoxelNode>::Value>::Type>
	static TArray<FPin*> Call_Multi(const TArray<TArray<FPin*>>& Pins)
	{
		return Call_Multi(T::StaticStruct(), Pins);
	}
	template<typename... TArg, typename = typename TEnableIf<TAnd<TIsSame<TArg, TArray<FPin*>>...>::Value>::Type>
	static TArray<FPin*> Call_Multi(UScriptStruct* NodeStruct, TArg... Args)
	{
		return Call_Multi(NodeStruct, { Args... });
	}
	template<typename T, typename... TArg, typename = typename TEnableIf<TAnd<TIsSame<TArg, TArray<FPin*>>..., TIsDerivedFrom<T, FVoxelNode>>::Value>::Type>
	static TArray<FPin*> Call_Multi(TArg... Args)
	{
		return Call_Multi(T::StaticStruct(), { Args... });
	}
	static TArray<FPin*> Call_Multi(UScriptStruct* NodeStruct, const TArray<TArray<FPin*>>& Pins);

	template<typename LambdaType, typename... TArgs, typename = typename TEnableIf<TIsInvocable<LambdaType, FPin*, TArgs...>::Value>::Type>
	static TArray<FPin*> Apply(TConstArrayView<FPin*> InPins, LambdaType&& Lambda, TArgs... Args)
	{
		TArray<FPin*> Pins(InPins);
		for (FPin*& Pin : Pins)
		{
			Pin = Lambda(Pin, Args...);
		}
		return Pins;
	}

	template<typename LambdaType, typename... TArgs, typename = typename TEnableIf<TIsInvocable<LambdaType, FPin*, TArgs...>::Value>::Type>
	static TArray<TArray<FPin*>> ApplyVector(TConstArrayView<FPin*> InPins, LambdaType&& Lambda, TArgs... Args)
	{
		TArray<TArray<FPin*>> Result;
		TArray<FPin*> Pins(InPins);
		for (FPin*& Pin : Pins)
		{
			Result.Add(Lambda(Pin, Args...));
		}
		return Result;
	}
};

USTRUCT(Category = "Template", meta = (Abstract))
struct VOXELMETAGRAPH_API FVoxelTemplateNode : public FVoxelNode, public FVoxelTemplateNodeUtilities
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	FVoxelTemplateNode() = default;

	VOXEL_USE_NAMESPACE_TYPES(MetaGraph, FNode, FGraph);

	virtual EExecType GetExecType() const final override { unimplemented(); return {}; }
	virtual void ExpandNode(FGraph& Graph, FNode& Node) const;
	virtual void ExpandPins(FNode& Node, TArray<FPin*> Pins, const TArray<FPin*>& AllPins, TArray<FPin*>& OutPins) const
	{
		OutPins = { ExpandPins(Node, Pins, AllPins) };
	}
	virtual FPin* ExpandPins(FNode& Node, TArray<FPin*> Pins, const TArray<FPin*>& AllPins) const VOXEL_PURE_VIRTUAL({});
};
