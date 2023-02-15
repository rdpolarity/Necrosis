// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphStructNode.h"
#include "VoxelMetaGraphSeed.h"
#include "VoxelMetaGraphSchema.h"
#include "VoxelNodeLibrary.h"
#include "EdGraphSchema_K2.h"
#include "GraphEditorSettings.h"

void UVoxelMetaGraphStructNode::AllocateDefaultPins()
{
	if (!Struct.IsValid())
	{
		return;
	}

	CachedName = Struct->GetDisplayName();

	for (const FVoxelPin& Pin : Struct->GetPins())
	{
		UEdGraphPin* GraphPin = CreatePin(
			Pin.bIsInput ? EGPD_Input : EGPD_Output,
			Pin.GetType().GetEdGraphPinType(),
			Pin.Name);

		GraphPin->PinFriendlyName = FText::FromString(Pin.Metadata.DisplayName);
		if (GraphPin->PinFriendlyName.IsEmpty())
		{
			GraphPin->PinFriendlyName = VOXEL_LOCTEXT(" ");
		}

		GraphPin->PinToolTip = Pin.Metadata.Tooltip + "\n\nType: ";
		GraphPin->PinToolTip += FVoxelPinType(GraphPin->PinType).ToString();

		GraphPin->bHidden = Struct->IsPinHidden(Pin);

		InitializeDefaultValue(Pin, *GraphPin);
	}

	// If we only have a single pin hide its name
	if (Pins.Num() == 1 &&
		Pins[0]->Direction == EGPD_Output)
	{
		Pins[0]->PinFriendlyName = VOXEL_LOCTEXT(" ");
	}

	Super::AllocateDefaultPins();
}

void UVoxelMetaGraphStructNode::PostPasteNode()
{
	Super::PostPasteNode();

	if (!Struct.IsValid())
	{
		return;
	}

	Struct->PostSerialize();
}

FText UVoxelMetaGraphStructNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (!Struct.IsValid())
	{
		return FText::FromString(CachedName);
	}

	const FText CompactTitle = Struct.GetScriptStruct()->GetMetaDataText(FBlueprintMetadata::MD_CompactNodeTitle);
	if (!CompactTitle.IsEmpty())
	{
		return CompactTitle;
	}

	return FText::FromString(Struct->GetDisplayName());
}

FLinearColor UVoxelMetaGraphStructNode::GetNodeTitleColor() const
{
	if (Struct.IsValid() &&
		Struct.GetScriptStruct()->HasMetaDataHierarchical(STATIC_FNAME("NodeColor")))
	{
		return GetNodeColor(GetStringMetaDataHierarchical(Struct.GetScriptStruct(), STATIC_FNAME("NodeColor")));
	}

	if (Struct.IsValid() &&
		Struct->IsPureNode())
	{
		return GetDefault<UGraphEditorSettings>()->PureFunctionCallNodeTitleColor;
	}

	return GetDefault<UGraphEditorSettings>()->FunctionCallNodeTitleColor;
}

FText UVoxelMetaGraphStructNode::GetTooltipText() const
{
	if (!Struct.IsValid())
	{
		return {};
	}

	return FText::FromString(Struct->GetTooltip());
}

FSlateIcon UVoxelMetaGraphStructNode::GetIconAndTint(FLinearColor& OutColor) const
{
	FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
	if (Struct.IsValid() &&
		Struct.GetScriptStruct()->HasMetaDataHierarchical("NodeIcon"))
	{
		Icon = GetNodeIcon(GetStringMetaDataHierarchical(Struct.GetScriptStruct(), "NodeIcon"));
	}

	if (Struct.IsValid() &&
		Struct.GetScriptStruct()->HasMetaDataHierarchical("NodeIconColor"))
	{
		OutColor = GetNodeColor(GetStringMetaDataHierarchical(Struct.GetScriptStruct(), "NodeIconColor"));
		return Icon;
	}

	if (Struct.IsValid() &&
		Struct->IsPureNode())
	{
		OutColor = GetDefault<UGraphEditorSettings>()->PureFunctionCallNodeTitleColor;
		return Icon;
	}

	OutColor = GetDefault<UGraphEditorSettings>()->FunctionCallNodeTitleColor;
	return Icon;
}

