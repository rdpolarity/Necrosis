// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelPinType.h"
#include "VoxelPinValue.h"
#include "VoxelMetaGraph.generated.h"

struct FVoxelNode;
class UVoxelMetaGraph;

VOXEL_FWD_NAMESPACE_CLASS(FVoxelMetaGraphDebugGraph, MetaGraph, FGraph);

USTRUCT()
struct FVoxelMetaGraphCompiledPinRef
{
	GENERATED_BODY()

	UPROPERTY()
	int32 NodeIndex = -1;

	UPROPERTY()
	int32 PinIndex = -1;
		
	UPROPERTY()
	bool bIsInput = false;
};

USTRUCT()
struct FVoxelMetaGraphCompiledPin
{
	GENERATED_BODY()

	UPROPERTY()
	FVoxelPinType Type;

	UPROPERTY()
	FName PinName;
	
	UPROPERTY()
	FVoxelPinValue DefaultValue;

	UPROPERTY()
	TArray<FVoxelMetaGraphCompiledPinRef> LinkedTo;
};

UENUM()
enum class EVoxelMetaGraphCompiledNodeType : uint8
{
	Struct,
	Macro,
	Parameter,
	MacroInput,
	MacroOutput
};

USTRUCT()
struct FVoxelMetaGraphCompiledNode
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UEdGraphNode> SourceGraphNode = nullptr;

	UPROPERTY()
	EVoxelMetaGraphCompiledNodeType Type = {};
	
	UPROPERTY()
#if CPP
	TVoxelInstancedStruct<FVoxelNode> Struct;
#else
	FVoxelInstancedStruct Struct;
#endif
	
	UPROPERTY()
	TObjectPtr<UVoxelMetaGraph> MetaGraph = nullptr;

	UPROPERTY()
	FGuid Guid;

	UPROPERTY()
	TArray<FVoxelMetaGraphCompiledPin> InputPins;

	UPROPERTY()
	TArray<FVoxelMetaGraphCompiledPin> OutputPins;
};

USTRUCT()
struct FVoxelMetaGraphCompiledGraph
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FVoxelMetaGraphCompiledNode> Nodes;

	FVoxelMetaGraphCompiledPin* FindPin(const FVoxelMetaGraphCompiledPinRef& Ref)
	{
		if (!Nodes.IsValidIndex(Ref.NodeIndex))
		{
			return nullptr;
		}

		FVoxelMetaGraphCompiledNode& Node = Nodes[Ref.NodeIndex];
		TArray<FVoxelMetaGraphCompiledPin>& Pins = Ref.bIsInput ? Node.InputPins : Node.OutputPins;
		if (!Pins.IsValidIndex(Ref.PinIndex))
		{
			return nullptr;
		}

		return &Pins[Ref.PinIndex];
	}
	const FVoxelMetaGraphCompiledPin* FindPin(const FVoxelMetaGraphCompiledPinRef& Ref) const
	{
		return VOXEL_CONST_CAST(this)->FindPin(Ref);
	}
};

UENUM()
enum class EVoxelMetaGraphParameterType
{
	Parameter,
	MacroInput,
	MacroOutput,
	LocalVariable,
	Count
};
ENUM_RANGE_BY_COUNT(EVoxelMetaGraphParameterType, EVoxelMetaGraphParameterType::Count);

USTRUCT()
struct FVoxelMetaGraphParameter
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid Guid;

	UPROPERTY(EditAnywhere, Category = "Voxel")
	FName Name = "NewParam";
	
	UPROPERTY(EditAnywhere, Category = "Voxel")
	FVoxelPinType Type;

	UPROPERTY(EditAnywhere, Category = "Voxel")
	FString Category;

	UPROPERTY(EditAnywhere, Category = "Voxel")
	FString Description;
	
	UPROPERTY(EditAnywhere, Category = "Voxel")
	FVoxelPinValue DefaultValue;

	UPROPERTY()
	EVoxelMetaGraphParameterType ParameterType = {};

	bool operator==(const FGuid& OtherGuid) const
	{
		return Guid == OtherGuid;
	}
	bool operator==(FName OtherName) const
	{
		return Name == OtherName;
	}
};

USTRUCT()
struct FVoxelMetaGraphParameterCategories
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FString> Categories;
};

UCLASS(meta = (VoxelAssetType, AssetColor=LightBlue))
class VOXELMETAGRAPH_API UVoxelMetaGraph : public UObject
{
	GENERATED_BODY()

public:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TMap<FName, TObjectPtr<UEdGraph>> Graphs;
#endif

	UEdGraph* GetMainGraph() const
	{
#if WITH_EDITOR
		return Graphs.FindRef("Main");
#else
		return nullptr;
#endif
	}

public:
	UPROPERTY(EditAnywhere, Category = "Config", meta = (InlineEditConditionToggle))
	bool bOverrideMacroName = false;

	UPROPERTY(EditAnywhere, DisplayName = "Name", Category = "Config", meta = (EditCondition = "bOverrideMacroName"))
	FString MacroName;

	UPROPERTY(EditAnywhere, Category = "Config")
	FString Tooltip;
	
	UPROPERTY(EditAnywhere, Category = "Config")
	FString Category = "Misc";

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Config", AssetRegistrySearchable)
	FString Description;

	// Choose title bar color for this macro usages
	UPROPERTY(EditAnywhere, Category = "Config", meta = (EditCondition = "bIsMacroGraph", EditConditionHides))
	FLinearColor InstanceColor = FLinearColor::Gray;
#endif

	UPROPERTY(EditAnywhere, Category = "Config", AssetRegistrySearchable)
	bool bIsMacroGraph = false;

	UPROPERTY()
	int32 PreviewSize = 512;

	UPROPERTY(EditAnywhere, Category = "Config")
	TArray<FVoxelMetaGraphParameter> Parameters;

	UPROPERTY()
	TMap<EVoxelMetaGraphParameterType, FVoxelMetaGraphParameterCategories> ParametersCategories;

	UPROPERTY()
	FVoxelMetaGraphCompiledGraph CompiledGraph;

	FSimpleMulticastDelegate OnChanged;
	FSimpleMulticastDelegate OnParametersChanged;

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnDebugGraph, const TSharedRef<FVoxelMetaGraphDebugGraph>& Graph);
	FOnDebugGraph OnDebugGraph;

	UPROPERTY()
	FEdGraphPinReference PreviewedPin;

public:
	UVoxelMetaGraph();

public:
	//~ Begin UObject Interface
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface

public:
	FString GetMacroName() const;

	void UpdateVoxelActors() const;
	void FixupParameters();

	FVoxelMetaGraphParameter* FindParameterByGuid(const FGuid& TargetGuid)
	{
		return Parameters.FindByKey(TargetGuid);
	}

	FVoxelMetaGraphParameter* FindParameterByName(const EVoxelMetaGraphParameterType ParameterType, const FName TargetName)
	{
		return Parameters.FindByPredicate([&](const FVoxelMetaGraphParameter& Parameter)
		{
			return
				Parameter.ParameterType == ParameterType &&
				Parameter.Name == TargetName;
		});
	}

	TArray<FString>& GetCategories(const EVoxelMetaGraphParameterType Type)
	{
		return ParametersCategories[Type].Categories;
	}

private:
	void FixupCategories();
};