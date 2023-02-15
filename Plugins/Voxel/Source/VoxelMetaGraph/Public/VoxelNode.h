// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNodeHelpers.h"
#include "VoxelNodeMessages.h"
#include "VoxelNodeDefinition.h"
#include "VoxelRuntime/VoxelSubsystem.h"
#include "VoxelNode.generated.h"

class IVoxelSubsystem;
class UVoxelMetaGraph;
class FVoxelNodeDefinition;
struct FVoxelMetaGraphCompiledNode;

VOXEL_FWD_NAMESPACE_CLASS(FVoxelMetaGraphNodeCustomization, MetaGraph, FVoxelNodeCustomization);
VOXEL_FWD_NAMESPACE_CLASS(FVoxelMetaGraphNodePinArrayCustomization, MetaGraph, FVoxelNodePinArrayCustomization);


enum class EVoxelPinFlags : uint32
{
	None           = 0,
	MathPin        = 1 << 0,
	ArrayPin       = 1 << 1,
};
ENUM_CLASS_FLAGS(EVoxelPinFlags);

struct FVoxelPinMetadata
{
	FString DisplayName;
	FString Tooltip;
	FString Category;
	FString CategoryTooltip;
	FName Tag;
	bool bDisplayLast = false;
	bool bPropertyBind = false;
};

struct VOXELMETAGRAPH_API FVoxelPin
{
public:
	const FName Name;
	const bool bIsInput;
	const float SortOrder;
	const FName ArrayOwner;
	const EVoxelPinFlags Flags;
	const FVoxelPinType BaseType;
	const FVoxelPinValue DefaultValue;
	const FVoxelPinMetadata Metadata;

	bool IsPromotable() const
	{
		return BaseType.IsWildcard();
	}

	void SetType(const FVoxelPinType& NewType)
	{
		ensure(IsPromotable());
		ChildType = NewType;
	}
	const FVoxelPinType& GetType() const
	{
		ensure(BaseType == ChildType || IsPromotable());
		return ChildType;
	}

private:
	FVoxelPinType ChildType;

	FVoxelPin(
		const FName Name,
		const bool bIsInput,
		const float SortOrder,
		const FName ArrayOwner,
		const EVoxelPinFlags Flags,
		const FVoxelPinType& BaseType,
		const FVoxelPinType& ChildType,
		const FVoxelPinValue& DefaultValue,
		const FVoxelPinMetadata& Metadata)
		: Name(Name)
		, bIsInput(bIsInput)
		, SortOrder(SortOrder)
		, ArrayOwner(ArrayOwner)
		, Flags(Flags)
		, BaseType(BaseType)
		, DefaultValue(DefaultValue)
		, Metadata(Metadata)
		, ChildType(ChildType)
	{
		ensure(BaseType.IsValid());
		ensure(ChildType.IsValid());
	}

	friend struct FVoxelNode;
};

struct FVoxelPinRef
{
public:
	using Type = void;

	FVoxelPinRef() = default;
	explicit FVoxelPinRef(const FName Name)
		: Name(Name)
	{
	}
	operator FName() const
	{
		return Name;
	}

private:
	FName Name;
};

struct FVoxelPinArrayRef
{
public:
	using Type = void;
	
	FVoxelPinArrayRef() = default;
	explicit FVoxelPinArrayRef(const FName Name)
		: Name(Name)
	{
	}
	operator FName() const
	{
		return Name;
	}

private:
	FName Name;
};

template<typename T>
struct TVoxelPinRef : FVoxelPinRef
{
	using Type = T;
	
	TVoxelPinRef() = default;
	explicit TVoxelPinRef(const FName Name)
		: FVoxelPinRef(Name)
	{
	}
};

template<typename T>
struct TVoxelPinArrayRef : FVoxelPinArrayRef
{
	using Type = T;
	
	TVoxelPinArrayRef() = default;
	explicit TVoxelPinArrayRef(const FName Name)
		: FVoxelPinArrayRef(Name)
	{
	}
};

class VOXELMETAGRAPH_API FVoxelNodeRuntime : public FVoxelNodeAliases
{
public:
	FVoxelNodeRuntime() = default;
	UE_NONCOPYABLE(FVoxelNodeRuntime);

	FVoxelFutureValue Get(const FVoxelPinRef& Pin, const FVoxelQuery& Query) const;
	TValue<FVoxelBufferView> GetBufferView(const FVoxelPinRef& Pin, const FVoxelQuery& Query) const;

