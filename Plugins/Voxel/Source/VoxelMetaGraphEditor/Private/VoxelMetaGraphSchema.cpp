// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphSchema.h"
#include "VoxelMetaGraph.h"
#include "VoxelNode.h"
#include "VoxelExecNode.h"
#include "VoxelNodeLibrary.h"
#include "VoxelMetaGraphVisuals.h"
#include "VoxelMetaGraphToolkit.h"
#include "VoxelGraphEditorToolkit.h"
#include "Widgets/SVoxelMetaGraphMembers.h"
#include "Nodes/VoxelMetaGraphKnotNode.h"
#include "Nodes/VoxelMetaGraphMacroNode.h"
#include "Nodes/VoxelMetaGraphStructNode.h"
#include "Nodes/VoxelMetaGraphParameterNode.h"
#include "Nodes/Templates/VoxelOperatorNodes.h"
#include "Nodes/VoxelMetaGraphLocalVariableNode.h"
#include "Nodes/VoxelMetaGraphMacroParameterNode.h"
#include "Widgets/SVoxelMetaGraphPinTypeSelector.h"

#include "ToolMenu.h"
#include "GraphEditorActions.h"
#include "GraphEditorSettings.h"
#include "Styling/SlateIconFinder.h"
#include "AssetRegistry/AssetRegistryModule.h"

UEdGraphNode* FVoxelMetaGraphSchemaAction_NewMetaGraphNode::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	const FVoxelTransaction Transaction(ParentGraph, "New graph node");

	FGraphNodeCreator<UVoxelMetaGraphMacroNode> NodeCreator(*ParentGraph);
	UVoxelMetaGraphMacroNode* Node = NodeCreator.CreateUserInvokedNode(bSelectNewNode);
	Node->MetaGraph = MetaGraph;
	Node->NodePosX = Location.X;
	Node->NodePosY = Location.Y;
	NodeCreator.Finalize();
	
	if (FromPin)
	{
		Node->AutowireNewNode(FromPin);
	}

	return Node;
}

void FVoxelMetaGraphSchemaAction_NewMetaGraphNode::GetIcon(FSlateIcon& Icon, FLinearColor& Color)
{
	static const FSlateIcon MacroIcon("EditorStyle", "GraphEditor.Macro_16x");
	Icon = MacroIcon;
}

UEdGraphNode* FVoxelMetaGraphSchemaAction_NewParameterUsage::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	check(Guid.IsValid());
	const FVoxelTransaction Transaction(ParentGraph, "Create new parameter usage");

	UEdGraphNode* NewNode;
	if (ParameterType == EVoxelMetaGraphParameterType::Parameter)
	{
		FGraphNodeCreator<UVoxelMetaGraphParameterNode> NodeCreator(*ParentGraph);
		UVoxelMetaGraphParameterNode* Node = NodeCreator.CreateUserInvokedNode(bSelectNewNode);
		Node->Guid = Guid;
		Node->NodePosX = Location.X;
		Node->NodePosY = Location.Y;
		NodeCreator.Finalize();

		NewNode = Node;
	}
	else if (
		ParameterType == EVoxelMetaGraphParameterType::MacroInput ||
		ParameterType == EVoxelMetaGraphParameterType::MacroOutput)
	{
		FGraphNodeCreator<UVoxelMetaGraphMacroParameterNode> NodeCreator(*ParentGraph);
		UVoxelMetaGraphMacroParameterNode* Node = NodeCreator.CreateUserInvokedNode(bSelectNewNode);
		Node->Guid = Guid;
		Node->Type = ParameterType;
		Node->NodePosX = Location.X;
		Node->NodePosY = Location.Y;
		NodeCreator.Finalize();

		NewNode = Node;
	}
	else if (ParameterType == EVoxelMetaGraphParameterType::LocalVariable)
	{
		if (bDeclaration)
		{
			FGraphNodeCreator<UVoxelMetaGraphLocalVariableDeclarationNode> NodeCreator(*ParentGraph);
			UVoxelMetaGraphLocalVariableDeclarationNode* Node = NodeCreator.CreateUserInvokedNode(bSelectNewNode);
			Node->Guid = Guid;
			Node->NodePosX = Location.X;
			Node->NodePosY = Location.Y;
			NodeCreator.Finalize();

			NewNode = Node;
		}
		else
		{
			FGraphNodeCreator<UVoxelMetaGraphLocalVariableUsageNode> NodeCreator(*ParentGraph);
			UVoxelMetaGraphLocalVariableUsageNode* Node = NodeCreator.CreateUserInvokedNode(bSelectNewNode);
			Node->Guid = Guid;
			Node->NodePosX = Location.X;
			Node->NodePosY = Location.Y;
			NodeCreator.Finalize();

			NewNode = Node;
		}
	}
	else
	{
		ensure(false);
		return nullptr;
	}

	if (FromPin)
	{
		NewNode->AutowireNewNode(FromPin);
	}

	return NewNode;
}

void FVoxelMetaGraphSchemaAction_NewParameterUsage::GetIcon(FSlateIcon& Icon, FLinearColor& Color)
{
	Icon = FVoxelMetaGraphVisuals::GetPinIcon(PinType);
	Color = FVoxelMetaGraphVisuals::GetPinColor(PinType);
}

