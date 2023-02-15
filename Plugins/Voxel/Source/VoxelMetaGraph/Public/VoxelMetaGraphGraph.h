// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMetaGraph.h"

struct FVoxelNode;
struct FVoxelTemplateNodeUtilities;

BEGIN_VOXEL_NAMESPACE(MetaGraph)

class FPin;
class FNode;
class FGraph;

template<typename IteratorType, typename Type>
struct TDereferencerIterator
{
	IteratorType Iterator;

	TDereferencerIterator(IteratorType&& Iterator)
		: Iterator(Iterator)
	{
	}

	TDereferencerIterator& operator++()
	{
		++Iterator;
		return *this;
	}
	explicit operator bool() const
	{
		return bool(Iterator);
	}
	Type& operator*() const
	{
		return **Iterator;
	}

	friend bool operator!=(const TDereferencerIterator& Lhs, const TDereferencerIterator& Rhs)
	{
		return Lhs.Iterator != Rhs.Iterator;
	}
};

template<typename T>
class TPinsView
{
public:
	TPinsView() = default;
	TPinsView(const TArray<FPin*>& Pins)
		: Pins(Pins)
	{
	}

	T& operator[](int32 Index)
	{
		return *Pins[Index];
	}
	const T& operator[](int32 Index) const
	{
		return *Pins[Index];
	}

	int32 Num() const
	{
		return Pins.Num();
	}

	const TArray<FPin*>& Array() const
	{
		return Pins;
	}

	using FIterator = TDereferencerIterator<TArray<FPin*>::RangedForConstIteratorType, T>;

	FIterator begin() const { return FIterator{ Pins.begin() }; }
	FIterator end() const { return FIterator{ Pins.end() }; }

private:
	const TArray<FPin*>& Pins;
};

template<typename T>
class TNodesView
{
public:
	TNodesView(const TSet<FNode*>& Nodes)
		: Nodes(Nodes)
	{
	}

	using FIterator = TDereferencerIterator<TSet<FNode*>::TRangedForConstIterator, T>;

	FIterator begin() const { return FIterator{ Nodes.begin() }; }
	FIterator end() const { return FIterator{ Nodes.end() }; }

private:
	const TSet<FNode*>& Nodes;
};

template<typename T>
class TNodesCopy
{
public:
	TNodesCopy(const TSet<FNode*>& Nodes)
		: Nodes(Nodes)
	{
	}

	using FIterator = TDereferencerIterator<TSet<FNode*>::TRangedForConstIterator, T>;

	FIterator begin() const { return FIterator{ Nodes.begin() }; }
	FIterator end() const { return FIterator{ Nodes.end() }; }

private:
	TSet<FNode*> Nodes;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enum class EPinDirection : uint8
{
	Input,
	Output
};

class VOXELMETAGRAPH_API FPin
{
public:
	const FName Name;
	const FVoxelPinType Type;
	const EPinDirection Direction;
	FNode& Node;

	UE_NONCOPYABLE(FPin);
	
	const FVoxelPinValue& GetDefaultValue() const
	{
		check(Direction == EPinDirection::Input);
		return DefaultValue;
	}
	void SetDefaultValue(const FVoxelPinValue& NewDefaultValue)
	{
		check(Direction == EPinDirection::Input);
		ensure(NewDefaultValue.GetType().IsDerivedFrom(Type));
		DefaultValue = NewDefaultValue;
	}

private:
	FVoxelPinValue DefaultValue;

public:
	TPinsView<FPin> GetLinkedTo() const
	{
		return LinkedTo;
	}
	
	void MakeLinkTo(FPin& Other)
	{
		ensure(Direction == EPinDirection::Input || Type.IsDerivedFrom(Other.Type));
		ensure(Direction == EPinDirection::Output || Other.Type.IsDerivedFrom(Type));
		ensure(Direction != Other.Direction);
		ensure(!LinkedTo.Contains(&Other));
		LinkedTo.Add(&Other);
		Other.LinkedTo.Add(this);
	}
	bool IsLinkedTo(FPin& Other) const
	{
		ensure(LinkedTo.Contains(&Other) == Other.LinkedTo.Contains(this));
		return LinkedTo.Contains(&Other);
	}
	void TryMakeLinkTo(FPin& Other)
	{
		if (!IsLinkedTo(Other))
		{
			MakeLinkTo(Other);
		}
	}

	void BreakLinkTo(FPin& Other)
	{
		ensure(LinkedTo.Remove(&Other));
		ensure(Other.LinkedTo.Remove(this));
	}
	void BreakAllLinks()
	{
		for (FPin* Other : LinkedTo)
		{
			ensure(Other->LinkedTo.Remove(this));
		}
		LinkedTo.Reset();
	}