	bool IsDefaultValue(const FVoxelPinRef& Pin) const;

public:
	template<typename T>
	TValue<T> Get(const FVoxelPinRef& Pin, const FVoxelQuery& Query) const
	{
		check(GetPinData(Pin).Type.IsDerivedFrom<T>());
		return TValue<T>(Get(Pin, Query));
	}
	template<typename T>
	TValue<TBufferView<T>> GetBufferView(const FVoxelPinRef& Pin, const FVoxelQuery& Query) const
	{
		check(GetPinData(Pin).Type.IsDerivedFrom<TVoxelBuffer<T>>());
		
		const TValue<TVoxelBuffer<T>> Buffer = this->Get<TVoxelBuffer<T>>(Pin, Query);

		return FVoxelTask::New<TBufferView<T>>(
			MakeShared<FVoxelTaskStat>(),
			STATIC_FNAME("MakeView"),
			EVoxelTaskThread::AnyThread,
			{ Buffer },
			[Buffer]
			{
				return Buffer.Get_CheckCompleted().MakeView();
			});
	}

public:
	template<typename T>
	TValue<T> Get(const TVoxelPinRef<T>& Pin, const FVoxelQuery& Query) const
	{
		return this->Get<T>(static_cast<const FVoxelPinRef&>(Pin), Query);
	}
	template<typename T, typename UniformType = typename T::UniformType, typename = typename TEnableIf<TIsDerivedFrom<T, FVoxelBuffer>::Value>::Type>
	TValue<TBufferView<UniformType>> GetBufferView(const TVoxelPinRef<T>& Pin, const FVoxelQuery& Query) const
	{
		const TValue<T> Buffer = this->Get(Pin, Query);

		return FVoxelTask::New<TBufferView<UniformType>>(
			MakeShared<FVoxelTaskStat>(),
			STATIC_FNAME("MakeView"),
			EVoxelTaskThread::AnyThread,
			{ Buffer },
			[Buffer]
			{
				return Buffer.Get_CheckCompleted().MakeView();
			});
	}

public:
	template<typename T>
	TArray<TValue<T>> Get(const FVoxelPinArrayRef& ArrayPin, const FVoxelQuery& Query) const
	{
		if (const TFunction<TArray<FVoxelFutureValue>(const FVoxelQuery&)>* Override = PinArrayOverrides.Find(ArrayPin))
		{
			return TArray<TValue<T>>((*Override)(Query));
		}

		TArray<TValue<T>> Array;
		for (const FName Pin : PinArrays[ArrayPin])
		{
			Array.Add(Get<T>(FVoxelPinRef(Pin), Query));
		}
		return Array;
	}
	template<typename T>
	TArray<TValue<TBufferView<T>>> GetBufferView(const FVoxelPinArrayRef& ArrayPin, const FVoxelQuery& Query) const
	{
		if (const TFunction<TArray<FVoxelFutureValue>(const FVoxelQuery&)>* Override = PinArrayOverrides.Find(ArrayPin))
		{
			TArray<TValue<TBufferView<T>>> Array;
			for (const FVoxelFutureValue& Value : (*Override)(Query))
			{
				Array.Add(FVoxelTask::New<TBufferView<T>>(
					MakeShared<FVoxelTaskStat>(),
					STATIC_FNAME("MakeView"),
					EVoxelTaskThread::AnyThread,
					{ Value },
					[Value]
					{
						return Value.Get_CheckCompleted<TVoxelBuffer<T>>().MakeView();
					}));
			}
			return Array;
		}

		TArray<TValue<TBufferView<T>>> Array;
		for (const FName Pin : PinArrays[ArrayPin])
		{
			Array.Add(GetBufferView<T>(FVoxelPinRef(Pin), Query));
		}
		return Array;
	}

public:
	template<typename T>
	TArray<TValue<T>> Get(const TVoxelPinArrayRef<T>& ArrayPin, const FVoxelQuery& Query) const
	{
		return Get<T>(FVoxelPinArrayRef(ArrayPin), Query);
	}
	template<typename T, typename UniformType = typename T::UniformType, typename = typename TEnableIf<TIsDerivedFrom<T, FVoxelBuffer>::Value>::Type>
	TArray<TValue<TBufferView<UniformType>>> GetBufferView(const TVoxelPinArrayRef<T>& ArrayPin, const FVoxelQuery& Query) const
	{
		return GetBufferView<UniformType>(FVoxelPinArrayRef(ArrayPin), Query);
	}

public:
	FVoxelRuntime& GetRuntime() const
	{
		return *WeakRuntime.Pin().ToSharedRef();
	}
	TSharedRef<IVoxelNodeOuter> GetOuter() const
	{
		return WeakOuter.Pin().ToSharedRef();
	}
	const Voxel::MetaGraph::FNode* GetSourceNode() const
	{
		return SourceNode;
	}
	FName GetStatName() const
	{
		return StatName;
	}

	template<typename T>
	FORCEINLINE T& GetSubsystem() const
	{
		return GetRuntime().GetSubsystem<T>();
	}

public:
	struct FComputeState
	{
		FVoxelPinType Type;
		const FVoxelNode* Node;
		FName StatName;
		TVoxelFunction<FVoxelFutureValue(const FVoxelQuery& Query)> Compute;
	};
	struct FPinData : TSharedFromThis<FPinData>
	{
		const FVoxelPinType Type;
		const bool bIsInput;
		const FName StatName;

		FVoxelSharedPinValue DefaultValue;
		TSharedPtr<const FPinData> OutputPinData;
		TSharedPtr<const FComputeState> ComputeState;

		TArray<const FVoxelNode*> Exec_LinkedTo;

		FPinData(const FVoxelPinType& Type, bool bIsInput, FName StatName)
			: Type(Type.WithoutTag())
			, bIsInput(bIsInput)
			, StatName(StatName)
		{
		}
	};
	FPinData& GetPinData(const FName PinName) const
	{
		return *PinDatas[PinName];
	}

private:
	const FVoxelNode* Node = nullptr;
	TWeakPtr<FVoxelRuntime> WeakRuntime;
	TWeakPtr<IVoxelNodeOuter> WeakOuter;
	TWeakObjectPtr<const UObject> WeakGraph;
	const Voxel::MetaGraph::FNode* SourceNode = nullptr;
	FName StatName;