UEdGraphNode* FVoxelMetaGraphSchemaAction_NewParameter::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	VOXEL_USE_NAMESPACE(MetaGraph);

	const FVoxelTransaction Transaction(ParentGraph, "Create new parameter");

	const TSharedPtr<FVoxelMetaGraphEditorToolkit> Toolkit = UVoxelMetaGraphSchema::GetToolkit(ParentGraph);
	if (!ensure(Toolkit))
	{
		return nullptr;
	}

	UVoxelMetaGraph& MetaGraph = Toolkit->GetAssetAs<UVoxelMetaGraph>();

	FVoxelMetaGraphParameter NewParameter;
	NewParameter.Guid = FGuid::NewGuid();
	NewParameter.ParameterType = ParameterType;
	NewParameter.Category = ParameterCategory;
	
	if (FromPin)
	{
		const FString PinName = FromPin->GetDisplayName().ToString();
		if (!PinName.TrimStartAndEnd().IsEmpty())
		{
			NewParameter.Name = *PinName;
		}
		NewParameter.Type = FromPin->PinType;
		NewParameter.DefaultValue = FVoxelPinValue::MakeFromPinDefaultValue(*FromPin);
	}
	else
	{
		NewParameter.Type = FVoxelPinType::Make<float>();
		NewParameter.DefaultValue = FVoxelPinValue(NewParameter.Type.GetExposedType());
	}

	UVoxelGraphNode* ResultNode;
	{
		const FVoxelTransaction MetaGraphTransaction(MetaGraph);

		MetaGraph.Parameters.Add(NewParameter);

		if (ParameterType == EVoxelMetaGraphParameterType::Parameter)
		{
			FGraphNodeCreator<UVoxelMetaGraphParameterNode> NodeCreator(*ParentGraph);
			UVoxelMetaGraphParameterNode* Node = NodeCreator.CreateUserInvokedNode(bSelectNewNode);
			Node->Guid = NewParameter.Guid;
			Node->NodePosX = Location.X;
			Node->NodePosY = Location.Y;
			NodeCreator.Finalize();
			
			ResultNode = Node;
		}
		else if (
			ParameterType == EVoxelMetaGraphParameterType::MacroInput ||
			ParameterType == EVoxelMetaGraphParameterType::MacroOutput)
		{
			FGraphNodeCreator<UVoxelMetaGraphMacroParameterNode> NodeCreator(*ParentGraph);
			UVoxelMetaGraphMacroParameterNode* Node = NodeCreator.CreateUserInvokedNode(bSelectNewNode);
			Node->Guid = NewParameter.Guid;
			Node->Type = ParameterType;
			Node->NodePosX = Location.X;
			Node->NodePosY = Location.Y;
			NodeCreator.Finalize();
			
			ResultNode = Node;
		}
		else if (ParameterType == EVoxelMetaGraphParameterType::LocalVariable)
		{
			FGraphNodeCreator<UVoxelMetaGraphLocalVariableDeclarationNode> NodeCreator(*ParentGraph);
			UVoxelMetaGraphLocalVariableDeclarationNode* Node = NodeCreator.CreateUserInvokedNode(bSelectNewNode);
			Node->Guid = NewParameter.Guid;
			Node->NodePosX = Location.X;
			Node->NodePosY = Location.Y;
			NodeCreator.Finalize();
			
			ResultNode = Node;
		}
		else
		{
			ensure(false);
			return nullptr;
		}

		if (FromPin)
		{
			ResultNode->AutowireNewNode(FromPin);
			switch (ParameterType)
			{
			default: check(false);
			case EVoxelMetaGraphParameterType::Parameter: break;
			case EVoxelMetaGraphParameterType::MacroOutput: break;
			case EVoxelMetaGraphParameterType::MacroInput:
			case EVoxelMetaGraphParameterType::LocalVariable:
			{
				if (UEdGraphPin* InputPin = ResultNode->GetInputPin(0))
				{
					InputPin->DefaultObject = FromPin->DefaultObject;
					InputPin->DefaultValue = FromPin->DefaultValue;
					InputPin->DefaultTextValue = FromPin->DefaultTextValue;
				}
			}
			break;
			}
		}
	}

	const TSharedRef<SMembers> Members = Toolkit->GetGraphMembers();
	Members->Refresh();
	Members->SelectAndRequestRename(NewParameter.Guid, GetSection(ParameterType));

	return ResultNode;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UEdGraphNode* FVoxelMetaGraphSchemaAction_NewStructNode::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	const FVoxelTransaction Transaction(ParentGraph, "New graph node");

	FGraphNodeCreator<UVoxelMetaGraphStructNode> NodeCreator(*ParentGraph);
	UVoxelMetaGraphStructNode* Node = NodeCreator.CreateUserInvokedNode(bSelectNewNode);
	Node->Struct = Struct;
	Node->NodePosX = Location.X;
	Node->NodePosY = Location.Y;
	NodeCreator.Finalize();
	
	if (FromPin)
	{
		Node->AutowireNewNode(FromPin);
	}

	return Node;
}

