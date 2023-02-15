// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelNode.h"
#include "VoxelExecNode.h"
#include "VoxelNodeDocs.h"
#include "VoxelGraphMessages.h"
#include "VoxelMetaGraphGraph.h"
#include "VoxelRuntime/VoxelSubsystem.h"
#include "Nodes/Templates/VoxelTemplateNode.h"

TMap<FName, FVoxelNodeComputePtrs> GVoxelNodeStaticComputes;

void FVoxelNodeComputePtrs::Initialize(
	const UScriptStruct* Node, 
	const FName PinName,
	const FVoxelNodeComputePtr Ptr, 
	const FString& Debug, 
	const bool bCpu, 
	const bool bGpu)
{
	for (const UScriptStruct* ChildNode : GetDerivedStructs(Node, true))
	{
		FVoxelNodeComputePtrs& Ptrs = GVoxelNodeStaticComputes.FindOrAdd(*(ChildNode->GetStructCPPName() + "." + PinName.ToString()));

		if (bCpu)
		{
			checkf(!Ptrs.Cpu, TEXT("%s and %s both initialize the same node output"), *Ptrs.DebugCpu, *Debug);
			Ptrs.Cpu = Ptr;
			Ptrs.DebugCpu = Debug;
		}

		if (bGpu)
		{
			checkf(!Ptrs.Gpu, TEXT("%s and %s both initialize the same node output"), *Ptrs.DebugGpu, *Debug);
			Ptrs.Gpu = Ptr;
			Ptrs.DebugGpu = Debug;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
VOXEL_RUN_ON_STARTUP_GAME(FixupVoxelNodes)
{
	VOXEL_FUNCTION_COUNTER();

	for (UScriptStruct* Struct : GetDerivedStructs<FVoxelNode>())
	{
		if (Struct->HasMetaData("Abstract"))
		{
			continue;
		}

		if (Struct->GetPathName().StartsWith("/Script/Voxel"))
		{
			if (Struct->IsChildOf(FVoxelTemplateNode::StaticStruct()))
			{
				ensure(Struct->GetStructCPPName().StartsWith("FVoxelTemplateNode_"));
			}
			else if (Struct->IsChildOf(FVoxelExecNode::StaticStruct()))
			{
				ensure(Struct->GetStructCPPName().StartsWith("FVoxelExecNode_"));
			}
			else if (Struct->IsChildOf(FVoxelChunkExecNode::StaticStruct()))
			{
				ensure(Struct->GetStructCPPName().StartsWith("FVoxelChunkExecNode_"));
			}
			else
			{
				ensure(Struct->GetStructCPPName().StartsWith("FVoxelNode_"));
			}
		}

		const FString DefaultName = FName::NameToDisplayString(Struct->GetName(), false);

		TArray<FString> PotentialObjectNames;
		FString StructName = Struct->GetName();
		{
			TArray<FString> Array;
			StructName.ParseIntoArray(Array, TEXT("_"));
			StructName = Array.Last();

			for (const FString& Name : Array)
			{
				if (Name != StructName &&
					Name.StartsWith(TEXT("F"), ESearchCase::CaseSensitive))
				{
					PotentialObjectNames.Add(Name);
				}
			}
		}
		const FString FixedName = FName::NameToDisplayString(StructName, false);

		if (Struct->GetDisplayNameText().ToString() == DefaultName)
		{
			Struct->SetMetaData("DisplayName", *FixedName);
		}

		if (PotentialObjectNames.Num() == 0)
		{
			continue;
		}

		// Look for object name

		TVoxelInstancedStruct<FVoxelNode> Node(Struct);
		for (const FVoxelPin& Pin : Node->GetPins())
		{
			if (!Pin.GetType().IsStruct())
			{
				continue;
			}

			const UScriptStruct* PinStruct = Pin.GetType().GetStruct();
			if (!PotentialObjectNames.Contains(PinStruct->GetStructCPPName()))
			{
				continue;
			}

			FString PinStructName = PinStruct->GetDisplayNameText().ToString();
			PinStructName.RemoveFromStart("Voxel ");

			FString Name = Struct->GetDisplayNameText().ToString();
			Name += "\n";
			Name += Pin.bIsInput ? "from " : "as ";
			Name += PinStructName;

			Struct->SetMetaData("DisplayName", *Name);
			break;
		}
	}

	FVoxelNodeDocs::Get().Initialize();
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelFutureValue FVoxelNodeRuntime::Get(const FVoxelPinRef& Pin, const FVoxelQuery& Query) const
{
	if (const TFunction<FVoxelFutureValue(const FVoxelQuery&)>* Override = PinOverrides.Find(Pin))
	{
		return (*Override)(Query);
	}

	const FPinData& PinData = GetPinData(Pin);
	check(PinData.bIsInput);

	if (!PinData.OutputPinData)
	{
		ensure(PinData.DefaultValue.IsValid());
		return PinData.DefaultValue;
	}

	const TSharedRef<const FComputeState> ComputeState = PinData.OutputPinData->ComputeState.ToSharedRef();

	FVoxelQuery LocalQuery = Query;
	LocalQuery.Callstack.Add(ComputeState->Node);

	return FVoxelTask::New(
		MakeShared<FVoxelTaskStat>(*ComputeState->Node),
		ComputeState->Type,
		ComputeState->StatName,
		EVoxelTaskThread::AnyThread,
		{},
		[ComputeState, LocalQuery = MoveTemp(LocalQuery)]
		{
			return ComputeState->Compute(LocalQuery);
		});
}

TVoxelFutureValue<FVoxelBufferView> FVoxelNodeRuntime::GetBufferView(const FVoxelPinRef& Pin, const FVoxelQuery& Query) const
{
	const TValue<FVoxelBuffer> Buffer = Get<FVoxelBuffer>(Pin, Query);

	return FVoxelTask::New<FVoxelBufferView>(
		MakeShared<FVoxelTaskStat>(),
		STATIC_FNAME("MakeView"),
		EVoxelTaskThread::AnyThread,
		{ Buffer },
		[Buffer]
		{
			return Buffer.Get_CheckCompleted().MakeGenericView();
		});
}

bool FVoxelNodeRuntime::IsDefaultValue(const FVoxelPinRef& Pin) const
{
	return !GetPinData(Pin).OutputPinData.IsValid();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelNode& FVoxelNode::operator=(const FVoxelNode& Other)
{
	check(GetStruct() == Other.GetStruct());
	ensure(!NodeRuntime);
	ensure(!Other.NodeRuntime);

	ExposedPins = Other.ExposedPins;
	ExposedPinsValues = Other.ExposedPinsValues;

	FlushDeferredPins();
	Other.FlushDeferredPins();

	InternalPinBackups.Reset();
	InternalPins.Reset();
	InternalPinArrays.Reset();
	InternalPinsOrder.Reset();

	SortOrderCounter = Other.SortOrderCounter;

	TArray<FDeferredPin> Pins;
	Other.InternalPinBackups.GenerateValueArray(Pins);

	// Register arrays first
	Pins.Sort([](const FDeferredPin& A, const FDeferredPin& B)
	{
		return A.ArrayOwner.IsNone() > B.ArrayOwner.IsNone();
	});

	for (const FDeferredPin& Pin : Pins)
	{
		RegisterPin(Pin, false);
	}

	for (FName PinName : Other.InternalPinsOrder)
	{
		InternalPinsOrder.Add(PinName);
	}

	LoadSerializedData(Other.GetSerializedData());

	return *this;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
FString FVoxelNode::GetCategory() const
{
	FString Category;
	ensure(GetStruct()->GetStringMetaDataHierarchical("Category", &Category));
	ensure(!Category.IsEmpty());
	return Category;
}

FString FVoxelNode::GetDisplayName() const
{
	return GetStruct()->GetDisplayNameText().ToString();
}

FString FVoxelNode::GetTooltip() const
{
	return FVoxelNodeDocs::Get().GetNodeTooltip(GetStruct());
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNode::ReturnToPool()
{
	VOXEL_FUNCTION_COUNTER();

	for (const FReturnToPoolFunc& ReturnToPoolFunc : ReturnToPoolFuncs)
	{
		(this->*ReturnToPoolFunc)();
	}
}

TVoxelFunction<FVoxelFutureValue(const FVoxelQuery&)> FVoxelNode::Compile(FName PinName) const
{
	const FName Name = FName(GetStruct()->GetStructCPPName() + "." + PinName.ToString());
	if (!GVoxelNodeStaticComputes.Contains(Name))
	{
		return nullptr;
	}

	const FVoxelNodeComputePtrs Ptrs = GVoxelNodeStaticComputes.FindChecked(Name);

	return [this, Ptrs, Outer = GetOuter()](const FVoxelQuery& Query)
	{
		(void)Outer;

		FVoxelNodeComputePtr Ptr;
		if (Query.IsGpu())
		{
			Ptr = Ptrs.Gpu;

			if (!Ptr)
			{
				Ptr = Ptrs.Cpu;
			}
		}
		else
		{
			Ptr = Ptrs.Cpu;

			if (!Ptr)
			{
				Ptr = Ptrs.Gpu;
			}
		}
		check(Ptr);

		return (*Ptr)(*this, Query);
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNode::PreSerialize()
{
	Super::PreSerialize();

	SerializedDataProperty = GetSerializedData();
}

void FVoxelNode::PostSerialize()
{
	Super::PostSerialize();

	LoadSerializedData(SerializedDataProperty);
	SerializedDataProperty = {};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<IVoxelNodeDefinition> FVoxelNode::GetNodeDefinition()
{
	return MakeShared<FVoxelNodeDefinition>(*this);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelPinTypeSet FVoxelNode::GetPromotionTypes(const FVoxelPin& Pin) const
{
	ensure(Pin.IsPromotable());

	FVoxelPinTypeSet Types;

	if (EnumHasAllFlags(Pin.Flags, EVoxelPinFlags::MathPin))
	{
		Types.Add(Pin.GetType().GetInnerType());
		Types.Add(Pin.GetType().GetBufferType());
	}
	else
	{
		Types.Add(Pin.GetType());
	}

	return Types;
}

void FVoxelNode::PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	ensure(EnumHasAllFlags(Pin.Flags, EVoxelPinFlags::MathPin));
	ensure(NewType.GetInnerType() == Pin.GetType().GetInnerType());

	for (FVoxelPin& OtherPin : GetPins())
	{
		if (!EnumHasAllFlags(OtherPin.Flags, EVoxelPinFlags::MathPin))
		{
			continue;
		}

		if (NewType.IsBuffer())
		{
			OtherPin.SetType(OtherPin.GetType().GetBufferType());
		}
		else
		{
			OtherPin.SetType(OtherPin.GetType().GetInnerType());
		}
	}
}

bool FVoxelNode::AreMathPinsBuffers() const
{
	VOXEL_FUNCTION_COUNTER();

	TOptional<bool> Result;
	for (const FVoxelPin& Pin : GetPins())
	{
		if (!EnumHasAllFlags(Pin.Flags, EVoxelPinFlags::MathPin))
		{
			continue;
		}

		if (!Result)
		{
			Result = Pin.GetType().IsBuffer();
		}
		else
		{
			ensure(*Result == Pin.GetType().IsBuffer());
		}
	}
	ensure(Result.IsSet());

	return Result.Get(false);
}

bool FVoxelNode::IsPinExposed(const FVoxelPin& Pin) const
{
	if (!Pin.Metadata.bPropertyBind)
	{
		return true;
	}

	return ExposedPins.Contains(Pin.Name);
}

bool FVoxelNode::IsPinHidden(const FVoxelPin& Pin) const
{
	return !IsPinExposed(Pin);
}

FVoxelPinValue FVoxelNode::GetPinDefaultValue(const FVoxelPin& Pin) const
{
	if (!Pin.Metadata.bPropertyBind)
	{
		return Pin.DefaultValue;
	}

	if (const FVoxelPinValue* PinValue = ExposedPinsValues.Find(Pin.Name))
	{
		return *PinValue;
	}

	ensure(false);
	return Pin.DefaultValue;
}

void FVoxelNode::UpdatePropertyBoundDefaultValue(const FVoxelPin& Pin, const FVoxelPinValue& NewValue)
{
	if (!Pin.Metadata.bPropertyBind)
	{
		return;
	}

	if (FVoxelPinValue* PinValue = ExposedPinsValues.Find(Pin.Name))
	{
		if (*PinValue != NewValue)
		{
			(*PinValue) = NewValue;
			OnExposedPinsUpdated.Broadcast();
		}
	}
	else
	{
		ExposedPinsValues.Add(Pin.Name, NewValue);
		OnExposedPinsUpdated.Broadcast();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelPin& FVoxelNode::GetUniqueInputPin()
{
	FVoxelPin* InputPin = nullptr;
	for (FVoxelPin& Pin : GetPins())
	{
		if (Pin.bIsInput)
		{
			check(!InputPin);
			InputPin = &Pin;
		}
	}
	check(InputPin);

	return *InputPin;
}

FVoxelPin& FVoxelNode::GetUniqueOutputPin()
{
	FVoxelPin* OutputPin = nullptr;
	for (FVoxelPin& Pin : GetPins())
	{
		if (!Pin.bIsInput)
		{
			check(!OutputPin);
			OutputPin = &Pin;
		}
	}
	check(OutputPin);

	return *OutputPin;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FName FVoxelNode::AddPinToArray(const FName ArrayName, FName PinName)
{
	FlushDeferredPins();

	const TSharedPtr<FPinArray> PinArray = InternalPinArrays.FindRef(ArrayName);
	if (!ensure(PinArray))
	{
		return {};
	}

	if (PinName.IsNone())
	{
		PinName = ArrayName + "_0";

		while (InternalPins.Contains(PinName) || InternalPinArrays.Contains(PinName))
		{
			PinName.SetNumber(PinName.GetNumber() + 1);
		}
	}

	if (!ensure(!InternalPins.Contains(PinName)) ||
		!ensure(!InternalPinArrays.Contains(PinName)))
	{
		return {};
	}

	FDeferredPin Pin = PinArray->PinTemplate;
	Pin.Name = PinName;
	RegisterPin(Pin);

	FixupArrayNames(ArrayName);

	return PinName;
}

FName FVoxelNode::InsertPinToArrayPosition(FName ArrayName, int32 Position)
{
	FlushDeferredPins();

	const TSharedPtr<FPinArray> PinArray = InternalPinArrays.FindRef(ArrayName);
	if (!ensure(PinArray))
	{
		return {};
	}

	FName PinName = ArrayName + "_0";
	while (InternalPins.Contains(PinName) || InternalPinArrays.Contains(PinName))
	{
		PinName.SetNumber(PinName.GetNumber() + 1);
	}

	if (!ensure(!InternalPins.Contains(PinName)) ||
		!ensure(!InternalPinArrays.Contains(PinName)))
	{
		return {};
	}

	FDeferredPin Pin = PinArray->PinTemplate;
	Pin.Name = PinName;
	RegisterPin(Pin);
	SortPins();

	for (int32 Index = Position; Index < PinArray->Pins.Num() - 1; Index++)
	{
		PinArray->Pins.Swap(Index, PinArray->Pins.Num() - 1);
	}

	FixupArrayNames(ArrayName);

	return PinName;
}

void FVoxelNode::FixupArrayNames(FName ArrayName)
{
	FlushDeferredPins();

	const TSharedPtr<FPinArray> PinArray = InternalPinArrays.FindRef(ArrayName);
	if (!ensure(PinArray))
	{
		return;
	}

	for (int32 Index = 0; Index < PinArray->Pins.Num(); Index++)
	{
		const FName ArrayPinName = PinArray->Pins[Index];
		const TSharedPtr<FVoxelPin> ArrayPin = InternalPins.FindRef(ArrayPinName);
		if (!ensure(ArrayPin))
		{
			continue;
		}

		VOXEL_CONST_CAST(ArrayPin->SortOrder) = PinArray->PinTemplate.SortOrder + Index / 100.f;
		VOXEL_CONST_CAST(ArrayPin->Metadata.DisplayName) = FName::NameToDisplayString(ArrayName.ToString(), false) + " " + FString::FromInt(Index);
	}

	SortPins();
}

FName FVoxelNode::CreatePin(
	FVoxelPinType Type,
	const bool bIsInput,
	const FName Name,
	const FVoxelPinValue& DefaultValue,
	const FVoxelPinMetadata& Metadata,
	const EVoxelPinFlags Flags,
	const int32 MinArrayNum)
{
	Type.SetTag(Metadata.Tag);

	FVoxelPinType BaseType = Type;
	const FVoxelPinType ChildType = Type;

	if (EnumHasAnyFlags(Flags, EVoxelPinFlags::MathPin))
	{
		BaseType = FVoxelPinType::MakeWildcard();
		ensure(!ChildType.IsWildcard());
	}

	RegisterPin(FDeferredPin
	{
		{},
		MinArrayNum,
		Name,
		bIsInput,
		0,
		Flags,
		BaseType,
		ChildType,
		DefaultValue,
		Metadata
	});

	if (Metadata.bDisplayLast)
	{
		InternalPinsOrder.Add(Name);
		DisplayLastPins++;
	}
	else
	{
		InternalPinsOrder.Add(Name);
		if (DisplayLastPins > 0)
		{
			for (int32 Index = InternalPinsOrder.Num() - DisplayLastPins - 1; Index < InternalPinsOrder.Num() - 1; Index++)
			{
				InternalPinsOrder.Swap(Index, InternalPinsOrder.Num() - 1);
			}
		}
	}

	return Name;
}

void FVoxelNode::RemovePin(const FName Name)
{
	FlushDeferredPins();

	const TSharedPtr<FVoxelPin> Pin = FindPin(Name);
	if (ensure(Pin) &&
		!Pin->ArrayOwner.IsNone() &&
		ensure(InternalPinArrays.Contains(Pin->ArrayOwner)))
	{
		ensure(InternalPinArrays[Pin->ArrayOwner]->Pins.Remove(Name));
	}

	ensure(InternalPinBackups.Remove(Name));
	ensure(InternalPins.Remove(Name));
	InternalPinsOrder.Remove(Name);

	if (Pin->Metadata.bPropertyBind)
	{
		OnExposedPinsUpdated.Broadcast();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNode::FlushDeferredPinsImpl()
{
	ensure(bIsDeferringPins);
	bIsDeferringPins = false;

	for (const FDeferredPin& Pin : DeferredPins)
	{
		RegisterPin(Pin);
	}
	DeferredPins.Empty();
}

void FVoxelNode::RegisterPin(FDeferredPin Pin, bool bApplyMinNum)
{
	ensure(!Pin.Name.IsNone());
	ensure(Pin.BaseType.IsValid());

	if (Pin.ArrayOwner.IsNone() &&
		Pin.SortOrder == 0)
	{
		Pin.SortOrder = SortOrderCounter++;
	}

	if (Pin.Metadata.bDisplayLast)
	{
		Pin.SortOrder += 10000.f;
	}

	if (bIsDeferringPins)
	{
		DeferredPins.Add(Pin);
		return;
	}

	ensure(!InternalPinBackups.Contains(Pin.Name));
	InternalPinBackups.Add(Pin.Name, Pin);

	const FString PinName = Pin.Name.ToString();

#if WITH_EDITOR
	if (Pin.Metadata.Tooltip.IsEmpty())
	{
		Pin.Metadata.Tooltip = FVoxelNodeDocs::Get().GetPinTooltip(GetStruct(), Pin.ArrayOwner.IsNone() ? Pin.Name : Pin.ArrayOwner);
	}

	if (!Pin.Metadata.Category.IsEmpty() &&
		Pin.ArrayOwner.IsNone() &&
		!MappedCategoryTooltips.Contains(FName(Pin.Metadata.Category)))
	{
		if (Pin.Metadata.CategoryTooltip.IsEmpty())
		{
			MappedCategoryTooltips.Add(FName(Pin.Metadata.Category), FVoxelNodeDocs::Get().GetCategoryTooltip(GetStruct(), FName(Pin.Metadata.Category)));
		}
		else
		{
			MappedCategoryTooltips.Add(FName(Pin.Metadata.Category), Pin.Metadata.CategoryTooltip);
		}
	}

	if (Pin.Metadata.DisplayName.IsEmpty())
	{
		Pin.Metadata.DisplayName = FName::NameToDisplayString(PinName, PinName.StartsWith("b", ESearchCase::CaseSensitive));
		Pin.Metadata.DisplayName.RemoveFromStart("Out ");

		if (Pin.BaseType.IsDerivedFrom<FVoxelExecBase>() && (Pin.Metadata.DisplayName == "Exec" || Pin.Metadata.DisplayName == "Then"))
		{
			Pin.Metadata.DisplayName = {};
		}
	}
#endif

	if (Pin.Metadata.bPropertyBind)
	{
		OnExposedPinsUpdated.Broadcast();
		if (!ExposedPinsValues.Contains(Pin.Name))
		{
			ExposedPinsValues.Add(Pin.Name, Pin.DefaultValue);
		}
	}

	if (EnumHasAnyFlags(Pin.Flags, EVoxelPinFlags::ArrayPin))
	{
		ensure(Pin.ArrayOwner.IsNone());

		FDeferredPin PinTemplate = Pin;
		PinTemplate.ArrayOwner = Pin.Name;
		EnumRemoveFlags(PinTemplate.Flags, EVoxelPinFlags::ArrayPin);

		ensure(!InternalPinArrays.Contains(Pin.Name));
		InternalPinArrays.Add(Pin.Name, MakeShared<FPinArray>(PinTemplate));

		if (bApplyMinNum)
		{
			for (int32 Index = 0; Index < Pin.MinArrayNum; Index++)
			{
				AddPinToArray(Pin.Name);
			}
		}
	}
	else
	{
		if (!Pin.ArrayOwner.IsNone() &&
			ensure(InternalPinArrays.Contains(Pin.ArrayOwner)))
		{
			FPinArray& PinArray = *InternalPinArrays[Pin.ArrayOwner];
			Pin.Metadata.Category = Pin.ArrayOwner.ToString();
			PinArray.Pins.Add(Pin.Name);
		}

		ensure(!InternalPins.Contains(Pin.Name));
		InternalPins.Add(Pin.Name, MakeSharedCopy(FVoxelPin(
			Pin.Name,
			Pin.bIsInput,
			Pin.SortOrder,
			Pin.ArrayOwner,
			Pin.Flags,
			Pin.BaseType,
			Pin.ChildType,
			Pin.DefaultValue,
			Pin.Metadata)));
	}

	SortPins();
}

void FVoxelNode::SortPins()
{
	VOXEL_FUNCTION_COUNTER();

	InternalPins.ValueSort([](const TSharedPtr<FVoxelPin>& A, const TSharedPtr<FVoxelPin>& B)
	{
		if (A->bIsInput != B->bIsInput)
		{
			return A->bIsInput > B->bIsInput;
		}

		return A->SortOrder < B->SortOrder;
	});
}

void FVoxelNode::SortArrayPins(FName PinArrayName)
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedPtr<FPinArray> PinArray = InternalPinArrays.FindRef(PinArrayName);
	if (!PinArray)
	{
		return;
	}

	TArray<int32> SortOrders;
	TArray<TSharedPtr<FVoxelPin>> AffectedPins;
	for (const FName PinName : PinArray->Pins)
	{
		if (TSharedPtr<FVoxelPin> Pin = InternalPins.FindRef(PinName))
		{
			SortOrders.Add(Pin->SortOrder);
		}
	}

	SortOrders.Sort();
	for (int32 Index = 0; Index < AffectedPins.Num(); Index++)
	{
		VOXEL_CONST_CAST(AffectedPins[Index]->SortOrder) = SortOrders[Index];
	}

	SortPins();
}

#if WITH_EDITOR
void FVoxelNode::GetExternalPinsData(TArray<FName>& OutPinNames, TArray<FName>& OutCategoryNames) const
{
	TArray<FName> InputPins;
	TArray<FName> OutputPins;

	TArray<FName> InputCategories;
	TArray<FName> OutputCategories;

	for (const FName PinName : InternalPinsOrder)
	{
		const FDeferredPin* Pin = nullptr;
		if (const FDeferredPin* PinBackup = InternalPinBackups.Find(PinName))
		{
			Pin = PinBackup;
		}
		else if (const FDeferredPin* DeferredPin = DeferredPins.FindByPredicate([PinName](const FDeferredPin& TargetPin)
		{
			return TargetPin.Name == PinName;
		}))
		{
			Pin = DeferredPin;
		}
		else
		{
			OutPinNames.Add(PinName);
			ensure(false);
			continue;
		}

		(Pin->bIsInput ? InputPins : OutputPins).Add(PinName);
		if (!Pin->Metadata.Category.IsEmpty())
		{
			(Pin->bIsInput ? InputCategories : OutputCategories).AddUnique(FName(Pin->Metadata.Category));
		}
	}

	OutPinNames.Append(InputPins);
	OutPinNames.Append(OutputPins);

	OutCategoryNames.Append(InputCategories);
	OutCategoryNames.Append(OutputCategories);
}
#endif

FString FVoxelNode::GetExternalPinTooltip(const FName PinName) const
{
#if WITH_EDITOR
	return FVoxelNodeDocs::Get().GetPinTooltip(GetStruct(), PinName);
#else
	return "";
#endif
}

FString FVoxelNode::GetExternalCategoryTooltip(const FName CategoryName) const
{
#if WITH_EDITOR
	return FVoxelNodeDocs::Get().GetCategoryTooltip(GetStruct(), CategoryName);
#else
	return "";
#endif
}

FVoxelNodeSerializedData FVoxelNode::GetSerializedData() const
{
	FlushDeferredPins();

	FVoxelNodeSerializedData SerializedData;

	for (const auto& It : InternalPins)
	{
		const FVoxelPin& Pin = *It.Value;
		if (!Pin.IsPromotable())
		{
			continue;
		}

		SerializedData.PinTypes.Add(Pin.Name, Pin.GetType());
	}

	for (const auto& It : InternalPinArrays)
	{
		FVoxelNodeSerializedArrayData ArrayData;
		ArrayData.PinNames = It.Value->Pins;
		SerializedData.ArrayDatas.Add(It.Key, ArrayData);
	}

	SerializedData.ExposedPins = ExposedPins;
	SerializedData.ExposedPinsValues = ExposedPinsValues;

	return SerializedData;
}

void FVoxelNode::LoadSerializedData(const FVoxelNodeSerializedData& SerializedData)
{
	ExposedPins = SerializedData.ExposedPins;
	ExposedPinsValues = SerializedData.ExposedPinsValues;

	FlushDeferredPins();

	for (const auto& It : InternalPinArrays)
	{
		FPinArray& PinArray = *It.Value;

		const TArray<FName> Pins = PinArray.Pins;
		for (const FName Name : Pins)
		{
			RemovePin(Name);
		}
		ensure(PinArray.Pins.Num() == 0);
	}

	for (const auto& It : SerializedData.ArrayDatas)
	{
		const TSharedPtr<FPinArray> PinArray = InternalPinArrays.FindRef(It.Key);
		if (!ensure(PinArray))
		{
			continue;
		}

		for (const FName Name : It.Value.PinNames)
		{
			AddPinToArray(It.Key, Name);
		}
	}

	// Ensure MinNum is applied
	for (const auto& It : InternalPinArrays)
	{
		FPinArray& PinArray = *It.Value;

		for (int32 Index = PinArray.Pins.Num(); Index < PinArray.PinTemplate.MinArrayNum; Index++)
		{
			AddPinToArray(It.Key);
		}
	}

	for (const auto& It : SerializedData.PinTypes)
	{
		const TSharedPtr<FVoxelPin> Pin = InternalPins.FindRef(It.Key);
		if (!ensure(Pin) ||
			!ensure(Pin->IsPromotable()))
		{
			continue;
		}

		if (EnumHasAnyFlags(Pin->Flags, EVoxelPinFlags::MathPin) &&
			!ensure(It.Value.GetInnerType() == Pin->ChildType.GetInnerType()))
		{
			continue;
		}

		Pin->SetType(It.Value);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNode::InitializeNodeRuntime(
	FVoxelRuntime* Runtime,
	const TWeakPtr<IVoxelNodeOuter> WeakOuter,
	const TWeakObjectPtr<const UVoxelMetaGraph> WeakGraph,
	const Voxel::MetaGraph::FNode* SourceNode)
{
	VOXEL_FUNCTION_COUNTER();

	FlushDeferredPins();

	FString DebugName = GetStruct()->GetName();
	DebugName.RemoveFromStart("VoxelNode_");
	DebugName.RemoveFromStart("VoxelMathNode_");
	DebugName.RemoveFromStart("VoxelTemplateNode_");

	check(!NodeRuntime);
	NodeRuntime = MakeShared<FVoxelNodeRuntime>();
	NodeRuntime->Node = this;
	if (Runtime)
	{
		NodeRuntime->WeakRuntime = Runtime->AsShared();
	}
	NodeRuntime->WeakOuter = WeakOuter;
	NodeRuntime->WeakGraph = WeakGraph;
	NodeRuntime->SourceNode = SourceNode;
	NodeRuntime->StatName = *DebugName;

	for (const FVoxelPin& Pin : GetPins())
	{
		NodeRuntime->PinDatas.Add(Pin.Name, MakeShared<FVoxelNodeRuntime::FPinData>(
			Pin.GetType(),
			Pin.bIsInput,
			FName(DebugName + "." + Pin.Name.ToString())));
	}

	for (const auto& It : InternalPinArrays)
	{
		NodeRuntime->PinArrays.Add(It.Key, It.Value->Pins);
	}

	// Assign compute states to output pins
	for (const FVoxelPin& Pin : GetPins())
	{
		if (Pin.bIsInput ||
			Pin.GetType().IsDerivedFrom<FVoxelExecBase>())
		{
			continue;
		}

		const TVoxelFunction<FVoxelFutureValue(const FVoxelQuery&)> Compute = Compile(Pin.Name);
		if (!Compute)
		{
			VOXEL_MESSAGE(Error, "INTERNAL ERROR: {0}.{1} has no Compute", this, Pin.Name);
			return;
		}

		FVoxelNodeRuntime::FPinData& PinData = NodeRuntime->GetPinData(Pin.Name);
		PinData.ComputeState = MakeSharedCopy(FVoxelNodeRuntime::FComputeState
		{
			Pin.GetType().WithoutTag(),
			this,
			FName(DebugName + "." + Pin.Name.ToString() + ".Compute"),
			Compute
		});
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<const IVoxelNodeDefinition::FNode> FVoxelNodeDefinition::GetInputs() const
{
	return GetPins(true);
}

TSharedPtr<const IVoxelNodeDefinition::FNode> FVoxelNodeDefinition::GetOutputs() const
{
	return GetPins(false);
}

TSharedPtr<const IVoxelNodeDefinition::FNode> FVoxelNodeDefinition::GetPins(const bool bInput) const
{
	TArray<TSharedRef<FNode>> Leaves;

	for (const FName PinName : Node.InternalPinsOrder)
	{
		const FVoxelNode::FDeferredPin* Pin = Node.InternalPinBackups.Find(PinName);
		if (!ensure(Pin))
		{
			continue;
		}

		if (Pin->bIsInput != bInput)
		{
			continue;
		}

		if (Pin->IsArrayElement())
		{
			continue;
		}

		if (Pin->IsArrayDeclaration())
		{
			TArray<FName> PinArrayPath = FNode::MakePath(Pin->Metadata.Category, PinName);
			TSharedRef<FNode> ArrayCategoryNode = FNode::MakeArrayCategory(PinName, PinArrayPath);
			Leaves.Add(ArrayCategoryNode);

			if (const TSharedPtr<FVoxelNode::FPinArray>& PinArray = Node.InternalPinArrays.FindRef(PinName))
			{
				for (const FName ArrayElement : PinArray->Pins)
				{
					ArrayCategoryNode->Children.Add(FNode::MakePin(ArrayElement, PinArrayPath));
				}
			}
			continue;
		}

		Leaves.Add(FNode::MakePin(PinName, FNode::MakePath(Pin->Metadata.Category)));
	}

	const TSharedRef<FNode> Root = FNode::MakeCategory({}, {});
	TMap<FName, TSharedPtr<FNode>> MappedCategories;

	const auto FindOrAddCategory = [&MappedCategories](const TSharedPtr<FNode>& Parent, const FString& PathElement, const FName FullPath)
	{
		if (const TSharedPtr<FNode>& CategoryNode = MappedCategories.FindRef(FullPath))
		{
			return CategoryNode.ToSharedRef();
		}

		TSharedRef<FNode> Category = FNode::MakeCategory(FName(PathElement), FNode::MakePath(FullPath.ToString()));
		Parent->Children.Add(Category);
		MappedCategories.Add(FullPath, Category);

		return Category;
	};

	for (const TSharedRef<FNode>& Leaf : Leaves)
	{
		int32 NumPath = Leaf->Path.Num();
		if (Leaf->NodeState != ENodeState::Pin)
		{
			NumPath--;
		}

		if (NumPath == 0)
		{
			Root->Children.Add(Leaf);
			continue;
		}

		FString CurrentPath = Leaf->Path[0].ToString();
		TSharedRef<FNode> ParentCategoryNode = FindOrAddCategory(Root, CurrentPath, FName(CurrentPath));

		for (int32 Index = 1; Index < NumPath; Index++)
		{
			CurrentPath += "|" + Leaf->Path[Index].ToString();
			ParentCategoryNode = FindOrAddCategory(ParentCategoryNode, Leaf->Path[Index].ToString(), FName(CurrentPath));
		}

		ParentCategoryNode->Children.Add(Leaf);
	}

	return Root;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString FVoxelNodeDefinition::GetAddPinLabel() const
{
	return "Add pin";
}

FString FVoxelNodeDefinition::GetAddPinTooltip() const
{
	return "Add pin";
}

FString FVoxelNodeDefinition::GetRemovePinTooltip() const
{
	return "Remove pin";
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelNodeDefinition::CanAddToCategory(FName Category) const
{
	return Node.InternalPinArrays.Contains(Category);
}

void FVoxelNodeDefinition::AddToCategory(FName Category)
{
	Node.AddPinToArray(Category);
}

bool FVoxelNodeDefinition::CanRemoveFromCategory(FName Category) const
{
	const TSharedPtr<FVoxelNode::FPinArray> PinArray = Node.InternalPinArrays.FindRef(Category);
	return PinArray && PinArray->Pins.Num() > PinArray->PinTemplate.MinArrayNum;
}

void FVoxelNodeDefinition::RemoveFromCategory(FName Category)
{
	const TSharedPtr<FVoxelNode::FPinArray> PinArray = Node.InternalPinArrays.FindRef(Category);
	if (!ensure(PinArray) ||
		!ensure(PinArray->Pins.Num() > 0) ||
		!ensure(PinArray->Pins.Num() > PinArray->PinTemplate.MinArrayNum))
	{
		return;
	}

	const FName PinName = PinArray->Pins.Last();
	Node.RemovePin(PinName);

	Node.ExposedPins.Remove(PinName);
	Node.ExposedPinsValues.Remove(PinName);

	Node.FixupArrayNames(Category);
}

bool FVoxelNodeDefinition::CanRemoveSelectedPin(FName PinName) const
{
	if (const TSharedPtr<FVoxelPin> Pin = Node.FindPin(PinName))
	{
		return CanRemoveFromCategory(Pin->ArrayOwner);
	}

	return false;
}

void FVoxelNodeDefinition::RemoveSelectedPin(FName PinName)
{
	if (!ensure(CanRemoveSelectedPin(PinName)))
	{
		return;
	}

	const TSharedPtr<FVoxelPin> Pin = Node.FindPin(PinName);

	if (Pin->Metadata.bPropertyBind)
	{
		Node.ExposedPins.Remove(Pin->Name);
		Node.ExposedPinsValues.Remove(Pin->Name);
	}

	Node.RemovePin(Pin->Name);

	Node.FixupArrayNames(Pin->ArrayOwner);
}

void FVoxelNodeDefinition::InsertPinBefore(FName PinName)
{
	const TSharedPtr<FVoxelPin> Pin = Node.FindPin(PinName);
	if (!Pin)
	{
		return;
	}

	const TSharedPtr<FVoxelNode::FPinArray> PinArray = Node.InternalPinArrays.FindRef(Pin->ArrayOwner);
	if (!PinArray)
	{
		return;
	}

	const int32 PinPosition = PinArray->Pins.IndexOfByPredicate([&Pin] (const FName& Name)
	{
		return Pin->Name == Name;
	});

	if (!ensure(PinPosition != -1))
	{
		return;
	}

	const FName NewPinName = Node.InsertPinToArrayPosition(Pin->ArrayOwner, PinPosition);
	Node.SortArrayPins(Pin->ArrayOwner);
	if (Pin->Metadata.bPropertyBind)
	{
		if (Node.ExposedPins.Contains(Pin->Name))
		{
			Node.ExposedPins.Add(NewPinName);
		}
	}
}

void FVoxelNodeDefinition::DuplicatePin(FName PinName)
{
	const TSharedPtr<FVoxelPin> Pin = Node.FindPin(PinName);
	if (!Pin)
	{
		return;
	}

	const TSharedPtr<FVoxelNode::FPinArray> PinArray = Node.InternalPinArrays.FindRef(Pin->ArrayOwner);
	if (!PinArray)
	{
		return;
	}

	const int32 PinPosition = PinArray->Pins.IndexOfByPredicate([&Pin] (const FName& Name)
	{
		return Pin->Name == Name;
	});

	if (!ensure(PinPosition != -1))
	{
		return;
	}

	const FName NewPinName = Node.InsertPinToArrayPosition(Pin->ArrayOwner, PinPosition + 1);
	Node.SortArrayPins(Pin->ArrayOwner);
	if (Pin->Metadata.bPropertyBind)
	{
		FVoxelPinValue NewValue;
		if (const FVoxelPinValue* PinValue = Node.ExposedPinsValues.Find(Pin->Name))
		{
			NewValue = *PinValue;
		}

		Node.ExposedPinsValues.Add(NewPinName, NewValue);

		if (Node.ExposedPins.Contains(Pin->Name))
		{
			Node.ExposedPins.Add(NewPinName);
		}
	}
}

#if WITH_EDITOR
FString FVoxelNodeDefinition::GetPinTooltip(FName PinName) const
{
	if (const FVoxelNode::FDeferredPin* Pin = Node.InternalPinBackups.Find(PinName))
	{
		return Pin->Metadata.Tooltip;
	}

	return {};
}

FString FVoxelNodeDefinition::GetCategoryTooltip(FName CategoryName) const
{
	if (const FString* CategoryTooltipPtr = Node.MappedCategoryTooltips.Find(CategoryName))
	{
		return *CategoryTooltipPtr;
	}

	return Node.GetExternalCategoryTooltip(CategoryName);
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelFutureValue FVoxelNodeCaller::CallNode(
	UScriptStruct* Struct,
	const FVoxelQuery& Query,
	const FVoxelPinRef OutputPin,
	const TFunctionRef<void(FBindings&, FVoxelNode& Node)> Bind)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(Query.Callstack.Num() > 0) ||
		!ensure(Query.Callstack.Last()))
	{
		return {};
	}
	const FVoxelNode& CallerNode = *Query.Callstack.Last();

	static FVoxelCriticalSection CriticalSection;
	static TMap<UScriptStruct*, TArray<TSharedRef<FVoxelNode>>> StructToPool;

	TSharedPtr<FVoxelNode> Node;
	{
		VOXEL_SCOPE_LOCK(CriticalSection);

		TArray<TSharedRef<FVoxelNode>>& Pool = StructToPool.FindOrAdd(Struct);
		if (Pool.Num() > 0)
		{
			Node = Pool.Pop(false);
		}
	}

	if (!Node)
	{
		struct FVoxelCallNodeOuter : public IVoxelNodeOuter {};
		static const TSharedRef<FVoxelCallNodeOuter> Outer = MakeShared<FVoxelCallNodeOuter>();

		Node = FVoxelSharedPinValue(FVoxelPinType::MakeStruct(Struct)).GetSharedStructCopy<FVoxelNode>();

		// Promote all math pins to buffers
		for (FVoxelPin& Pin : Node->GetPins())
		{
			if (EnumHasAllFlags(Pin.Flags, EVoxelPinFlags::MathPin))
			{
				Node->PromotePin(Pin, Pin.GetType().GetBufferType());
			}
		}

		Node->InitializeNodeRuntime(nullptr, Outer, nullptr, nullptr);
	}

	FVoxelNodeRuntime& NodeRuntime = VOXEL_CONST_CAST(Node->GetNodeRuntime());
	NodeRuntime.WeakRuntime = CallerNode.GetNodeRuntime().WeakRuntime;
	NodeRuntime.WeakOuter = CallerNode.GetNodeRuntime().WeakOuter;
	NodeRuntime.WeakGraph = CallerNode.GetNodeRuntime().WeakGraph;
	NodeRuntime.SourceNode = CallerNode.GetNodeRuntime().SourceNode;

	NodeRuntime.PinOverrides.Reset();
	NodeRuntime.PinArrayOverrides.Reset();

	for (const FVoxelPin& Pin : Node->GetPins())
	{
		if (!Pin.bIsInput ||
			!Pin.ArrayOwner.IsNone())
		{
			continue;
		}

		NodeRuntime.PinOverrides.Add(Pin.Name);
	}
	for (const auto& It : NodeRuntime.PinArrays)
	{
		NodeRuntime.PinArrayOverrides.Add(It.Key);
	}

	FBindings Bindings(NodeRuntime);
	Bind(Bindings, *Node);

	for (auto& It : NodeRuntime.PinOverrides)
	{
		if (!ensure(It.Value))
		{
			It.Value = [Type = NodeRuntime.PinDatas[It.Key]->Type](const FVoxelQuery&) -> FVoxelFutureValue
			{
				return FVoxelSharedPinValue(Type.MakeSafeDefault());
			};
		}
	}
	for (auto& It : NodeRuntime.PinArrayOverrides)
	{
		if (!ensure(It.Value))
		{
			It.Value = [](const FVoxelQuery&) -> TArray<FVoxelFutureValue>
			{
				return {};
			};
		}
	}

	const FVoxelNodeRuntime::FPinData& PinData = NodeRuntime.GetPinData(OutputPin);
	check(!PinData.bIsInput);

	const TSharedRef<const FVoxelNodeRuntime::FComputeState> ComputeState = PinData.ComputeState.ToSharedRef();

	FVoxelQuery LocalQuery = Query;
	LocalQuery.Callstack.Add(Node.Get());

	const FVoxelFutureValue Result = FVoxelTask::New(
		MakeShared<FVoxelTaskStat>(*Node),
		ComputeState->Type,
		ComputeState->StatName,
		EVoxelTaskThread::AnyThread,
		{},
		[ComputeState, LocalQuery = MoveTemp(LocalQuery)]
		{
			return ComputeState->Compute(LocalQuery);
		});

	return FVoxelTask::New(
		MakeShared<FVoxelTaskStat>(*Node),
		ComputeState->Type,
		STATIC_FNAME("Return node to pool"),
		EVoxelTaskThread::AnyThread,
		{ Result },
		[Result, Node]
		{
			Node->ReturnToPool();

			VOXEL_SCOPE_LOCK(CriticalSection);
			StructToPool.FindOrAdd(Node->GetStruct()).Add(Node.ToSharedRef());

			return Result;
		});
}