	FVoxelCriticalSection CriticalSection;
	TSet<FString> Errors;

	TMap<FName, TSharedPtr<FPinData>> PinDatas;
	TMap<FName, TArray<FName>> PinArrays;

	TMap<FName, TFunction<FVoxelFutureValue(const FVoxelQuery&)>> PinOverrides;
	TMap<FName, TFunction<TArray<FVoxelFutureValue>(const FVoxelQuery&)>> PinArrayOverrides;

	friend FVoxelNode;
	friend class FVoxelNodeCaller;
	friend struct TVoxelMessageArgProcessor<FVoxelNode>;
};

USTRUCT()
struct FVoxelNodeSerializedArrayData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FName> PinNames;
};

USTRUCT()
struct FVoxelNodeSerializedData
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<FName, FVoxelPinType> PinTypes;
	
	UPROPERTY()
	TMap<FName, FVoxelNodeSerializedArrayData> ArrayDatas;

	UPROPERTY()
	TSet<FName> ExposedPins;

	UPROPERTY()
	TMap<FName, FVoxelPinValue> ExposedPinsValues;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

using FVoxelNodeComputePtr = FVoxelFutureValue (*) (const FVoxelNode& Node, const FVoxelQuery& Query);

struct VOXELMETAGRAPH_API FVoxelNodeComputePtrs
{
	FVoxelNodeComputePtr Cpu;
	FVoxelNodeComputePtr Gpu;

	FString DebugCpu;
	FString DebugGpu;

	static void InitializeCpu(UScriptStruct* Node, FName PinName, FVoxelNodeComputePtr Ptr, const FString& Debug)
	{
		Initialize(Node, PinName, Ptr, Debug, true, false);
	}
	static void InitializeGpu(UScriptStruct* Node, FName PinName, FVoxelNodeComputePtr Ptr, const FString& Debug)
	{
		Initialize(Node, PinName, Ptr, Debug, false, true);
	}
	static void InitializeGeneric(UScriptStruct* Node, FName PinName, FVoxelNodeComputePtr Ptr, const FString& Debug)
	{
		Initialize(Node, PinName, Ptr, Debug, true, true);
	}

	static void Initialize(
		const UScriptStruct* Node,
		FName PinName,
		FVoxelNodeComputePtr Ptr,
		const FString& Debug,
		bool bCpu,
		bool bGpu);
};

USTRUCT(meta = (Abstract))
struct VOXELMETAGRAPH_API FVoxelNode : public FVoxelNodeInterface
{
	GENERATED_BODY()
	DECLARE_VIRTUAL_STRUCT_PARENT(FVoxelNode, GENERATED_VOXEL_NODE_BODY)

public:
	FVoxelNode() = default;
	FVoxelNode(const FVoxelNode& Other) = delete;
	FVoxelNode& operator=(const FVoxelNode& Other);

	//~ Begin FVoxelNodeInterface Interface
	FORCEINLINE virtual const FVoxelNode& GetNode() const final override
	{
		return *this;
	}
	FORCEINLINE virtual TSharedRef<IVoxelNodeOuter> GetOuter() const final override
	{
		return NodeRuntime->GetOuter();
	}
	//~ End FVoxelNodeInterface Interface

public:
#if WITH_EDITOR
	virtual FString GetCategory() const;
	virtual FString GetDisplayName() const;
	virtual FString GetTooltip() const;
#endif

	virtual bool ShowPromotablePinsAsWildcards() const
	{
		return true;
	}
	virtual bool IsPureNode() const
	{
		return GetExecType() != EExecType::Runtime;
	}
	virtual bool HasSideEffects() const
	{
		return false;
	}

	enum class EExecType
	{
		Runtime,
		CodeGen,
		Any
	};
	virtual EExecType GetExecType() const
	{
		return EExecType::Runtime;
	}

	bool IsCodeGen() const
	{
		return GetExecType() != EExecType::Runtime;
	}

	virtual FString GenerateCode(bool bIsGpu) const
	{
		check(GetExecType() == EExecType::CodeGen || GetExecType() == EExecType::Any);
		ensure(false);
		return {};
	}

	virtual void Initialize() {}
	virtual void ReturnToPool();
	virtual TVoxelFunction<FVoxelFutureValue(const FVoxelQuery&)> Compile(FName PinName) const;

public:
	virtual void PreSerialize() override;
	virtual void PostSerialize() override;

public:
	using FDefinition = FVoxelNodeDefinition;
	virtual TSharedRef<IVoxelNodeDefinition> GetNodeDefinition();

	// Pin will always be a promotable pin
	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType);

	bool AreMathPinsBuffers() const;

	bool IsPinExposed(const FVoxelPin& Pin) const;
	bool IsPinHidden(const FVoxelPin& Pin) const;
	FVoxelPinValue GetPinDefaultValue(const FVoxelPin& Pin) const;
	void UpdatePropertyBoundDefaultValue(const FVoxelPin& Pin, const FVoxelPinValue& NewValue);