void FVoxelMetaGraphSchemaAction_NewStructNode::GetIcon(FSlateIcon& Icon, FLinearColor& Color)
{
	static FSlateIcon StructIcon("EditorStyle", "Kismet.AllClasses.FunctionIcon");

	if (Struct &&
		Struct->HasMetaData("NodeIconColor"))
	{
		Color = UVoxelGraphNode::GetNodeColor(Struct->GetMetaData("NodeIconColor"));
	}
	else
	{
		Color = GetDefault<UGraphEditorSettings>()->FunctionCallNodeTitleColor;
	}

	if (Struct &&
		Struct->HasMetaData("NodeIcon"))
	{
		Icon = UVoxelGraphNode::GetNodeIcon(Struct->GetMetaData("NodeIcon"));
	}
	else
	{
		Icon = StructIcon;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UEdGraphNode* FVoxelMetaGraphSchemaAction_NewPromotableStructNode::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	const FVoxelTransaction Transaction(ParentGraph, "New graph node");

	FGraphNodeCreator<UVoxelMetaGraphStructNode> NodeCreator(*ParentGraph);
	UVoxelMetaGraphStructNode* Node = NodeCreator.CreateUserInvokedNode(bSelectNewNode);
	Node->Struct = Struct;
	Node->NodePosX = Location.X;
	Node->NodePosY = Location.Y;
	NodeCreator.Finalize();
	
	if (FromPin)
	{
		for (UEdGraphPin* InputPin : Node->GetInputPins())
		{
			if (PinTypes.Num() == 0)
			{
				break;
			}

			Node->PromotePin(*InputPin, PinTypes.Pop());
		}
		
		Node->AutowireNewNode(FromPin);
	}

	return Node;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UEdGraphNode* FVoxelMetaGraphSchemaAction_NewKnotNode::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	const FVoxelTransaction Transaction(ParentGraph, "New reroute node");

	FGraphNodeCreator<UVoxelMetaGraphKnotNode> NodeCreator(*ParentGraph);
	UVoxelMetaGraphKnotNode* Node = NodeCreator.CreateUserInvokedNode(bSelectNewNode);
	Node->NodePosX = Location.X;
	Node->NodePosY = Location.Y;
	NodeCreator.Finalize();

	if (FromPin)
	{
		Node->AutowireNewNode(FromPin);
	}

	Node->PropagatePinType();

	return Node;
}

void FVoxelMetaGraphSchemaAction_NewKnotNode::GetIcon(FSlateIcon& Icon, FLinearColor& Color)
{
	static const FSlateIcon KnotIcon = FSlateIcon("EditorStyle", "GraphEditor.Default_16x");
	Icon = KnotIcon;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<FEdGraphSchemaAction> UVoxelMetaGraphSchema::FindCastAction(const FEdGraphPinType& From, const FEdGraphPinType& To) const
{
	UScriptStruct* CastNode = FVoxelNodeLibrary::FindCastNode(From, To);
	if (!CastNode)
	{
		return nullptr;
	}

	const TSharedRef<FVoxelMetaGraphSchemaAction_NewStructNode> Action = MakeShared<FVoxelMetaGraphSchemaAction_NewStructNode>();
	Action->Struct = CastNode;
	return Action;
}

TOptional<FPinConnectionResponse> UVoxelMetaGraphSchema::GetCanCreateConnectionOverride(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const
{
	const UEdGraphPin* InputPin = nullptr;
	const UEdGraphPin* OutputPin = nullptr;
	if (!ensure(CategorizePinsByDirection(PinA, PinB, InputPin, OutputPin)))
	{
		return {};
	}

	if (FVoxelPinType(OutputPin->PinType).IsDerivedFrom(InputPin->PinType))
	{
		// Can connect directly
		return {};
	}

	if (FVoxelPinType(OutputPin->PinType).GetBufferType().IsDerivedFrom(InputPin->PinType))
	{
		// Can connect directly (uniform to buffer)
		return {};
	}

	const auto TryConnect = [this](const UEdGraphPin& PinToPromote, const UEdGraphPin& OtherPin)
	{
		FVoxelPinType Type;
		if (!TryGetPromotionType(PinToPromote, OtherPin.PinType, Type))
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, "");
		}

		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE_WITH_PROMOTION,
			FString::Printf(TEXT("Convert pin %s from %s to %s"),
				*PinToPromote.GetDisplayName().ToString(),
				*FVoxelPinType(PinToPromote.PinType).ToString(),
				*Type.ToString()));
	};

	{
		const FPinConnectionResponse Response = TryConnect(*PinB, *PinA);
		if (Response.Response != CONNECT_RESPONSE_DISALLOW)
		{
			return Response;
		}
	}

	if (CanCreateAutomaticConversionNode(InputPin, OutputPin))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE, FString::Printf(TEXT("Convert %s to %s"),
			*FVoxelPinType(OutputPin->PinType).ToString(),
			*FVoxelPinType(InputPin->PinType).ToString()));
	}

	{
		const FPinConnectionResponse Response = TryConnect(*PinA, *PinB);
		if (Response.Response != CONNECT_RESPONSE_DISALLOW)
		{
			return Response;
		}
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, FString::Printf(TEXT("Cannot convert %s to %s"),
		*FVoxelPinType(OutputPin->PinType).ToString(),
		*FVoxelPinType(InputPin->PinType).ToString()));
}

