// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelGraphNode.h"
#include "VoxelGraphSchema.h"
#include "VoxelGraphEditorToolkit.h"
#include "VoxelEdGraph.h"

#include "GraphEditor.h"
#include "Toolkits/ToolkitManager.h"
#include "Subsystems/AssetEditorSubsystem.h"

VOXEL_RUN_ON_STARTUP_EDITOR(RegisterOnFocusNode)
{
	FVoxelMessages::OnFocusNode.Add(MakeLambdaDelegate([](const UEdGraphNode& Node)
	{
		if (Node.IsA<UVoxelGraphNode>())
		{
			UVoxelGraphNode::FocusOnNode(VOXEL_CONST_CAST(&Node));
		}
	}));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TArray<UEdGraphPin*> UVoxelGraphNode::GetInputPins() const
{
	return Pins.FilterByPredicate([&](const UEdGraphPin* Pin) { return Pin->Direction == EGPD_Input; });
}

TArray<UEdGraphPin*> UVoxelGraphNode::GetOutputPins() const
{
	return Pins.FilterByPredicate([&](const UEdGraphPin* Pin) { return Pin->Direction == EGPD_Output; });
}

UEdGraphPin* UVoxelGraphNode::GetInputPin(int32 Index) const
{
	return GetInputPins()[Index];
}

UEdGraphPin* UVoxelGraphNode::GetOutputPin(int32 Index) const
{
	return GetOutputPins()[Index];
}

UEdGraphPin* UVoxelGraphNode::FindPinByPredicate_Unique(TFunctionRef<bool(UEdGraphPin* Pin)> Function) const
{
	UEdGraphPin* FoundPin = nullptr;
	for (UEdGraphPin* Pin : Pins)
	{
		if (!Function(Pin))
		{
			continue;
		}

		ensure(!FoundPin);
		FoundPin = Pin;
	}

	return FoundPin;
}

const UVoxelGraphSchema* UVoxelGraphNode::GetSchema() const
{
	return CastChecked<const UVoxelGraphSchema>(Super::GetSchema());
}

TSharedPtr<FVoxelGraphEditorToolkit> UVoxelGraphNode::GetToolkit() const
{
	return CastChecked<UVoxelEdGraph>(GetGraph())->GetGraphToolkit();
}

void UVoxelGraphNode::RefreshNode()
{
	const TSharedPtr<FVoxelGraphEditorToolkit> Toolkit = GetToolkit();
	if (!Toolkit)
	{
		ensure(GetGraph() && GetGraph()->HasAllFlags(RF_Transient));
		return;
	}

	const TArray<TSharedPtr<SGraphEditor>> GraphEditors = Toolkit->GetGraphEditors();
	for (const TSharedPtr<SGraphEditor>& GraphEditor : GraphEditors)
	{
		if (GraphEditor->GetCurrentGraph() != GetGraph())
		{
			continue;
		}

		GraphEditor->RefreshNode(*this);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool UVoxelGraphNode::CanRecombinePin(const UEdGraphPin& Pin) const
{
	if (!Pin.ParentPin)
	{
		return false;
	}

	ensure(Pin.ParentPin->bHidden);
	ensure(Pin.ParentPin->LinkedTo.Num() == 0);
	ensure(Pin.ParentPin->SubPins.Contains(&Pin));

	return true;
}

void UVoxelGraphNode::RecombinePin(UEdGraphPin& Pin)
{
	Modify();

	UEdGraphPin* ParentPin = Pin.ParentPin;
	check(ParentPin);

	ensure(ParentPin->bHidden);
	ParentPin->bHidden = false;

	const TArray<UEdGraphPin*> SubPins = ParentPin->SubPins;
	ensure(SubPins.Num() > 0);

	for (UEdGraphPin* SubPin : SubPins)
	{
		SubPin->MarkAsGarbage();
		ensure(Pins.Remove(SubPin) == 1);
	}

	ensure(ParentPin->SubPins.Num() == 0);

	RefreshNode();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraphNode::FocusOnNode(UEdGraphNode* Node)
{
	FocusOnNodes({ Node });
}

void UVoxelGraphNode::FocusOnNodes(const TArray<UEdGraphNode*>& Nodes)
{
	if (!ensure(Nodes.Num() > 0))
	{
		return;
	}

	const UEdGraphNode* FirstNode = Nodes[0];
	if (!ensure(FirstNode) ||
		!ensure(FirstNode->IsA<UVoxelGraphNode>()) ||
		!ensure(FirstNode->GetGraph()))
	{
		return;
	}

	UObject* Asset = FirstNode->GetGraph()->GetOuter();
	if (!ensure(Asset))
	{
		return;
	}

	for (const UEdGraphNode* Node : Nodes)
	{
		if (!ensure(Node) ||
			!ensure(Node->GetGraph()) ||
			!ensure(Node->GetGraph()->GetOuter() == Asset))
		{
			return;
		}
	}

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Asset);

	const TSharedPtr<IToolkit> FoundAssetEditor = FToolkitManager::Get().FindEditorForAsset(Asset);
	if (!ensure(FoundAssetEditor))
	{
		return;
	}

	FoundAssetEditor->BringToolkitToFront();

	const FVoxelGraphEditorToolkit& Toolkit = static_cast<FVoxelGraphEditorToolkit&>(*FoundAssetEditor);

	const TArray<TSharedPtr<SGraphEditor>> GraphEditors = Toolkit.GetGraphEditors();
	for (const TSharedPtr<SGraphEditor> GraphEditor : GraphEditors)
	{
		if (GraphEditor->GetCurrentGraph() != FirstNode->GetGraph())
		{
			continue;
		}

		GraphEditor->ClearSelectionSet();
		for (UEdGraphNode* Node : Nodes)
		{
			GraphEditor->SetNodeSelection(Node, true);
		}
		GraphEditor->ZoomToFit(true);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraphNode::ReconstructNode(bool bCreateOrphans)
{
	VOXEL_FUNCTION_COUNTER();

	TUniquePtr<TGuardValue<bool>> Guard;
	if (const TSharedPtr<FVoxelGraphEditorToolkit> Toolkit = GetToolkit())
	{
		Guard = MakeUnique<TGuardValue<bool>>(Toolkit->bDisableOnGraphChanged, true);
	}
	(void)Guard;

	Modify();

	// Sanitize links
	for (UEdGraphPin* Pin : Pins)
	{
		Pin->LinkedTo.Remove(nullptr);

		for (UEdGraphPin* OtherPin : Pin->LinkedTo)
		{
			if (!OtherPin->GetOwningNode()->Pins.Contains(OtherPin))
			{
				Pin->LinkedTo.Remove(OtherPin);
			}
		}
	}
	
	// Move the existing pins to a saved array
	const TArray<UEdGraphPin*> OldPins = Pins;
	Pins.Reset();

	// Recreate the new pins
	AllocateDefaultPins();

	// Split new pins
	for (const UEdGraphPin* OldPin : OldPins)
	{
		if (OldPin->SubPins.Num() == 0)
		{
			continue;
		}
		
		UEdGraphPin* NewPin = FindPinByPredicate_Unique([&](const UEdGraphPin* Pin)
		{
			return
				Pin->PinName == OldPin->PinName &&
				Pin->PinType == OldPin->PinType &&
				Pin->Direction == OldPin->Direction;
		});
		if (!NewPin)
		{
			continue;
		}

		SplitPin(*NewPin);
	}

	TMap<UEdGraphPin*, UEdGraphPin*> NewPinsToOldPins;
	TMap<UEdGraphPin*, UEdGraphPin*> OldPinsToNewPins;

	// Map by name
	for (UEdGraphPin* OldPin : OldPins)
	{
		UEdGraphPin* NewPin = FindPinByPredicate_Unique([&](const UEdGraphPin* Pin)
		{
			if (Pin->Direction != OldPin->Direction ||
				Pin->PinName != OldPin->PinName)
			{
				return false;
			}
			
			const bool bHasParent = Pin->ParentPin || OldPin->ParentPin;
			if (bHasParent && (!Pin->ParentPin || !OldPin->ParentPin))
			{
				return false;
			}

			if (bHasParent &&
				Pin->ParentPin->PinName != OldPin->ParentPin->PinName)
			{
				return false;
			}

			return true;
		});

		if (!NewPin)
		{
			continue;
		}

		ensure(!OldPinsToNewPins.Contains(OldPin));
		ensure(!NewPinsToOldPins.Contains(NewPin));

		OldPinsToNewPins.Add(OldPin, NewPin);
		NewPinsToOldPins.Add(NewPin, OldPin);
	}

	if (!bCreateOrphans)
	{
		// Map by index if we're not creating orphans
		for (int32 Index = 0; Index < OldPins.Num(); Index++)
		{
			UEdGraphPin* OldPin = OldPins[Index];
			if (OldPinsToNewPins.Contains(OldPin) ||
				!Pins.IsValidIndex(Index))
			{
				continue;
			}

			UEdGraphPin* NewPin = Pins[Index];
			if (NewPinsToOldPins.Contains(NewPin))
			{
				continue;
			}

			ensure(!OldPinsToNewPins.Contains(OldPin));
			ensure(!NewPinsToOldPins.Contains(NewPin));

			OldPinsToNewPins.Add(OldPin, NewPin);
			NewPinsToOldPins.Add(NewPin, OldPin);
		}
	}

	TSet<UEdGraphPin*> MigratedOldPins;
	for (const auto& It : OldPinsToNewPins)
	{
		UEdGraphPin* OldPin = It.Key;
		UEdGraphPin* NewPin = It.Value;

		ensure(!OldPin->bOrphanedPin);

		if (TryMigratePin(OldPin, NewPin))
		{
			ensure(OldPin->LinkedTo.Num() == 0);
			MigratedOldPins.Add(OldPin);
			continue;
		}

		if (bCreateOrphans)
		{
			continue;
		}

		// If we're not going to create an orphan try to keep the default value
		TryMigrateDefaultValue(OldPin, NewPin);
	}

	struct FConnectionToRestore
	{
		UEdGraphPin* Pin = nullptr;
		UEdGraphPin* LinkedTo = nullptr;
		UEdGraphPin* OrphanedPin = nullptr;
	};
	TArray<FConnectionToRestore> ConnectionsToRestore;

	// Throw away the original pins
	for (UEdGraphPin* OldPin : OldPins)
	{
		UEdGraphPin* OrphanedPin = nullptr;

		if ((bCreateOrphans || OldPin->bOrphanedPin) &&
			!MigratedOldPins.Contains(OldPin) &&
			(OldPin->LinkedTo.Num() > 0 || !OldPin->DoesDefaultValueMatchAutogenerated()))
		{
			FString NewName = OldPin->PinName.ToString();
			if (!NewName.StartsWith("ORPHANED_"))
			{
				// Avoid collisions
				NewName = "ORPHANED_" + NewName;
			}

			OrphanedPin = CreatePin(OldPin->Direction, OldPin->PinType, *NewName);
			ensure(TryMigratePin(OldPin, OrphanedPin));
			OrphanedPin->PinFriendlyName = OldPin->PinFriendlyName;
			OrphanedPin->bOrphanedPin = true;
		}

		if (UEdGraphPin* NewPin = OldPinsToNewPins.FindRef(OldPin))
		{
			for (UEdGraphPin* LinkedTo : OldPin->LinkedTo)
			{
				FConnectionToRestore ConnectionToRestore;
				ConnectionToRestore.Pin = NewPin;
				ConnectionToRestore.LinkedTo = LinkedTo;
				ConnectionToRestore.OrphanedPin = OrphanedPin;
				ConnectionsToRestore.Add(ConnectionToRestore);
			}
		}

		OldPin->Modify();
		DestroyPin(OldPin);
	}

	// Check bNotConnectable
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->bNotConnectable)
		{
			Pin->BreakAllPinLinks();
		}
	}

	PostReconstructNode();

	for (const FConnectionToRestore& Connection : ConnectionsToRestore)
	{
		// Never promote nor break connections
		if (GetSchema()->CanCreateConnection(Connection.Pin, Connection.LinkedTo).Response != CONNECT_RESPONSE_MAKE)
		{
			continue;
		}

		if (!ensure(GetSchema()->TryCreateConnection(Connection.Pin, Connection.LinkedTo)))
		{
			continue;
		}

		if (Connection.OrphanedPin)
		{
			Connection.OrphanedPin->BreakLinkTo(Connection.LinkedTo);
		}
	}

	RefreshNode();
}

void UVoxelGraphNode::AllocateDefaultPins()
{
	TSet<FName> Names;
	for (const UEdGraphPin* Pin : Pins)
	{
		ensure(!Names.Contains(Pin->PinName));
		Names.Add(Pin->PinName);
	}

	InputPinCategories = {};
	OutputPinCategories = {};

	TSet<FName> ValidCategories;
	for (const UEdGraphPin* Pin : Pins)
	{
		FName PinCategory = GetPinCategory(*Pin);
		ValidCategories.Add(PinCategory);

		switch (Pin->Direction)
		{
		default: check(false);
		case EGPD_Input: InputPinCategories.Add(PinCategory); break;
		case EGPD_Output: OutputPinCategories.Add(PinCategory); break;
		}
	}

	CollapsedInputCategories = CollapsedInputCategories.Intersect(ValidCategories);
	CollapsedOutputCategories = CollapsedOutputCategories.Intersect(ValidCategories);
}

void UVoxelGraphNode::ReconstructNode()
{
	ReconstructNode(true);
}

FLinearColor UVoxelGraphNode::GetNodeTitleColor() const
{
	return FLinearColor::Black;
}

void UVoxelGraphNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	if (!FromPin)
	{
		return;
	}

	const UEdGraphSchema* Schema = GetSchema();

	// Check non-promotable pins first
	for (UEdGraphPin* Pin : Pins)
	{
		const FPinConnectionResponse Response = Schema->CanCreateConnection(FromPin, Pin);

		if (Response.Response == CONNECT_RESPONSE_MAKE)
		{
			Schema->TryCreateConnection(FromPin, Pin);
			return;
		}
	}

	for (UEdGraphPin* Pin : Pins)
	{
		const FPinConnectionResponse Response = Schema->CanCreateConnection(FromPin, Pin);

		if (Response.Response == CONNECT_RESPONSE_MAKE_WITH_PROMOTION)
		{
			Schema->TryCreateConnection(FromPin, Pin);
			return;
		}
		else if (Response.Response == CONNECT_RESPONSE_BREAK_OTHERS_A)
		{
			// The pin we are creating from already has a connection that needs to be broken. We want to "insert" the new node in between, so that the output of the new node is hooked up too
			UEdGraphPin* OldLinkedPin = FromPin->LinkedTo[0];
			check(OldLinkedPin);

			FromPin->BreakAllPinLinks();

			// Hook up the old linked pin to the first valid output pin on the new node
			for (UEdGraphPin* InnerPin : Pins)
			{
				if (Schema->CanCreateConnection(OldLinkedPin, InnerPin).Response != CONNECT_RESPONSE_MAKE)
				{
					continue;
				}

				Schema->TryCreateConnection(OldLinkedPin, InnerPin);
				break;
			}

			Schema->TryCreateConnection(FromPin, Pin);
			return;
		}
	}
}

void UVoxelGraphNode::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);

	// If the default value is manually set then treat it as if the value was reset to default and remove the orphaned pin
	if (Pin->bOrphanedPin && Pin->DoesDefaultValueMatchAutogenerated())
	{
		PinConnectionListChanged(Pin);
	}

	UVoxelGraphSchema::OnGraphChanged(this);
}