public:
	template<typename T>
	struct TPinIterator
	{
		TMap<FName, TSharedPtr<FVoxelPin>>::TRangedForConstIterator Iterator;

		TPinIterator(TMap<FName, TSharedPtr<FVoxelPin>>::TRangedForConstIterator&& Iterator)
			: Iterator(Iterator)
		{
		}

		TPinIterator& operator++()
		{
			++Iterator;
			return *this;
		}
		explicit operator bool() const
		{
			return bool(Iterator);
		}
		T& operator*() const
		{
			return *Iterator.Value();
		}

		friend bool operator!=(const TPinIterator& Lhs, const TPinIterator& Rhs)
		{
			return Lhs.Iterator != Rhs.Iterator;
		}
	};
	template<typename T>
	struct TPinView
	{
		const TMap<FName, TSharedPtr<FVoxelPin>>& Pins;

		TPinView() = default;
		TPinView(const TMap<FName, TSharedPtr<FVoxelPin>>& Pins)
			: Pins(Pins)
		{
		}

		TPinIterator<T> begin() const { return TPinIterator<T>{ Pins.begin() }; }
		TPinIterator<T> end() const { return TPinIterator<T>{ Pins.end() }; }
	};

	TPinView<FVoxelPin> GetPins()
	{
		FlushDeferredPins();
		return TPinView<FVoxelPin>(InternalPins);
	}
	TPinView<const FVoxelPin> GetPins() const
	{
		FlushDeferredPins();
		return TPinView<const FVoxelPin>(InternalPins);
	}

	const TMap<FName, TSharedPtr<FVoxelPin>>& GetPinsMap()
	{
		FlushDeferredPins();
		return InternalPins;
	}
	const TMap<FName, TSharedPtr<const FVoxelPin>>& GetPinsMap() const
	{
		FlushDeferredPins();
		return ReinterpretCastRef<TMap<FName, TSharedPtr<const FVoxelPin>>>(InternalPins);
	}

	TSharedPtr<FVoxelPin> FindPin(FName Name)
	{
		return GetPinsMap().FindRef(Name);
	}
	TSharedPtr<const FVoxelPin> FindPin(FName Name) const
	{
		return GetPinsMap().FindRef(Name);
	}

	FVoxelPin& GetPin(const FVoxelPinRef& Pin)
	{
		return *GetPinsMap().FindChecked(Pin);
	}
	const FVoxelPin& GetPin(const FVoxelPinRef& Pin) const
	{
		return *GetPinsMap().FindChecked(Pin);
	}

	FVoxelPin& GetUniqueInputPin();
	FVoxelPin& GetUniqueOutputPin();

	const FVoxelPin& GetUniqueInputPin() const
	{
		return VOXEL_CONST_CAST(this)->GetUniqueInputPin();
	}
	const FVoxelPin& GetUniqueOutputPin() const
	{
		return VOXEL_CONST_CAST(this)->GetUniqueOutputPin();
	}

public:
	FName AddPinToArray(FName ArrayName, FName PinName = {});
	FName InsertPinToArrayPosition(FName ArrayName, int32 Position);
	FName AddPinToArray(FVoxelPinRef ArrayName, FName PinName = {}) = delete;

	const TArray<FVoxelPinRef>& GetArrayPins(const FVoxelPinArrayRef& ArrayRef) const
	{
		FlushDeferredPins();
		return ReinterpretCastArray<FVoxelPinRef>(InternalPinArrays[ArrayRef]->Pins);
	}
	template<typename T>
	const TArray<TVoxelPinRef<T>>& GetArrayPins(const TVoxelPinArrayRef<T>& ArrayRef) const
	{
		return ReinterpretCastArray<TVoxelPinRef<T>>(GetArrayPins(static_cast<const FVoxelPinArrayRef&>(ArrayRef)));
	}

private:
	void FixupArrayNames(FName ArrayName);

protected:
	FName CreatePin(
		FVoxelPinType Type,
		bool bIsInput,
		FName Name,
		const FVoxelPinValue& DefaultValue,
		const FVoxelPinMetadata& Metadata = {},
		EVoxelPinFlags Flags = EVoxelPinFlags::None,
		int32 MinArrayNum = 0);

	void RemovePin(FName Name);

protected:
	FVoxelPinRef CreateInputPin(
		const FVoxelPinType& Type,
		const FName Name,
		const FVoxelPinValue& DefaultValue,
		const FVoxelPinMetadata& Metadata = {},
		const EVoxelPinFlags Flags = EVoxelPinFlags::None)
	{
		return FVoxelPinRef(CreatePin(
			Type,
			true,
			Name,
			DefaultValue,
			Metadata,
			Flags));
	}
	FVoxelPinRef CreateOutputPin(
		const FVoxelPinType& Type,
		const FName Name,
		const FVoxelPinMetadata& Metadata = {},
		const EVoxelPinFlags Flags = EVoxelPinFlags::None)
	{
		return FVoxelPinRef(CreatePin(
			Type,
			false,
			Name,
			{},
			Metadata,
			Flags));
	}

protected:
	template<typename Type>
	TVoxelPinRef<Type> CreateInputPin(
		const FName Name,
		const FVoxelPinValue& DefaultValue,
		const FVoxelPinMetadata& Metadata,
		const EVoxelPinFlags Flags = EVoxelPinFlags::None)
	{
		return TVoxelPinRef<Type>(this->CreateInputPin(
			FVoxelPinType::Make<Type>(),
			Name,
			DefaultValue,
			Metadata,
			Flags));
	}
	template<typename Type>
	TVoxelPinRef<Type> CreateOutputPin(
		const FName Name,
		const FVoxelPinMetadata& Metadata,
		const EVoxelPinFlags Flags = EVoxelPinFlags::None)
	{
		return TVoxelPinRef<Type>(this->CreateOutputPin(
			FVoxelPinType::Make<Type>(),
			Name,
			Metadata,
			Flags));
	}
	