bool UVoxelMetaGraphStructNode::IsCompact() const
{
	if (!Struct.IsValid())
	{
		return {};
	}

	return Struct.GetScriptStruct()->HasMetaData(FBlueprintMetadata::MD_CompactNodeTitle);
}

bool UVoxelMetaGraphStructNode::GetOverlayInfo(FString& Type, FString& Tooltip, FString& Color)
{
	if (!Struct.IsValid())
	{
		return false;
	}

	const UScriptStruct* ScriptStruct = Struct.GetScriptStruct();
	if (!ScriptStruct->HasMetaDataHierarchical("OverlayTooltip") &&
		!ScriptStruct->HasMetaDataHierarchical("OverlayType") &&
		!ScriptStruct->HasMetaDataHierarchical("OverlayColor"))
	{
		return false;
	}

	if (ScriptStruct->HasMetaDataHierarchical("OverlayTooltip"))
	{
		Tooltip = GetStringMetaDataHierarchical(ScriptStruct, "OverlayTooltip");
	}

	if (ScriptStruct->HasMetaDataHierarchical("OverlayType"))
	{
		Type = GetStringMetaDataHierarchical(ScriptStruct, "OverlayType");
	}
	else
	{
		Type = "Warning";
	}

	if (ScriptStruct->HasMetaDataHierarchical("OverlayColor"))
	{
		Color = GetStringMetaDataHierarchical(ScriptStruct, "OverlayColor");
	}

	return true;
}

bool UVoxelMetaGraphStructNode::ShowAsPromotableWildcard(const UEdGraphPin& Pin) const
{
	if (Pin.ParentPin ||
		Pin.bOrphanedPin)
	{
		return false;
	}

	const TSharedPtr<const FVoxelPin> StructPin = Struct->FindPin(Pin.PinName);
	if (!ensure(StructPin))
	{
		return false;
	}

	if (!StructPin->IsPromotable() ||
		!Struct->ShowPromotablePinsAsWildcards())
	{
		return false;
	}

	return true;
}