void UVoxelGraphNode::PinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::PinConnectionListChanged(Pin);

	if (!Pin->bOrphanedPin ||
		Pin->LinkedTo.Num() > 0 ||
		!Pin->DoesDefaultValueMatchAutogenerated())
	{
		return;
	}

	// If we're not linked and this pin should no longer exist as part of the node, remove it

	RemovePin(Pin);
	RefreshNode();
}

void UVoxelGraphNode::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (!bDuplicateForPIE)
	{
		CreateNewGuid();
	}
}

void UVoxelGraphNode::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);

	DelayOnGraphChangedScopeStack.Add(MakeShared<FVoxelGraphDelayOnGraphChangedScope>());
}

void UVoxelGraphNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive)
	{
		return;
	}

	UVoxelGraphSchema::OnGraphChanged(this);

	if (ensure(DelayOnGraphChangedScopeStack.Num() > 0))
	{
		DelayOnGraphChangedScopeStack.Pop();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FSlateIcon UVoxelGraphNode::GetNodeIcon(const FString& IconName)
{
	static FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");

	if (IconName == "Loop")
	{
		return FSlateIcon("EditorStyle", "GraphEditor.Macro.Loop_16x");
	}
	else if (IconName == "Gate")
	{
		return FSlateIcon("EditorStyle", "GraphEditor.Macro.Gate_16x");
	}
	else if (IconName == "Do N")
	{
		return FSlateIcon("EditorStyle", "GraphEditor.Macro.DoN_16x");
	}
	else if (IconName == "Do Once")
	{
		return FSlateIcon("EditorStyle", "GraphEditor.Macro.DoOnce_16x");
	}
	else if (IconName == "IsValid")
	{
		return FSlateIcon("EditorStyle", "GraphEditor.Macro.IsValid_16x");
	}
	else if (IconName == "FlipFlop")
	{
		return FSlateIcon("EditorStyle", "GraphEditor.Macro.FlipFlop_16x");
	}
	else if (IconName == "ForEach")
	{
		return FSlateIcon("EditorStyle", "GraphEditor.Macro.ForEach_16x");
	}
	else if (IconName == "Event")
	{
		return FSlateIcon("EditorStyle", "GraphEditor.Event_16x");
	}
	else if (IconName == "Sequence")
	{
		return FSlateIcon("EditorStyle", "GraphEditor.Sequence_16x");
	}
	else if (IconName == "Cast")
	{
		return FSlateIcon("EditorStyle", "GraphEditor.Cast_16x");
	}
	else if (IconName == "Select")
	{
		return FSlateIcon("EditorStyle", "GraphEditor.Select_16x");
	}
	else if (IconName == "Switch")
	{
		return FSlateIcon("EditorStyle", "GraphEditor.Switch_16x");
	}

	return Icon;
}

FLinearColor UVoxelGraphNode::GetNodeColor(const FString& ColorName)
{
	if (ColorName == "LightBlue")
	{
		return FLinearColor(0.190525f, 0.583898f, 1.f);
	}
	else if (ColorName == "Blue")
	{
		return FLinearColor(0.f, 0.68359375f, 1.f);
	}
	else if (ColorName == "LightGreen")
	{
		return FLinearColor(0.4f, 0.85f, 0.35f);
	}
	else if (ColorName == "Green")
	{
		return FLinearColor(0.039216f, 0.666667f, 0.f);
	}
	else if (ColorName == "Red")
	{
		return FLinearColor(1.f, 0.f, 0.f);
	}
	else if (ColorName == "Orange")
	{
		return FLinearColor(1.f, 0.546875f, 0.f);
	}
	else if (ColorName == "White")
	{
		return FLinearColor::White;
	}
	
	return FLinearColor::Black;
}