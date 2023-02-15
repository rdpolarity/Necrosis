// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"

struct FVoxelNodeLibrary
{
public:
	static const TVoxelInstancedStruct<FVoxelNode>& GetNodeChecked(const UScriptStruct* Struct)
	{
		return Get().Nodes[Struct];
	}
	static const TMap<UScriptStruct*, TVoxelInstancedStruct<FVoxelNode>>& GetNodes()
	{
		return Get().Nodes;
	}

	static UScriptStruct* FindMakeNode(const FVoxelPinType& Type)
	{
		return Get().MakeNodes.FindRef(Type);
	}
	static UScriptStruct* FindBreakNode(const FVoxelPinType& Type)
	{
		return Get().BreakNodes.FindRef(Type);
	}

	static UScriptStruct* FindCastNode(const FVoxelPinType& From, const FVoxelPinType& To)
	{
		return Get().CastNodes.FindRef({ From, To });
	}

#if 0 // TODO
	static TArray<FVoxelPinType> GetParameterTypes()
	{
		TArray<FVoxelPinType> EnumTypes;
		for (UEnum* Enum : TObjectRange<UEnum>())
		{
			if (UEdGraphSchema_K2::IsAllowableBlueprintVariableType(Enum))
			{
				EnumTypes.Add(FVoxelPinType::MakeEnum(Enum));
			}
		}

		EnumTypes.Sort([&](const FVoxelPinType& A, const FVoxelPinType& B)
		{
			return A.ToString() < B.ToString();
		});

		TArray<FVoxelPinType> Types = Get().ParameterTypes;
		Types.Append(EnumTypes);
		return Types;
	}
#else
	static const TArray<FVoxelPinType>& GetParameterTypes()
	{
		return Get().ParameterTypes;
	}
#endif

private:
	TMap<UScriptStruct*, TVoxelInstancedStruct<FVoxelNode>> Nodes;
	TMap<FVoxelPinType, UScriptStruct*> MakeNodes;
	TMap<FVoxelPinType, UScriptStruct*> BreakNodes;
	TMap<TPair<FVoxelPinType, FVoxelPinType>, UScriptStruct*> CastNodes;

	TArray<FVoxelPinType> ParameterTypes;

	FVoxelNodeLibrary();

	static const FVoxelNodeLibrary& Get();
};