protected:
	FVoxelPinArrayRef CreateInputPinArray(
		const FVoxelPinType& Type,
		const FName Name,
		const FVoxelPinValue& DefaultValue,
		const FVoxelPinMetadata& Metadata,
		const int32 MinNum,
		const EVoxelPinFlags Flags = EVoxelPinFlags::None)
	{
		return FVoxelPinArrayRef(CreatePin(
			Type,
			true,
			Name,
			DefaultValue,
			Metadata,
			Flags | EVoxelPinFlags::ArrayPin,
			MinNum));
	}
	template<typename Type>
	TVoxelPinArrayRef<Type> CreateInputPinArray(
		const FName Name, 
		const FVoxelPinValue& DefaultValue, 
		const FVoxelPinMetadata& Metadata,
		const int32 MinNum, 
		const EVoxelPinFlags Flags = EVoxelPinFlags::None)
	{
		return TVoxelPinArrayRef<Type>(this->CreateInputPinArray(FVoxelPinType::Make<Type>(), Name, DefaultValue, Metadata, MinNum, Flags));
	}

protected:
	struct FDeferredPin
	{
		FName ArrayOwner;
		int32 MinArrayNum = 0;

		FName Name;
		bool bIsInput = false;
		float SortOrder = 0.f;
		EVoxelPinFlags Flags = {};
		FVoxelPinType BaseType;
		FVoxelPinType ChildType;
		FVoxelPinValue DefaultValue;
		FVoxelPinMetadata Metadata;

		bool IsArrayElement() const
		{
			return !ArrayOwner.IsNone();
		}

		bool IsArrayDeclaration() const
		{
			return EnumHasAllFlags(Flags, EVoxelPinFlags::ArrayPin);
		}
	};
	struct FPinArray
	{
		const FDeferredPin PinTemplate;
		TArray<FName> Pins;

		explicit FPinArray(const FDeferredPin& PinTemplate)
			: PinTemplate(PinTemplate)
		{
		}
	};

private:
	int32 SortOrderCounter = 1;

	bool bIsDeferringPins = true;
	TArray<FDeferredPin> DeferredPins;

	TMap<FName, FDeferredPin> InternalPinBackups;
	TArray<FName> InternalPinsOrder;
	int32 DisplayLastPins = 0;
	TMap<FName, TSharedPtr<FVoxelPin>> InternalPins;
	TMap<FName, TSharedPtr<FPinArray>> InternalPinArrays;

#if WITH_EDITOR
	TMap<FName, FString> MappedCategoryTooltips;
#endif

	UPROPERTY(EditAnywhere, Category = "Voxel", Transient)
	TMap<FName, FVoxelPinValue> ExposedPinsValues;

	TSet<FName> ExposedPins;
	FSimpleMulticastDelegate OnExposedPinsUpdated;

	FORCEINLINE void FlushDeferredPins() const
	{
		if (bIsDeferringPins)
		{
			VOXEL_CONST_CAST(this)->FlushDeferredPinsImpl();
		}
	}
	void FlushDeferredPinsImpl();
	void RegisterPin(FDeferredPin Pin, bool bApplyMinNum = true);
	void SortPins();
	void SortArrayPins(FName PinArrayName);

public:
#if WITH_EDITOR
	virtual void GetExternalPinsData(TArray<FName>& OutPinNames, TArray<FName>& OutCategoryNames) const;
#endif

protected:
	FString GetExternalPinTooltip(FName PinName) const;
	FString GetExternalCategoryTooltip(FName CategoryName) const;

private:
	UPROPERTY()
	FVoxelNodeSerializedData SerializedDataProperty;

	FVoxelNodeSerializedData GetSerializedData() const;
	void LoadSerializedData(const FVoxelNodeSerializedData& SerializedData);
	
public:
	void InitializeNodeRuntime(
		FVoxelRuntime* Runtime,
		TWeakPtr<IVoxelNodeOuter> WeakOuter,
		TWeakObjectPtr<const UVoxelMetaGraph> WeakGraph,
		const Voxel::MetaGraph::FNode* SourceNode);

	const FVoxelNodeRuntime& GetNodeRuntime() const
	{
		return *NodeRuntime;
	}

private:
	TSharedPtr<FVoxelNodeRuntime> NodeRuntime;

protected:
	using FReturnToPoolFunc = void (FVoxelNode::*)();

	void AddReturnToPoolFunc(FReturnToPoolFunc ReturnToPool)
	{
		ReturnToPoolFuncs.Add(ReturnToPool);
	}

private:
	TArray<FReturnToPoolFunc> ReturnToPoolFuncs;

	friend struct TVoxelMessageArgProcessor<FVoxelNode>;
	friend class FVoxelMetaGraphStructNodeDefinition;
	friend FVoxelMetaGraphNodeCustomization;
	friend FVoxelMetaGraphNodePinArrayCustomization;
	friend class FVoxelNodeDefinition;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define GENERATED_VOXEL_NODE_DEFINITION_BODY(NodeType) \
	NodeType& Node; \
	explicit FDefinition(NodeType& Node) \
		: Super::FDefinition(Node) \
		, Node(Node) \
	{}