bool UVoxelMetaGraphSchema::CreatePromotedConnectionSafe(UEdGraphPin*& PinA, UEdGraphPin*& PinB) const
{
	const auto TryPromote = [&](UEdGraphPin& PinToPromote, const UEdGraphPin& OtherPin)
	{
		FVoxelPinType Type;
		if (!TryGetPromotionType(PinToPromote, OtherPin.PinType, Type))
		{
			return false;
		}

		UVoxelMetaGraphNode* Node = CastChecked<UVoxelMetaGraphNode>(PinToPromote.GetOwningNode());

		const FName PinAName = PinA->GetFName();
		const FName PinBName = PinB->GetFName();
		const UVoxelGraphNode* NodeA = CastChecked<UVoxelGraphNode>(PinA->GetOwningNode());
		const UVoxelGraphNode* NodeB = CastChecked<UVoxelGraphNode>(PinB->GetOwningNode());
		{
			// Tricky: PromotePin might reconstruct the node, invalidating pin pointers
			Node->PromotePin(PinToPromote, Type);
		}
		PinA = NodeA->FindPin(PinAName);
		PinB = NodeB->FindPin(PinBName);

		return true;
	};

	if (!TryPromote(*PinB, *PinA) &&
		!TryPromote(*PinA, *PinB))
	{
		return false;
	}

	if (!ensure(PinA) ||
		!ensure(PinB) ||
		!ensure(CanCreateConnection(PinA, PinB).Response != CONNECT_RESPONSE_MAKE_WITH_PROMOTION) ||
		!ensure(TryCreateConnection(PinA, PinB)))
	{
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelMetaGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	VOXEL_FUNCTION_COUNTER();

	Super::GetGraphContextActions(ContextMenuBuilder);
	
	ContextMenuBuilder.AddAction(MakeShared<FVoxelMetaGraphSchemaAction_NewKnotNode>(
		FText(),
		VOXEL_LOCTEXT("Add reroute node"),
		VOXEL_LOCTEXT("Create a reroute node"),
		0));

	const TSharedPtr<FVoxelMetaGraphEditorToolkit> Toolkit = GetToolkit(ContextMenuBuilder.CurrentGraph);
	if (!ensure(Toolkit))
	{
		return;
	}

	TArray<FAssetData> AssetDatas;
	FARFilter Filter;
#if VOXEL_ENGINE_VERSION < 501
	Filter.ClassNames.Add(GetClassFName<UVoxelMetaGraph>());
#else
	Filter.ClassPaths.Add(UVoxelMetaGraph::StaticClass()->GetClassPathName());
#endif

	const IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	AssetRegistry.GetAssets(Filter, AssetDatas);
	
	TArray<UVoxelMetaGraph*> MetaGraphs;
	for (const FAssetData& AssetData : AssetDatas)
	{
		UObject* Asset = AssetData.GetAsset();
		if (!ensure(Asset) ||
			!ensure(Asset->IsA<UVoxelMetaGraph>()))
		{
			continue;
		}

		UVoxelMetaGraph* MetaGraph = CastChecked<UVoxelMetaGraph>(Asset);

		if (!ensure(MetaGraph))
		{
			continue;
		}

		if (!MetaGraph->bIsMacroGraph)
		{
			continue;
		}

		MetaGraphs.Add(MetaGraph);
	}

	for (UVoxelMetaGraph* MetaGraph : MetaGraphs)
	{
		if (ContextMenuBuilder.FromPin)
		{
			bool bHasMatch = false;

			for (const FVoxelMetaGraphParameter& Parameter : MetaGraph->Parameters)
			{
				if (ContextMenuBuilder.FromPin->PinType == Parameter.Type &&
					(ContextMenuBuilder.FromPin->Direction == EGPD_Output) == (Parameter.ParameterType == EVoxelMetaGraphParameterType::MacroInput))
				{
					bHasMatch = true;
					break;
				}
			}

			if (!bHasMatch)
			{
				continue;
			}
		}

		const TSharedRef<FVoxelMetaGraphSchemaAction_NewMetaGraphNode> Action = MakeShared<FVoxelMetaGraphSchemaAction_NewMetaGraphNode>(
			FText::FromString(MetaGraph->Category),
			FText::FromString(MetaGraph->GetMacroName()),
			FText::FromString(MetaGraph->Tooltip),
			0);

		Action->MetaGraph = MetaGraph;

		ContextMenuBuilder.AddAction(Action);
	}
	
	for (auto& It : FVoxelNodeLibrary::GetNodes())
	{
		const TVoxelInstancedStruct<FVoxelNode>& Node = It.Value;

		if (Node->GetStruct()->HasMetaData("Internal"))
		{
			continue;
		}

		const auto HasPinMatch = [&](FVoxelPinTypeSet& OutPromotionTypes)
		{
			const UEdGraphPin& FromPin = *ContextMenuBuilder.FromPin;

			for (const FVoxelPin& Pin : Node->GetPins())
			{
				if (FromPin.Direction == (Pin.bIsInput ? EGPD_Input : EGPD_Output))
				{
					continue;
				}

				if (Pin.IsPromotable())
				{
					OutPromotionTypes = Node->GetPromotionTypes(Pin);
				}
				else
				{
					OutPromotionTypes.Add(Pin.GetType());
				}
				
				if (OutPromotionTypes.IsAll())
				{
					return true;
				}

				for (const FVoxelPinType& Type : OutPromotionTypes.GetTypes())
				{
					if (FromPin.Direction == EGPD_Input && Type.IsDerivedFrom(FVoxelPinType(FromPin.PinType)))
					{
						return true;
					}
					if (FromPin.Direction == EGPD_Output && FVoxelPinType(FromPin.PinType).IsDerivedFrom(Type))
					{
						return true;
					}
				}
			}

			return false;
		};

		FVoxelPinTypeSet PromotionTypes;
		if (ContextMenuBuilder.FromPin && !HasPinMatch(PromotionTypes))
		{
			continue;
		}

		FString Keywords;
		Node->GetStruct()->GetStringMetaDataHierarchical(STATIC_FNAME("Keywords"), &Keywords);
		Keywords += Node->GetStruct()->GetMetaData(STATIC_FNAME("CompactNodeTitle"));

		FString DisplayName = Node->GetDisplayName();
		Keywords += DisplayName;
		DisplayName.ReplaceInline(TEXT("\n"), TEXT(" "));

		if (ContextMenuBuilder.FromPin &&
			Node->GetStruct()->HasMetaData(STATIC_FNAME("Operator")))
		{
			FString Operator = Node->GetStruct()->GetMetaData(STATIC_FNAME("Operator"));

			TMap<FVoxelPinType, TSet<FVoxelPinType>> Permutations = CollectOperatorPermutations(Node, *ContextMenuBuilder.FromPin, PromotionTypes);
			for (const auto& PermutationIt : Permutations)
			{
				FVoxelPinType InnerType = PermutationIt.Key.GetInnerType();
				for (const FVoxelPinType& SecondType : PermutationIt.Value)
				{
					FVoxelPinType SecondInnerType = SecondType.GetInnerType();

					const TSharedRef<FVoxelMetaGraphSchemaAction_NewPromotableStructNode> Action = MakeShared<FVoxelMetaGraphSchemaAction_NewPromotableStructNode>(
						FText::FromString(Node->GetCategory()),
						FText::FromString(InnerType.ToString() + " " + Operator + " " + SecondInnerType.ToString()),
						FText::FromString(Node->GetTooltip()),
						0,
						FText::FromString(Keywords + " " + InnerType.ToString() + " " + SecondInnerType.ToString()));

					Action->PinTypes = {SecondType, PermutationIt.Key};
					Action->Struct = Node->GetStruct();

					ContextMenuBuilder.AddAction(Action);
				}
			}
		}
		else
		{
			const TSharedRef<FVoxelMetaGraphSchemaAction_NewStructNode> Action = MakeShared<FVoxelMetaGraphSchemaAction_NewStructNode>(
				FText::FromString(Node->GetCategory()),
				FText::FromString(DisplayName),
				FText::FromString(Node->GetTooltip()),
				0,
				FText::FromString(Keywords));

			Action->Struct = Node->GetStruct();

			ContextMenuBuilder.AddAction(Action);
		}
	}

	if (!Toolkit->GetAssetAs<UVoxelMetaGraph>().bIsMacroGraph)
	{
		if (!ContextMenuBuilder.FromPin ||
			ContextMenuBuilder.FromPin->Direction == EGPD_Input)
		{
			for (const FVoxelMetaGraphParameter& Parameter : Toolkit->GetAssetAs<UVoxelMetaGraph>().Parameters)
			{
				if (Parameter.ParameterType != EVoxelMetaGraphParameterType::Parameter)
				{
					continue;
				}

				if (ContextMenuBuilder.FromPin && 
					ContextMenuBuilder.FromPin->PinType != Parameter.Type)
				{
					continue;
				}

				const TSharedRef<FVoxelMetaGraphSchemaAction_NewParameterUsage> Action = MakeShared<FVoxelMetaGraphSchemaAction_NewParameterUsage>(
					VOXEL_LOCTEXT("Parameters"),
					FText::Format(VOXEL_LOCTEXT("Get {0}"), FText::FromName(Parameter.Name)),
					FText(),
					1);

				Action->Guid = Parameter.Guid;
				Action->ParameterType = EVoxelMetaGraphParameterType::Parameter;
				Action->PinType = Parameter.Type.GetEdGraphPinType();

				ContextMenuBuilder.AddAction(Action);
			}
		}
	}

	for (const FVoxelMetaGraphParameter& Parameter : Toolkit->GetAssetAs<UVoxelMetaGraph>().Parameters)
	{
		if (Parameter.ParameterType != EVoxelMetaGraphParameterType::LocalVariable)
		{
			continue;
		}

		if (!ContextMenuBuilder.FromPin ||
			(ContextMenuBuilder.FromPin->Direction == EGPD_Input && ContextMenuBuilder.FromPin->PinType == Parameter.Type))
		{
			const TSharedRef<FVoxelMetaGraphSchemaAction_NewParameterUsage> Action = MakeShared<FVoxelMetaGraphSchemaAction_NewParameterUsage>(
				VOXEL_LOCTEXT("Local Variables"),
				FText::Format(VOXEL_LOCTEXT("Get {0}"), FText::FromName(Parameter.Name)),
				FText(),
				1);

			Action->Guid = Parameter.Guid;
			Action->ParameterType = EVoxelMetaGraphParameterType::LocalVariable;
			Action->bDeclaration = false;
			Action->PinType = Parameter.Type.GetEdGraphPinType();

			ContextMenuBuilder.AddAction(Action);
		}

		if (!ContextMenuBuilder.FromPin ||
			(ContextMenuBuilder.FromPin->Direction == EGPD_Output && ContextMenuBuilder.FromPin->PinType == Parameter.Type))
		{
			const TSharedRef<FVoxelMetaGraphSchemaAction_NewParameterUsage> Action = MakeShared<FVoxelMetaGraphSchemaAction_NewParameterUsage>(
				VOXEL_LOCTEXT("Local Variables"),
				FText::Format(VOXEL_LOCTEXT("Set {0}"), FText::FromName(Parameter.Name)),
				FText(),
				1);

			Action->Guid = Parameter.Guid;
			Action->ParameterType = EVoxelMetaGraphParameterType::LocalVariable;
			Action->bDeclaration = true;
			Action->PinType = Parameter.Type.GetEdGraphPinType();

			ContextMenuBuilder.AddAction(Action);
		}
	}

	if (!ContextMenuBuilder.FromPin)
	{
		if (!Toolkit->GetAssetAs<UVoxelMetaGraph>().bIsMacroGraph)
		{
			const TSharedRef<FVoxelMetaGraphSchemaAction_NewParameter> Action = MakeShared<FVoxelMetaGraphSchemaAction_NewParameter>(
				FText(),
				VOXEL_LOCTEXT("Create new parameter"),
				VOXEL_LOCTEXT("Create new parameter"),
				1);
			Action->ParameterType = EVoxelMetaGraphParameterType::Parameter;
			ContextMenuBuilder.AddAction(Action);
		}
		else
		{
			{
				const TSharedRef<FVoxelMetaGraphSchemaAction_NewParameter> Action = MakeShared<FVoxelMetaGraphSchemaAction_NewParameter>(
					FText(),
					VOXEL_LOCTEXT("Create new input"),
					VOXEL_LOCTEXT("Create new input"),
					1);
				Action->ParameterType = EVoxelMetaGraphParameterType::MacroInput;
				ContextMenuBuilder.AddAction(Action);
			}
			{
				const TSharedRef<FVoxelMetaGraphSchemaAction_NewParameter> Action = MakeShared<FVoxelMetaGraphSchemaAction_NewParameter>(
					FText(),
					VOXEL_LOCTEXT("Create new output"),
					VOXEL_LOCTEXT("Create new output"),
					1);
				Action->ParameterType = EVoxelMetaGraphParameterType::MacroOutput;
				ContextMenuBuilder.AddAction(Action);
			}
		}

		const TSharedRef<FVoxelMetaGraphSchemaAction_NewParameter> Action = MakeShared<FVoxelMetaGraphSchemaAction_NewParameter>(
			FText(),
			VOXEL_LOCTEXT("Create new local variable"),
			VOXEL_LOCTEXT("Create new local variable"),
			1);
		Action->ParameterType = EVoxelMetaGraphParameterType::LocalVariable;
		ContextMenuBuilder.AddAction(Action);
	}
	else
	{
		if (!Toolkit->GetAssetAs<UVoxelMetaGraph>().bIsMacroGraph)
		{
			if (ContextMenuBuilder.FromPin->Direction == EGPD_Input)
			{
				const TSharedRef<FVoxelMetaGraphSchemaAction_NewParameter> Action = MakeShared<FVoxelMetaGraphSchemaAction_NewParameter>(
					FText(),
					VOXEL_LOCTEXT("Promote to parameter"),
					VOXEL_LOCTEXT("Promote to parameter"),
					1);
				Action->ParameterType = EVoxelMetaGraphParameterType::Parameter;
				ContextMenuBuilder.AddAction(Action);
			}
		}
		else
		{
			if (ContextMenuBuilder.FromPin->Direction == EGPD_Input)
			{
				const TSharedRef<FVoxelMetaGraphSchemaAction_NewParameter> Action = MakeShared<FVoxelMetaGraphSchemaAction_NewParameter>(
					FText(),
					VOXEL_LOCTEXT("Promote to input"),
					VOXEL_LOCTEXT("Promote to input"),
					1);
				Action->ParameterType = EVoxelMetaGraphParameterType::MacroInput;
				ContextMenuBuilder.AddAction(Action);
			}
			else
			{
				const TSharedRef<FVoxelMetaGraphSchemaAction_NewParameter> Action = MakeShared<FVoxelMetaGraphSchemaAction_NewParameter>(
					FText(),
					VOXEL_LOCTEXT("Promote to output"),
					VOXEL_LOCTEXT("Promote to output"),
					1);
				Action->ParameterType = EVoxelMetaGraphParameterType::MacroOutput;
				ContextMenuBuilder.AddAction(Action);
			}
		}

		{
			const TSharedRef<FVoxelMetaGraphSchemaAction_NewParameter> Action = MakeShared<FVoxelMetaGraphSchemaAction_NewParameter>(
				FText(),
				VOXEL_LOCTEXT("Promote to local variable"),
				VOXEL_LOCTEXT("Promote to local variable"),
				1);
			Action->ParameterType = EVoxelMetaGraphParameterType::LocalVariable;
			ContextMenuBuilder.AddAction(Action);
		}
	}
}

void UVoxelMetaGraphSchema::GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	Super::GetContextMenuActions(Menu, Context);

	const UEdGraphPin* Pin = Context->Pin;
	if (!Pin ||
		!ensure(Pin->GetOwningNode()) ||
		!ensure(Pin->GetOwningNode()->GetGraph()) ||
		Pin->bOrphanedPin ||
		Pin->bNotConnectable)
	{
		return;
	}

	const TSharedPtr<FVoxelMetaGraphEditorToolkit> Toolkit = GetToolkit(Pin->GetOwningNode()->GetGraph());
	if (!ensure(Toolkit))
	{
		return;
	}

	FToolMenuSection& Section = Menu->FindOrAddSection("EdGraphSchemaPinActions");

	if (UVoxelMetaGraphNode* Node = Cast<UVoxelMetaGraphNode>(Pin->GetOwningNode()))
	{
		FVoxelPinTypeSet Types;
		if (Node->CanPromotePin(*Pin, Types))
		{
			const FVoxelPinType CurrentPinType = Pin->PinType;
			const FVoxelPinType InnerType = CurrentPinType.GetInnerType();
			const FVoxelPinType BufferType = CurrentPinType.GetBufferType();

			const bool bIsBuffer =
				CurrentPinType == BufferType &&
				CurrentPinType != InnerType;

			TArray<FVoxelPinType> TypesList = Types.IsAll() ? FVoxelNodeLibrary::GetParameterTypes() : Types.GetTypes().Array();
			if (TypesList.Num() > 2 ||
				(TypesList.Num() == 2 && TypesList[0].GetInnerType() != TypesList[1].GetInnerType()))
			{
				Section.AddSubMenu(
					"PromotePin",
					VOXEL_LOCTEXT("Convert pin"),
					VOXEL_LOCTEXT("Convert this pin"),
					MakeLambdaDelegate([=](const FToolMenuContext&) -> TSharedRef<SWidget>
					{
						VOXEL_USE_NAMESPACE(MetaGraph);
					
						return
							SNew(SPinTypeSelector)
							.PinTypes_Lambda([=]
							{
								return TypesList;
							})
							.OnTypeChanged_Lambda([=](FVoxelPinType NewType)
							{
								const FVoxelTransaction Transaction(Pin, "Convert pin");
								if (bIsBuffer)
								{
									const FVoxelPinType NewBufferType = NewType.GetBufferType();
									if (Types.Contains(NewBufferType))
									{
										NewType = NewBufferType;
									}
								}
								Node->PromotePin(VOXEL_CONST_CAST(*Pin), NewType);
							})
							.OnCloseMenu_Lambda([=]
							{
								FSlateApplication::Get().ClearAllUserFocus();
							});
					})
				);
			}

			if (CurrentPinType != InnerType &&
				CurrentPinType == BufferType &&
				Types.Contains(InnerType))
			{
				Section.AddMenuEntry(
					"ConvertToUniform",
					VOXEL_LOCTEXT("Convert pin to Uniform"),
					VOXEL_LOCTEXT("Convert pin to Uniform"),
					FSlateIcon(),
					FUIAction(MakeLambdaDelegate([=]
					{
						const FVoxelTransaction Transaction(Pin, "Convert pin to Uniform");
						Node->PromotePin(VOXEL_CONST_CAST(*Pin), InnerType);
					}))
				);
			}
			else if (
				CurrentPinType == InnerType &&
				CurrentPinType != BufferType &&
				Types.Contains(BufferType))
			{
				Section.AddMenuEntry(
					"ConvertToBuffer",
					VOXEL_LOCTEXT("Convert pin to Buffer"),
					VOXEL_LOCTEXT("Convert pin to Buffer"),
					FSlateIcon(),
					FUIAction(MakeLambdaDelegate([=]
					{
						const FVoxelTransaction Transaction(Pin, "Convert pin to Buffer");
						Node->PromotePin(VOXEL_CONST_CAST(*Pin), BufferType);
					}))
				);
			}
		}
	}

	if (Pin->LinkedTo.Num() == 0 &&
		!Pin->bNotConnectable)
	{
		if (!Toolkit->GetAssetAs<UVoxelMetaGraph>().bIsMacroGraph)
		{
			if (Pin->Direction == EGPD_Input)
			{
				Section.AddMenuEntry(
					"PromoteToParameter",
					VOXEL_LOCTEXT("Promote to Parameter"),
					VOXEL_LOCTEXT("Promote to Parameter"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateUObject(this, &UVoxelMetaGraphSchema::PromoteToVariable, VOXEL_CONST_CAST(Pin), EVoxelMetaGraphParameterType::Parameter))
				);
			}
		}
		else
		{
			if (Pin->Direction == EGPD_Input)
			{
				Section.AddMenuEntry(
					"PromoteToInput",
					VOXEL_LOCTEXT("Promote to Input"),
					VOXEL_LOCTEXT("Promote to Input"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateUObject(this, &UVoxelMetaGraphSchema::PromoteToVariable, VOXEL_CONST_CAST(Pin), EVoxelMetaGraphParameterType::MacroInput))
				);
			}
			else
			{
				Section.AddMenuEntry(
					"PromoteToParameter",
					VOXEL_LOCTEXT("Promote to Output"),
					VOXEL_LOCTEXT("Promote to Output"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateUObject(this, &UVoxelMetaGraphSchema::PromoteToVariable, VOXEL_CONST_CAST(Pin), EVoxelMetaGraphParameterType::MacroOutput))
				);
			}
		}

		Section.AddMenuEntry(
			"PromoteToLocalVariable",
			VOXEL_LOCTEXT("Promote to Local Variable"),
			VOXEL_LOCTEXT("Promote to Local Variable"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateUObject(this, &UVoxelMetaGraphSchema::PromoteToVariable, VOXEL_CONST_CAST(Pin), EVoxelMetaGraphParameterType::LocalVariable))
		);
	}

	FToolMenuSection& PreviewSection = Menu->FindOrAddSection("PreviewSection");
	PreviewSection.InitSection("PreviewSection", VOXEL_LOCTEXT("Preview"), FToolMenuInsert({}, EToolMenuInsertType::First));

	if (Toolkit->GetAssetAs<UVoxelMetaGraph>().PreviewedPin.Get() == Pin)
	{
		PreviewSection.AddMenuEntry(
			"StopPinPreview",
			VOXEL_LOCTEXT("Stop pin preview"),
			VOXEL_LOCTEXT("Stop pin preview"),
			FSlateIcon(),
			FUIAction(
				MakeLambdaDelegate([WeakToolkit = MakeWeakPtr(Toolkit)]
				{
					const TSharedPtr<FVoxelMetaGraphEditorToolkit> PinnedToolkit = WeakToolkit.Pin();
					if (!PinnedToolkit)
					{
						return;
					}

					PinnedToolkit->GetAssetAs<UVoxelMetaGraph>().PreviewedPin = {};
					PinnedToolkit->OnGraphChanged();
				})
			)
		);
	}
	else 
	{
		const UEdGraphPin* PinToPreview = Pin;
		if (PinToPreview->Direction == EGPD_Input)
		{
			if (PinToPreview->LinkedTo.Num() != 1)
			{
				PinToPreview = nullptr;
			}
			else
			{
				PinToPreview = PinToPreview->LinkedTo[0];
			}
		}

		if (PinToPreview &&
			!FVoxelPinType(PinToPreview->PinType).IsDerivedFrom<FVoxelExecBase>())
		{
			PreviewSection.AddMenuEntry(
				"PreviewedPin",
				VOXEL_LOCTEXT("Preview pin"),
				VOXEL_LOCTEXT("Preview this pin"),
				FSlateIcon(),
				FUIAction(
					MakeLambdaDelegate([WeakToolkit = MakeWeakPtr(Toolkit), PinToPreview]
					{
						const TSharedPtr<FVoxelMetaGraphEditorToolkit> PinnedToolkit = WeakToolkit.Pin();
						if (!PinnedToolkit)
						{
							return;
						}

						PinnedToolkit->GetAssetAs<UVoxelMetaGraph>().PreviewedPin = PinToPreview;
						PinnedToolkit->OnGraphChanged();
					})
				)
			);
		}
	}
}