FName UVoxelMetaGraphStructNode::GetPinCategory(const UEdGraphPin& Pin) const
{
	const TSharedPtr<const FVoxelPin> StructPin = Struct->FindPin(Pin.PinName);
	if (!ensure(StructPin))
	{
		return {};
	}

	return *StructPin->Metadata.Category;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<IVoxelNodeDefinition> UVoxelMetaGraphStructNode::GetNodeDefinition()
{
	if (Struct)
	{
		return MakeShared<FVoxelMetaGraphStructNodeDefinition>(*this, Struct->GetNodeDefinition());
	}

	ensure(false);
	return MakeShared<IVoxelNodeDefinition>();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool UVoxelMetaGraphStructNode::CanRemovePin_ContextMenu(const UEdGraphPin& Pin) const
{
	if (!Struct.IsValid() ||
		Pin.ParentPin ||
		Pin.bOrphanedPin)
	{
		return false;
	}

	return VOXEL_CONST_CAST(this)->GetNodeDefinition()->CanRemoveSelectedPin(Pin.PinName);
}

void UVoxelMetaGraphStructNode::RemovePin_ContextMenu(UEdGraphPin& Pin)
{
	GetNodeDefinition()->RemoveSelectedPin(Pin.PinName);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool UVoxelMetaGraphStructNode::CanPromotePin(const UEdGraphPin& Pin, FVoxelPinTypeSet& OutTypes) const
{
	if (Pin.ParentPin ||
		Pin.bOrphanedPin)
	{
		return false;
	}

	const TSharedPtr<const FVoxelPin> VoxelPin = Struct->FindPin(Pin.PinName);
	if (!ensure(VoxelPin) ||
		!VoxelPin->IsPromotable())
	{
		return false;
	}

	OutTypes = Struct->GetPromotionTypes(*VoxelPin);
	return true;
}

void UVoxelMetaGraphStructNode::PromotePin(UEdGraphPin& Pin, const FVoxelPinType& NewType)
{
	Modify();
	
	const TSharedPtr<FVoxelPin> VoxelPin = Struct->FindPin(Pin.PinName);
	if (!ensure(VoxelPin) ||
		!ensure(VoxelPin->IsPromotable()))
	{
		return;
	}

	Struct->PromotePin(*VoxelPin, NewType);

	ReconstructFromVoxelNode();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool UVoxelMetaGraphStructNode::CanSplitPin(const UEdGraphPin& Pin) const
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

void UVoxelMetaGraphStructNode::SplitPin(UEdGraphPin& Pin)
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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool UVoxelMetaGraphStructNode::TryMigratePin(UEdGraphPin* OldPin, UEdGraphPin* NewPin) const
{
	if (!ensure(Struct))
	{
		return false;
	}

	if (Super::TryMigratePin(OldPin, NewPin))
	{
		if (const TSharedPtr<const FVoxelPin> VoxelPin = Struct->FindPin(NewPin->PinName))
		{
			if (VoxelPin->Metadata.bPropertyBind)
			{
				const FVoxelPinValue DefaultValue = Struct->GetPinDefaultValue(*VoxelPin.Get());
				DefaultValue.ApplyToPinDefaultValue(*NewPin);
			}
		}

		return true;
	}

	return false;
}

bool UVoxelMetaGraphStructNode::TryMigrateDefaultValue(const UEdGraphPin* OldPin, UEdGraphPin* NewPin) const
{
	if (const TSharedPtr<const FVoxelPin> VoxelPin = Struct->FindPin(NewPin->PinName))
	{
		if (VoxelPin->Metadata.bPropertyBind)
		{
			return false;
		}
	}

	return Super::TryMigrateDefaultValue(OldPin, NewPin);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelMetaGraphStructNode::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	if (const TSharedPtr<const FVoxelPin> VoxelPin = Struct->FindPin(Pin->PinName))
	{
		if (VoxelPin->Metadata.bPropertyBind)
		{
			const FVoxelPinValue DefaultValue = FVoxelPinValue::MakeFromPinDefaultValue(*Pin);
			Struct->UpdatePropertyBoundDefaultValue(*VoxelPin.Get(), DefaultValue);
		}
	}

	Super::PinDefaultValueChanged(Pin);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool UVoxelMetaGraphStructNode::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const
{
	return Schema->IsA<UVoxelMetaGraphSchema>();
}

void UVoxelMetaGraphStructNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (const FProperty* Property = PropertyChangedEvent.MemberProperty)
	{
		if (Property->GetFName() == GET_MEMBER_NAME_STATIC(UVoxelMetaGraphStructNode, Struct))
		{
			ReconstructFromVoxelNode();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelMetaGraphStructNode::ReconstructFromVoxelNode()
{
	ReconstructNode(false);
}

void UVoxelMetaGraphStructNode::InitializeDefaultValue(const FVoxelPin& VoxelPin, UEdGraphPin& GraphPin)
{
	const FVoxelPinType ExposedType = FVoxelPinType(GraphPin.PinType).GetExposedType();

	FVoxelPinValue DefaultValue = Struct->GetPinDefaultValue(VoxelPin);

	if (ExposedType.IsWildcard() || ExposedType.IsObject())
	{
		if (ensure(!DefaultValue.IsValid()))
		{
			GraphPin.ResetDefaultValue();
			GraphPin.AutogeneratedDefaultValue = {};
			return;
		}
	}

	if (!DefaultValue.IsValid())
	{
		DefaultValue = FVoxelPinValue(ExposedType);
	}
	if (DefaultValue.Is<FVoxelMetaGraphSeed>())
	{
		DefaultValue.Get<FVoxelMetaGraphSeed>().Randomize();
	}
	DefaultValue.ApplyToPinDefaultValue(GraphPin);
	Struct->UpdatePropertyBoundDefaultValue(VoxelPin, DefaultValue);

	if (DefaultValue.Is<FVoxelMetaGraphSeed>())
	{
		GraphPin.AutogeneratedDefaultValue = {};
	}
	else
	{
		GraphPin.AutogeneratedDefaultValue = GraphPin.DefaultValue;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<const IVoxelNodeDefinition::FNode> FVoxelMetaGraphStructNodeDefinition::GetInputs() const
{
	return NodeDefinition->GetInputs();
}

TSharedPtr<const IVoxelNodeDefinition::FNode> FVoxelMetaGraphStructNodeDefinition::GetOutputs() const
{
	return NodeDefinition->GetOutputs();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString FVoxelMetaGraphStructNodeDefinition::GetAddPinLabel() const
{
	return NodeDefinition->GetAddPinLabel();
}

FString FVoxelMetaGraphStructNodeDefinition::GetAddPinTooltip() const
{
	return NodeDefinition->GetAddPinTooltip();
}

FString FVoxelMetaGraphStructNodeDefinition::GetRemovePinTooltip() const
{
	return NodeDefinition->GetRemovePinTooltip();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelMetaGraphStructNodeDefinition::CanAddInputPin() const
{
	return NodeDefinition->CanAddInputPin();
}

void FVoxelMetaGraphStructNodeDefinition::AddInputPin()
{
	FScope Scope(this, "Add Input Pin");
	NodeDefinition->AddInputPin();
}

bool FVoxelMetaGraphStructNodeDefinition::CanRemoveInputPin() const
{
	return NodeDefinition->CanRemoveInputPin();
}

void FVoxelMetaGraphStructNodeDefinition::RemoveInputPin()
{
	FScope Scope(this, "Remove Input Pin");
	NodeDefinition->RemoveInputPin();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelMetaGraphStructNodeDefinition::CanAddToCategory(FName Category) const
{
	return NodeDefinition->CanAddToCategory(Category);
}

void FVoxelMetaGraphStructNodeDefinition::AddToCategory(FName Category)
{
	FScope Scope(this, "Add To Category");
	NodeDefinition->AddToCategory(Category);
}

bool FVoxelMetaGraphStructNodeDefinition::CanRemoveFromCategory(FName Category) const
{
	return NodeDefinition->CanRemoveFromCategory(Category);
}

void FVoxelMetaGraphStructNodeDefinition::RemoveFromCategory(FName Category)
{
	FScope Scope(this, "Remove From Category");
	NodeDefinition->RemoveFromCategory(Category);
}

bool FVoxelMetaGraphStructNodeDefinition::CanRemoveSelectedPin(FName PinName) const
{
	return NodeDefinition->CanRemoveSelectedPin(PinName);
}

void FVoxelMetaGraphStructNodeDefinition::RemoveSelectedPin(FName PinName)
{
	FScope Scope(this, "Remove " + PinName.ToString() + " Pin");
	NodeDefinition->RemoveSelectedPin(PinName);
}

void FVoxelMetaGraphStructNodeDefinition::InsertPinBefore(FName PinName)
{
	FScope Scope(this, "Insert Pin Before " + PinName.ToString());
	NodeDefinition->InsertPinBefore(PinName);
}

void FVoxelMetaGraphStructNodeDefinition::DuplicatePin(FName PinName)
{
	FScope Scope(this, "Duplicate Pin " + PinName.ToString());
	NodeDefinition->DuplicatePin(PinName);
}

FString FVoxelMetaGraphStructNodeDefinition::GetPinTooltip(FName PinName) const
{
	return NodeDefinition->GetPinTooltip(PinName);
}

FString FVoxelMetaGraphStructNodeDefinition::GetCategoryTooltip(FName CategoryName) const
{
	return NodeDefinition->GetCategoryTooltip(CategoryName);
}