class VOXELMETAGRAPH_API FVoxelNodeDefinition : public IVoxelNodeDefinition
{
public:
	FVoxelNode& Node;

	explicit FVoxelNodeDefinition(FVoxelNode& Node)
		: Node(Node)
	{
	}

	virtual TSharedPtr<const FNode> GetInputs() const override;
	virtual TSharedPtr<const FNode> GetOutputs() const override;
	TSharedPtr<const FNode> GetPins(const bool bInput) const;

	virtual FString GetAddPinLabel() const override;
	virtual FString GetAddPinTooltip() const override;
	virtual FString GetRemovePinTooltip() const override;

	virtual bool CanAddToCategory(FName Category) const override;
	virtual void AddToCategory(FName Category) override;

	virtual bool CanRemoveFromCategory(FName Category) const override;
	virtual void RemoveFromCategory(FName Category) override;

	virtual bool CanRemoveSelectedPin(FName PinName) const override;
	virtual void RemoveSelectedPin(FName PinName) override;

	virtual void InsertPinBefore(FName PinName) override;
	virtual void DuplicatePin(FName PinName) override;

#if WITH_EDITOR
	virtual FString GetPinTooltip(FName PinName) const override;
	virtual FString GetCategoryTooltip(FName CategoryName) const override;
#endif
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define GENERATED_VOXEL_NODE_BODY() \
	GENERATED_VIRTUAL_STRUCT_BODY_IMPL(FVoxelNode) \
	virtual TSharedRef<IVoxelNodeDefinition> GetNodeDefinition() override { return MakeShared<FDefinition>(*this); }

namespace FVoxelPinMetadataBuilder
{
	struct SeedPin {};
	struct DensityPin {};
	struct DisplayLast {};

	template<typename T, typename = typename TEnableIf<TIsEnum<T>::Value && sizeof(T) == 1>::Type>
	struct EnumPin {};

	namespace Internal
	{
		struct None {};

		template<typename T>
		struct TStringParam
		{
			FString Value;

			explicit TStringParam(const FString& Value)
				: Value(Value)
			{
			}

			T operator()() const { return ReinterpretCastRef<const T&>(*this); }
		};
	}

	struct DisplayName : Internal::TStringParam<DisplayName> { using TStringParam<DisplayName>::TStringParam; };
	struct Tooltip : Internal::TStringParam<Tooltip> { using TStringParam<Tooltip>::TStringParam; };
	struct Category : Internal::TStringParam<Category> { using TStringParam<Category>::TStringParam; };
	struct CategoryTooltip : Internal::TStringParam<CategoryTooltip> { using TStringParam<CategoryTooltip>::TStringParam; };

	struct PropertyBind {};

	template<typename Type>
	struct TBuilder
	{
		static void MakeImpl(FVoxelPinMetadata&, Internal::None) {}

		static void MakeImpl(FVoxelPinMetadata& Metadata, SeedPin)
		{
			checkStatic(TIsSame<Type, int32>::Value || TIsSame<Type, FVoxelInt32Buffer>::Value || TIsSame<Type, void>::Value);

			ensure(Metadata.Tag.IsNone());
			Metadata.Tag = "Seed";
		}
		static void MakeImpl(FVoxelPinMetadata& Metadata, DensityPin)
		{
			checkStatic(TIsSame<Type, float>::Value || TIsSame<Type, FVoxelFloatBuffer>::Value || TIsSame<Type, void>::Value);

			ensure(Metadata.Tag.IsNone());
			Metadata.Tag = "Density";
		}
		template<typename T>
		static void MakeImpl(FVoxelPinMetadata& Metadata, EnumPin<T>)
		{
			checkStatic(TIsSame<Type, uint8>::Value || TIsSame<Type, FVoxelByteBuffer>::Value || TIsSame<Type, void>::Value);

			ensure(Metadata.Tag.IsNone());
			Metadata.Tag = StaticEnum<T>()->GetFName();
		}

		static void MakeImpl(FVoxelPinMetadata& Metadata, DisplayName Value)
		{
			ensure(Metadata.DisplayName.IsEmpty());
			Metadata.DisplayName = Value.Value;
		}
		static void MakeImpl(FVoxelPinMetadata& Metadata, Tooltip Value)
		{
			ensure(Metadata.Category.IsEmpty());
			Metadata.Tooltip = Value.Value;
		}
		static void MakeImpl(FVoxelPinMetadata& Metadata, Category Value)
		{
			ensure(Metadata.Category.IsEmpty());
			Metadata.Category = Value.Value;
		}
		static void MakeImpl(FVoxelPinMetadata& Metadata, CategoryTooltip Value)
		{
			ensure(Metadata.CategoryTooltip.IsEmpty());
			Metadata.CategoryTooltip = Value.Value;
		}

		static void MakeImpl(FVoxelPinMetadata& Metadata, DisplayLast)
		{
			ensure(!Metadata.bDisplayLast);
			Metadata.bDisplayLast = true;
		}

		static void MakeImpl(FVoxelPinMetadata& Metadata, PropertyBind)
		{
			ensure(!Metadata.bPropertyBind);
			Metadata.bPropertyBind = true;
		}