	void CopyInputPinTo(FPin& Target) const
	{
		ensure(Type == Target.Type);
		ensure(Direction == Target.Direction);
		ensure(Direction == EPinDirection::Input);

		ensure(LinkedTo.Num() <= 1);
		for (FPin* Other : LinkedTo)
		{
			Other->MakeLinkTo(Target);
		}

		Target.SetDefaultValue(GetDefaultValue());
	}
	void CopyOutputPinTo(FPin& Target) const
	{
		ensure(Type == Target.Type);
		ensure(Direction == Target.Direction);
		ensure(Direction == EPinDirection::Output);

		for (FPin* Other : LinkedTo)
		{
			Other->MakeLinkTo(Target);
		}
	}

	void Check(const FGraph& Graph) const;

private:
	TArray<FPin*> LinkedTo;

	FPin(
		FName Name,
		const FVoxelPinType& Type,
		const FVoxelPinValue& DefaultValue,
		EPinDirection Direction,
		FNode& Node)
		: Name(Name)
		, Type(Type)
		, Direction(Direction)
		, Node(Node)
		, DefaultValue(DefaultValue)
	{
		ensure(Type.IsValid());
	}

	friend class FGraph;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct FSourceNode
{
	// Graph.Macro.Node
	TArray<TWeakObjectPtr<UEdGraphNode>> GraphNodes;
	TArray<FSourceNode> ChildNodes;

	template<typename LambdaType>
	void ForeachGraphNode(LambdaType Lambda, float Strength = 1.f) const
	{
		for (const TWeakObjectPtr<UEdGraphNode>& GraphNode : GraphNodes)
		{
			Lambda(GraphNode, Strength);
		}

		for (const FSourceNode& ChildNode : ChildNodes)
		{
			ChildNode.ForeachGraphNode(Lambda, Strength / ChildNodes.Num());
		}
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enum class ENodeType : uint8
{
	Struct,
	Macro,
	Parameter,
	MacroInput,
	MacroOutput,
	Passthrough
};

class VOXELMETAGRAPH_API FNode
{
public:
	const ENodeType Type;
	FSourceNode Source;

public:
	struct FMetaGraph
	{
		const UVoxelMetaGraph* Graph = nullptr;
	};
	struct FParameter
	{
		FGuid Guid;
		FName Name;
		FVoxelPinType Type;
	};
	struct FMacroInputOutput
	{
		FGuid Guid;
		FName Name;
		FVoxelPinType Type;
	};

	TVoxelInstancedStruct<FVoxelNode>& Struct()
	{
		check(Type == ENodeType::Struct);
		return ImplStruct;
	}
	FMetaGraph& Macro()
	{
		check(Type == ENodeType::Macro);
		return ImplMacro;
	}
	FParameter& Parameter()
	{
		check(Type == ENodeType::Parameter);
		return ImplParameter;
	}
	FMacroInputOutput& MacroInput()
	{
		check(Type == ENodeType::MacroInput);
		return ImplMacroInput;
	}
	FMacroInputOutput& MacroOutput()
	{
		check(Type == ENodeType::MacroOutput);
		return ImplMacroOutput;
	}

	const TVoxelInstancedStruct<FVoxelNode>& Struct() const
	{
		return VOXEL_CONST_CAST(this)->Struct();
	}
	const FMetaGraph& Macro() const
	{
		return VOXEL_CONST_CAST(this)->Macro();
	}
	const FParameter& Parameter() const
	{
		return VOXEL_CONST_CAST(this)->Parameter();
	}
	const FMacroInputOutput& MacroInput() const
	{
		return VOXEL_CONST_CAST(this)->MacroInput();
	}
	const FMacroInputOutput& MacroOutput() const
	{
		return VOXEL_CONST_CAST(this)->MacroOutput();
	}

	void CopyFrom(const FNode& Src)
	{
		ensure(Type == Src.Type);

		ImplStruct = Src.ImplStruct;
		ImplMacro = Src.ImplMacro;
		ImplParameter = Src.ImplParameter;
		ImplMacroInput = Src.ImplMacroInput;
		ImplMacroOutput = Src.ImplMacroOutput;
	}

private:
	TVoxelInstancedStruct<FVoxelNode> ImplStruct;
	FMetaGraph ImplMacro;
	FParameter ImplParameter;
	FMacroInputOutput ImplMacroInput;
	FMacroInputOutput ImplMacroOutput;

public:
	FPin& GetInputPin(int32 Index) { return *InputPins[Index]; }
	FPin& GetOutputPin(int32 Index) { return *OutputPins[Index]; }

	const FPin& GetInputPin(int32 Index) const { return *InputPins[Index]; }
	const FPin& GetOutputPin(int32 Index) const { return *OutputPins[Index]; }

public:
	TPinsView<FPin> GetPins() { return Pins; }
	TPinsView<const FPin> GetPins() const { return Pins; }

	TPinsView<FPin> GetInputPins() { return InputPins; }
	TPinsView<FPin> GetOutputPins() { return OutputPins; }

	TPinsView<const FPin> GetInputPins() const { return InputPins; }
	TPinsView<const FPin> GetOutputPins() const { return OutputPins; }

public:
	FPin* FindInput(FName Name) { return InputPinsMap.FindRef(Name); }
	FPin* FindOutput(FName Name) { return OutputPinsMap.FindRef(Name); }

	FPin& FindInputChecked(FName Name) { return *InputPinsMap.FindChecked(Name); }
	FPin& FindOutputChecked(FName Name) { return *OutputPinsMap.FindChecked(Name); }

	const FPin* FindInput(FName Name) const { return InputPinsMap.FindRef(Name); }
	const FPin* FindOutput(FName Name) const { return OutputPinsMap.FindRef(Name); }

	const FPin& FindInputChecked(FName Name) const { return *InputPinsMap.FindChecked(Name); }
	const FPin& FindOutputChecked(FName Name) const { return *OutputPinsMap.FindChecked(Name); }

public:
	FPin& NewInputPin(FName Name, const FVoxelPinType& PinType, const FVoxelPinValue& DefaultValue = {});
	FPin& NewOutputPin(FName Name, const FVoxelPinType& PinType);

	void Check();
	void BreakAllLinks();
	void RemovePin(FPin* Pin);

	const FPin* FindPropertyPin(const FProperty& Property) const;
	const FPin& FindPropertyPinChecked(const FProperty& Property) const;

private:
	FNode(ENodeType Type, const FSourceNode& Source, FGraph& Graph)
		: Type(Type)
		, Source(Source)
		, Graph(Graph)
	{
		ensure(Source.GraphNodes.Num() > 0 || Source.ChildNodes.Num() > 0);
	}
	UE_NONCOPYABLE(FNode);

	FGraph& Graph;
	
	TArray<FPin*> Pins;
	TArray<FPin*> InputPins;
	TArray<FPin*> OutputPins;

	TMap<FName, FPin*> InputPinsMap;
	TMap<FName, FPin*> OutputPinsMap;

	friend class FGraph;
	friend FVoxelTemplateNodeUtilities;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class VOXELMETAGRAPH_API FGraph
{
public:
	FGraph() = default;
	UE_NONCOPYABLE(FGraph);
	
	TNodesView<FNode> GetNodes()
	{
		return Nodes;
	}
	TNodesView<const FNode> GetNodes() const
	{
		return Nodes;
	}
	
	TNodesCopy<FNode> GetNodesCopy()
	{
		return Nodes;
	}
	TNodesCopy<const FNode> GetNodesCopy() const
	{
		return Nodes;
	}
	
	TArray<FNode*> GetNodesArray()
	{
		return Nodes.Array();
	}
	TArray<const FNode*> GetNodesArray() const
	{
		return ReinterpretCastArray<const FNode*>(Nodes.Array());
	}

	FNode& NewNode(ENodeType Type, const FSourceNode& Source);

	template<typename PredicateType>
	void RemoveNodes(PredicateType Predicate)
	{
		for (auto It = Nodes.CreateIterator(); It; ++It)
		{
			FNode& Node = **It;
			if (!Predicate(Node))
			{
				continue;
			}

			Node.BreakAllLinks();
			It.RemoveCurrent();
		}
	}
	void RemoveNode(FNode& Node)
	{
		Node.BreakAllLinks();
		ensure(Nodes.Remove(&Node));
	}

	void Check();

	void CloneTo(
		FGraph& TargetGraph,
		TMap<const FNode*, FNode*>* OldToNewNodesPtr = nullptr,
		TMap<const FPin*, FPin*>* OldToNewPinsPtr = nullptr) const;

	TSharedRef<FGraph> Clone(
		TMap<const FNode*, FNode*>* OldToNewNodesPtr = nullptr,
		TMap<const FPin*, FPin*>* OldToNewPinsPtr = nullptr) const;

private:
	TSet<FNode*> Nodes;

	TArray<TUniquePtr<FPin>> PinRefs;
	TArray<TUniquePtr<FNode>> NodeRefs;

	FPin& NewPin(
		FName Name,
		const FVoxelPinType& Type,
		const FVoxelPinValue& DefaultValue,
		EPinDirection Direction,
		FNode& Node);

	friend class FPin;
	friend class FNode;
};

END_VOXEL_NAMESPACE(MetaGraph)