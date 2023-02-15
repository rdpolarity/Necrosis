#include "VoxelMetaGraphParameterNodeBase.h"
#include "VoxelBuffer.h"
#include "VoxelNodeLibrary.h"
#include "VoxelMetaGraphSchema.h"

FVoxelMetaGraphParameter* UVoxelMetaGraphParameterNodeBase::GetParameter() const
{
	UVoxelMetaGraph* MetaGraph = GetTypedOuter<UVoxelMetaGraph>();
	if (!ensure(MetaGraph))
	{
		return nullptr;
	}

	return MetaGraph->FindParameterByGuid(Guid);
}

void UVoxelMetaGraphParameterNodeBase::AllocateDefaultPins()
{
	if (const FVoxelMetaGraphParameter* Parameter = GetParameter())
	{
		AllocateParameterPins(*Parameter);
		CachedParameter = *Parameter;
	}
	else
	{
		AllocateParameterPins(CachedParameter);
		for (UEdGraphPin* Pin : Pins)
		{
			Pin->bOrphanedPin = true;
		}
	}

	Super::AllocateDefaultPins();
}

bool UVoxelMetaGraphParameterNodeBase::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const
{
	return Schema->IsA<UVoxelMetaGraphSchema>();
}

void UVoxelMetaGraphParameterNodeBase::PrepareForCopying()
{
	Super::PrepareForCopying();

	const FVoxelMetaGraphParameter* Parameter = GetParameter();
	if (!ensure(Parameter))
	{
		return;
	}

	CachedParameter = *Parameter;
}

bool UVoxelMetaGraphParameterNodeBase::CanSplitPin(const UEdGraphPin& Pin) const
{
	if (Pin.ParentPin ||
		Pin.bOrphanedPin ||
		Pin.LinkedTo.Num() > 0 ||
		!ensure(Pin.SubPins.Num() == 0))
	{
		return false;
	}
	
	return
		FVoxelNodeLibrary::FindMakeNode(Pin.PinType) &&
		FVoxelNodeLibrary::FindBreakNode(Pin.PinType);
}

void UVoxelMetaGraphParameterNodeBase::SplitPin(UEdGraphPin& Pin)
{
	Modify();

	UScriptStruct* NodeStruct = Pin.Direction == EGPD_Input
		? FVoxelNodeLibrary::FindMakeNode(Pin.PinType)
		: FVoxelNodeLibrary::FindBreakNode(Pin.PinType);

	if (!ensure(NodeStruct))
	{
		return;
	}

	TVoxelInstancedStruct<FVoxelNode> Node(NodeStruct);
	{
		FVoxelPin& NodePin =
			Pin.Direction == EGPD_Input
			? Node->GetUniqueOutputPin()
			: Node->GetUniqueInputPin();

		if (NodePin.IsPromotable() &&
			ensure(Node->GetPromotionTypes(NodePin).Contains(Pin.PinType)))
		{
			// Promote so the type are correct - eg if we are an array pin, the split pin should be array too
			Node->PromotePin(NodePin, Pin.PinType);
		}
	}

	TArray<const FVoxelPin*> NewPins;
	for (const FVoxelPin& NewPin : Node->GetPins())
	{
		if (Pin.Direction == EGPD_Input && !NewPin.bIsInput)
		{
			continue;
		}
		if (Pin.Direction == EGPD_Output && NewPin.bIsInput)
		{
			continue;
		}

		NewPins.Add(&NewPin);
	}

	Pin.bHidden = true;

	for (const FVoxelPin* NewPin : NewPins)
	{
		UEdGraphPin* SubPin = CreatePin(
			NewPin->bIsInput ? EGPD_Input : EGPD_Output,
			NewPin->GetType().GetEdGraphPinType(),
			NewPin->Name,
			Pins.IndexOfByKey(&Pin));

		FString Name = Pin.GetDisplayName().ToString();
		if (IsCompact())
		{
			Name.Reset();
		}
		if (!Name.IsEmpty())
		{
			Name += " ";
		}
		Name += NewPin->Metadata.DisplayName;

		SubPin->PinFriendlyName = FText::FromString(Name);
		SubPin->PinToolTip = Name + "\n" + Pin.PinToolTip;

		SubPin->ParentPin = &Pin;
		Pin.SubPins.Add(SubPin);
	}
	
	RefreshNode();
}

const FVoxelMetaGraphParameter& UVoxelMetaGraphParameterNodeBase::GetParameterSafe() const
{
	if (FVoxelMetaGraphParameter* Parameter = GetParameter())
	{
		return *Parameter;
	}

	return CachedParameter;
}