		template<typename T>
		using TIsValid = TIsSame<decltype(TBuilder<Type>::MakeImpl(DeclVal<FVoxelPinMetadata&>(), DeclVal<T>())), void>;

		template<typename... ArgTypes, typename = typename TEnableIf<TAnd<TIsValid<ArgTypes>...>::Value>::Type>
		static FVoxelPinMetadata Make(ArgTypes... Args)
		{
			FVoxelPinMetadata Metadata;
			VOXEL_FOLD_EXPRESSION(TBuilder<Type>::MakeImpl(Metadata, Args));
			return Metadata;
		}
	};
}

#define INTERNAL_VOXEL_PIN_METADATA_FOREACH(X) FVoxelPinMetadataBuilder::X()
#define VOXEL_PIN_METADATA_IMPL(Type, ...) FVoxelPinMetadataBuilder::TBuilder<Type>::Make(VOXEL_FOREACH_COMMA(INTERNAL_VOXEL_PIN_METADATA_FOREACH, Internal::None, ##__VA_ARGS__))
#define VOXEL_PIN_METADATA(...) VOXEL_PIN_METADATA_IMPL(void, ##__VA_ARGS__)

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define INTERNAL_DECLARE_VOXEL_PIN(Name) INTELLISENSE_ONLY(void VOXEL_APPEND_LINE(__DummyFunction)(FVoxelPinRef Name);)
#define INTERNAL_VOXEL_PIN_DEFAULT(Type, Default) FVoxelNodeDefaultValueHelper::Get(static_cast<Type*>(nullptr), Default)

#define VOXEL_INPUT_PIN(Type, Name, Default, ...) \
	INTERNAL_DECLARE_VOXEL_PIN(Name); \
	TVoxelPinRef<Type> Name ## Pin = ( \
		[] { checkStatic(!TIsEnum<Type>::Value); }, \
		CreateInputPin<Type>(STATIC_FNAME(#Name), INTERNAL_VOXEL_PIN_DEFAULT(Type, Default), VOXEL_PIN_METADATA_IMPL(Type, ##__VA_ARGS__)));

#define VOXEL_INPUT_PIN_ARRAY(Type, Name, Default, MinNum, ...) \
	INTERNAL_DECLARE_VOXEL_PIN(Name); \
	TVoxelPinArrayRef<Type> Name ## Pins = ( \
		[] { checkStatic(!TIsEnum<Type>::Value); }, \
		CreateInputPinArray<Type>(STATIC_FNAME(#Name), INTERNAL_VOXEL_PIN_DEFAULT(Type, Default), VOXEL_PIN_METADATA_IMPL(Type, ##__VA_ARGS__), MinNum));

#define VOXEL_OUTPUT_PIN(Type, Name, ...) \
	INTERNAL_DECLARE_VOXEL_PIN(Name); \
	TVoxelPinRef<Type> Name ## Pin = ( \
		[] { checkStatic(!TIsEnum<Type>::Value); }, \
		CreateOutputPin<Type>(STATIC_FNAME(#Name), VOXEL_PIN_METADATA_IMPL(Type, ##__VA_ARGS__)));

///////////////////////////////////////////////////////////////////////////////
//////////////// Generic pins are wild card pins with Promote /////////////////
///////////////////////////////////////////////////////////////////////////////

#define VOXEL_GENERIC_INPUT_PIN(Name, ...) \
	INTERNAL_DECLARE_VOXEL_PIN(Name); \
	FVoxelPinRef Name ## Pin = CreateInputPin(FVoxelPinType::MakeWildcard(), STATIC_FNAME(#Name), {}, VOXEL_PIN_METADATA_IMPL(void, ##__VA_ARGS__));

#define VOXEL_GENERIC_INPUT_PIN_ARRAY(Name, MinNum, ...) \
	INTERNAL_DECLARE_VOXEL_PIN(Name); \
	FVoxelPinArrayRef Name ## Pins = CreateInputPinArray(FVoxelPinType::MakeWildcard(), STATIC_FNAME(#Name), {}, VOXEL_PIN_METADATA_IMPL(void, ##__VA_ARGS__), MinNum);

#define VOXEL_GENERIC_OUTPUT_PIN(Name, ...) \
	INTERNAL_DECLARE_VOXEL_PIN(Name); \
	FVoxelPinRef Name ## Pin = CreateOutputPin(FVoxelPinType::MakeWildcard(), STATIC_FNAME(#Name), VOXEL_PIN_METADATA_IMPL(void, ##__VA_ARGS__));

///////////////////////////////////////////////////////////////////////////////
///////////////////// Math pins can be scalar or array ////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define VOXEL_MATH_INPUT_PIN(Type, Name, Default, ...) \
	INTERNAL_DECLARE_VOXEL_PIN(Name); \
	FVoxelPinRef Name ## Pin = ( \
		[] { checkStatic(!TIsEnum<Type>::Value); }, \
		[] { checkStatic(!TIsDerivedFrom<Type, FVoxelBuffer>::Value); }, \
		CreateInputPin(FVoxelPinType::Make<TVoxelBuffer<Type>>(), STATIC_FNAME(#Name), INTERNAL_VOXEL_PIN_DEFAULT(Type, Default), VOXEL_PIN_METADATA_IMPL(Type, ##__VA_ARGS__), EVoxelPinFlags::MathPin));

#define VOXEL_MATH_INPUT_PIN_ARRAY(Type, Name, Default, MinNum, ...) \
	INTERNAL_DECLARE_VOXEL_PIN(Name); \
	FVoxelPinArrayRef Name ## Pins = ( \
		[] { checkStatic(!TIsEnum<Type>::Value); }, \
		[] { checkStatic(!TIsDerivedFrom<Type, FVoxelBuffer>::Value); }, \
		CreateInputPinArray(FVoxelPinType::Make<TVoxelBuffer<Type>>(), STATIC_FNAME(#Name), INTERNAL_VOXEL_PIN_DEFAULT(Type, Default), VOXEL_PIN_METADATA_IMPL(Type, ##__VA_ARGS__), MinNum, EVoxelPinFlags::MathPin));

#define VOXEL_MATH_OUTPUT_PIN(Type, Name, ...) \
	INTERNAL_DECLARE_VOXEL_PIN(Name); \
	FVoxelPinRef Name ## Pin = ( \
		[] { checkStatic(!TIsEnum<Type>::Value); }, \
		[] { checkStatic(!TIsDerivedFrom<Type, FVoxelBuffer>::Value); }, \
		CreateOutputPin(FVoxelPinType::Make<TVoxelBuffer<Type>>(), STATIC_FNAME(#Name), VOXEL_PIN_METADATA_IMPL(Type, ##__VA_ARGS__), EVoxelPinFlags::MathPin));

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define VOXEL_PIN_NAME(NodeType, PinName) ([]() -> const auto& { static const auto StaticName = NodeType().PinName; return StaticName; }())

#define VOXEL_CALL_PARAM(Type, Name) \
	Type Name; \
	void Internal_ReturnToPool_ ## Name() \
	{ \
		Name = {}; \
	} \
	VOXEL_ON_CONSTRUCT() \
	{ \
		using ThisType = VOXEL_THIS_TYPE; \
		AddReturnToPoolFunc(static_cast<FReturnToPoolFunc>(&ThisType::Internal_ReturnToPool_ ## Name)); \
	};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Abstract))
struct VOXELMETAGRAPH_API FVoxelNode_CodeGen : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	virtual EExecType GetExecType() const final override
	{
		return EExecType::CodeGen;
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class VOXELMETAGRAPH_API FVoxelNodeCaller
{
public:
	class FBindings
	{
	public:
		template<typename T>
		TFunction<TVoxelFutureValue<T>(const FVoxelQuery&)>& Bind(const TVoxelPinRef<T> Name) const
		{
			return ::ReinterpretCastRef<TFunction<TVoxelFutureValue<T>(const FVoxelQuery&)>>(NodeRuntime.PinOverrides[Name]);
		}
		template<typename T>
		TFunction<TArray<TVoxelFutureValue<T>>(const FVoxelQuery&)>& Bind(const TVoxelPinArrayRef<T> Name) const
		{
			return ::ReinterpretCastRef<TFunction<TArray<TVoxelFutureValue<T>>(const FVoxelQuery&)>>(NodeRuntime.PinArrayOverrides[Name]);
		}

	private:
		FVoxelNodeRuntime& NodeRuntime;

		explicit FBindings(FVoxelNodeRuntime& NodeRuntime)
			: NodeRuntime(NodeRuntime)
		{
		}

		friend FVoxelNodeCaller;
	};

	template<typename NodeType, typename OutputType>
	struct TLambdaCaller
	{
		const FVoxelQuery& Query;
		const FVoxelPinRef OutputPin;

		template<typename LambdaType>
		TVoxelFutureValue<OutputType> operator+(LambdaType Lambda)
		{
			return TVoxelFutureValue<OutputType>(FVoxelNodeCaller::CallNode(
				NodeType::StaticStruct(),
				Query,
				OutputPin,
				[&](FBindings& Bindings, FVoxelNode& Node)
				{
					Lambda(Bindings, CastChecked<NodeType>(Node));
				}));
		}
	};

private:
	static FVoxelFutureValue CallNode(
		UScriptStruct* Struct,
		const FVoxelQuery& Query, 
		const FVoxelPinRef OutputPin, 
		const TFunctionRef<void(FBindings&, FVoxelNode& Node)> Bind);
};

#define VOXEL_CALL_NODE(NodeType, OutputPin) \
	FVoxelNodeCaller::TLambdaCaller<NodeType, VOXEL_GET_TYPE(NodeType().OutputPin)::Type>{ Query, VOXEL_PIN_NAME(NodeType, OutputPin) } + \
		[&](FVoxelNodeCaller::FBindings& Bindings, NodeType& Node)

#define VOXEL_CALL_NODE_BIND(Name, ...) \
	Bindings.Bind(Node.Name) = [this, \
		ReturnPinType = Node.GetNodeRuntime().GetPinData(Node.Name).Type, \
		ReturnPinStatName = Node.GetNodeRuntime().GetPinData(Node.Name).StatName, \
		ReturnType = TVoxelTypeInstance<decltype(Node.Name)>(), \
		##__VA_ARGS__](const FVoxelQuery& Query) \
	-> \
	TChooseClass< \
		TIsDerivedFrom<decltype(Node.Name), FVoxelPinRef>::Value, \
		TValue<decltype(Node.Name)::Type>, \
		TArray<TValue<decltype(Node.Name)::Type>> \
	>::Result