FLinearColor UVoxelMetaGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	return FVoxelMetaGraphVisuals::GetPinColor(PinType);
}

void UVoxelMetaGraphSchema::OnPinConnectionDoubleCicked(UEdGraphPin* PinA, UEdGraphPin* PinB, const FVector2D& GraphPosition) const
{
	const FVoxelTransaction Transaction(PinA, "Create Reroute Node");

	const FVector2D NodeSpacerSize(42.0f, 24.0f);
	const FVector2D KnotTopLeft = GraphPosition - (NodeSpacerSize * 0.5f);

	UEdGraph* ParentGraph = PinA->GetOwningNode()->GetGraph();

	UVoxelMetaGraphKnotNode* NewKnot = CastChecked<UVoxelMetaGraphKnotNode>(FVoxelMetaGraphSchemaAction_NewKnotNode().PerformAction(
		ParentGraph,
		nullptr,
		KnotTopLeft,
		true));

	// Move the connections across (only notifying the knot, as the other two didn't really change)
	PinA->BreakLinkTo(PinB);

	PinA->MakeLinkTo(NewKnot->GetPin(PinB->Direction));
	PinB->MakeLinkTo(NewKnot->GetPin(PinA->Direction));

	NewKnot->PropagatePinType();
}

TSharedPtr<FVoxelMetaGraphEditorToolkit> UVoxelMetaGraphSchema::GetToolkit(const UEdGraph* Graph)
{
	return StaticCastSharedPtr<FVoxelMetaGraphEditorToolkit>(Super::GetToolkit(Graph));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool UVoxelMetaGraphSchema::TryGetPromotionType(const UEdGraphPin& Pin, const FVoxelPinType& TargetType, FVoxelPinType& OutType) const
{
	OutType = {};

	const UVoxelMetaGraphNode* Node = Cast<UVoxelMetaGraphNode>(Pin.GetOwningNode());
	if (!ensure(Node))
	{
		return false;
	}

	FVoxelPinTypeSet Types;
	if (!Node->CanPromotePin(Pin, Types))
	{
		return false;
	}

	const FVoxelPinType CurrentType(Pin.PinType);

	const auto TryType = [&](const FVoxelPinType& NewType)
	{
		if (!OutType.IsValid() &&
			Types.Contains(NewType))
		{
			OutType = NewType;
		}
	};

	if (Pin.Direction == EGPD_Input)
	{
		// We're an input pin, we can implicitly promote to buffer

		if (CurrentType.IsBuffer())
		{
			// If we're currently a buffer, try to preserve that
			TryType(TargetType.GetBufferType());
			TryType(TargetType);
		}
		else
		{
			// Otherwise try the raw type first
			TryType(TargetType);
			TryType(TargetType.GetBufferType());
		}
	}
	else
	{
		// We're an output pin, we can never implicitly promote
		TryType(TargetType);
	}

	return OutType.IsValid();
}

void UVoxelMetaGraphSchema::PromoteToVariable(UEdGraphPin* Pin, EVoxelMetaGraphParameterType ParameterType) const
{
	if (!ensure(Pin))
	{
		return;
	}

	FVoxelMetaGraphSchemaAction_NewParameter Action;
	Action.ParameterType = ParameterType;

	const UEdGraphNode* OwningNode = Pin->GetOwningNode();
	FVector2D Position;
	Position.X = Pin->Direction == EGPD_Input ? OwningNode->NodePosX - 200 : OwningNode->NodePosX + 400;
	Position.Y = OwningNode->NodePosY + 75;

	Action.PerformAction(OwningNode->GetGraph(), Pin, Position, true);
}

TMap<FVoxelPinType, TSet<FVoxelPinType>> UVoxelMetaGraphSchema::CollectOperatorPermutations(const TVoxelInstancedStruct<FVoxelNode>& Node, const UEdGraphPin& FromPin, const FVoxelPinTypeSet& PromotionTypes)
{
	const FVoxelPinType FromPinType = FVoxelPinType(FromPin.PinType);
	const bool bIsBuffer = FromPinType.IsBuffer();

	const bool bIsCommutativeOperator = Node->GetStruct()->IsChildOf(FVoxelTemplateNode_CommutativeAssociativeOperator::StaticStruct());

	TMap<FVoxelPinType, TSet<FVoxelPinType>> Result;
	if (FromPin.Direction == EGPD_Output)
	{
		for (const FVoxelPinType& Type : PromotionTypes.GetTypes())
		{
			if (Type.IsBuffer() != bIsBuffer)
			{
				continue;
			}

			Result.FindOrAdd(FromPinType, {}).Add(Type);
			if (!bIsCommutativeOperator)
			{
				Result.FindOrAdd(Type, {}).Add(FromPinType);
			}
		}
	}
	else
	{
		const int32 FromDimension = FVoxelTemplateNodeUtilities::GetDimension(FromPinType);

		for (const FVoxelPinType& Type : PromotionTypes.GetTypes())
		{
			if (Type.IsBuffer() != bIsBuffer ||
				FromDimension < FVoxelTemplateNodeUtilities::GetDimension(Type))
			{
				continue;
			}

			if (bIsCommutativeOperator)
			{
				Result.FindOrAdd(FromPinType, {}).Add(Type);
				continue;
			}

			for (const FVoxelPinType& SecondType : PromotionTypes.GetTypes())
			{
				if (SecondType.IsBuffer() != bIsBuffer ||
					FromDimension < FVoxelTemplateNodeUtilities::GetDimension(SecondType))
				{
					continue;
				}

				if (Type != FromPinType &&
					SecondType != FromPinType)
				{
					continue;
				}

				Result.FindOrAdd(Type, {}).Add(SecondType);
			}
		}
	}

	return Result;
}