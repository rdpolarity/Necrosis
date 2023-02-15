// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelRuntime/VoxelSubsystem.h"
#include "VoxelExposeDataNode.generated.h"

struct FVoxelNode_ExposeData;

UCLASS()
class VOXELMETAGRAPH_API UVoxelExposedDataSubsystemProxy : public UVoxelSubsystemProxy
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_PROXY_BODY(FVoxelExposedDataSubsystem);
};

class VOXELMETAGRAPH_API FVoxelExposedDataSubsystem : public IVoxelSubsystem
{
public:
	GENERATED_VOXEL_SUBSYSTEM_BODY(UVoxelExposedDataSubsystemProxy);

	FVoxelPinType GetType(FName Name) const;
	FVoxelFutureValue GetData(FName Name, const FVoxelQuery& Query) const;
	FVoxelFutureValue GetData(FName Name, const FVoxelQuery& Query, const FVector3f& Position) const;

	void RegisterNode(FName Name, const FVoxelNode_ExposeData& Node);
	
private:
	struct FData
	{
		const FVoxelNode_ExposeData& Node;
		const TSharedRef<IVoxelNodeOuter> NodeOuter;

		FData(const FVoxelNode_ExposeData& Node, const TSharedRef<IVoxelNodeOuter>& NodeOuter)
			: Node(Node)
			, NodeOuter(NodeOuter)
		{
		}
	};
	mutable FVoxelCriticalSection CriticalSection;
	TMap<FName, TSharedPtr<FData>> Datas;
};

USTRUCT(Category = "Misc", meta = (NodeColor = "Red", NodeIconColor = "White"))
struct VOXELMETAGRAPH_API FVoxelNode_ExposeData : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FName, Name, "MyData");
	VOXEL_GENERIC_INPUT_PIN(Data);

	virtual bool HasSideEffects() const override
	{
		return true;
	}

	virtual void Initialize() override;

	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;
};

#if 0 // TODO
UCLASS()
class VOXELMETAGRAPH_API UVoxelExposedDataFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	template<typename BufferType, typename T = typename BufferType::UniformType>
	static T GetGraphData(AVoxelActor* Actor, FName Name, FVector Position)
	{
		if (!Actor)
		{
			VOXEL_MESSAGE(Error, "Actor is null");
			return FVoxelUtilities::MakeSafe<T>();
		}

		const TSharedPtr<FVoxelRuntime> Runtime = FVoxelRuntimeUtilities::GetRuntime(Actor);
		if (!Runtime)
		{
			VOXEL_MESSAGE(Error, "Actor Voxel Runtime is not created");
			return FVoxelUtilities::MakeSafe<T>();
		}

		const FVoxelExposedDataSubsystem* Subsystem = Runtime->GetSubsystem<FVoxelExposedDataSubsystem>();
		if (!Subsystem)
		{
			VOXEL_MESSAGE(Error, "Graph has no ExposeData node");
			return FVoxelUtilities::MakeSafe<T>();
		}
		
		TArray<FName> ValidNames;
		Datas.GenerateKeyArray(ValidNames);
		VOXEL_MESSAGE(Error, "No output named {0} found. Valid names: {1}", Name, ValidNames);

		if (NodeType != Type)
		{
			VOXEL_MESSAGE(Error, "Trying to query output {0} as {1}, but it has type {2}", Name, Type.ToString(), NodeType.ToString());
			return {};
		}
		return Subsystem->GetData<BufferType>(Name, Position);
	}

	UFUNCTION(BlueprintCallable, Category = "Voxel|Exposed Data")
	static float GetGraphFloatData(AVoxelActor* Actor, FName Name, FVector Position)
	{
		return GetGraphData<FVoxelFloatBuffer>(Actor, Name, Position);
	}
	
	UFUNCTION(BlueprintCallable, Category = "Voxel|Exposed Data", DisplayName = "Get Graph Vector2D Data")
	static FVector2D GetGraphVector2DData(AVoxelActor* Actor, FName Name, FVector Position)
	{
		return GetGraphData<FVoxelVector2DBuffer>(Actor, Name, Position);
	}
	
	UFUNCTION(BlueprintCallable, Category = "Voxel|Exposed Data")
	static FVector GetGraphVectorData(AVoxelActor* Actor, FName Name, FVector Position)
	{
		return GetGraphData<FVoxelVectorBuffer>(Actor, Name, Position);
	}
	
	UFUNCTION(BlueprintCallable, Category = "Voxel|Exposed Data")
	static FLinearColor GetGraphColorData(AVoxelActor* Actor, FName Name, FVector Position)
	{
		return GetGraphData<FVoxelLinearColorBuffer>(Actor, Name, Position);
	}
};
#endif