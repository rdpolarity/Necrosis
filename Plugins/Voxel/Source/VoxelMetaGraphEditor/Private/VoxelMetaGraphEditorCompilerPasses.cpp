// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphEditorCompilerPasses.h"
#include "Nodes/VoxelBasicNodes.h"
#include "Nodes/VoxelExposeDataNode.h"
#include "Nodes/VoxelMetaGraphStructNode.h"
#include "Nodes/VoxelMetaGraphLocalVariableNode.h"
#include "VoxelNodeLibrary.h"
#include "EdGraph/EdGraph.h"

BEGIN_VOXEL_NAMESPACE(MetaGraph)

void FEditorCompilerPasses::SetupPinPreview(FEditorCompiler& Compiler, const FEdGraphPinReference& PreviewedPinRef)
{
	const UEdGraphPin* SourcePreviewedPin = PreviewedPinRef.Get();
	if (!SourcePreviewedPin)
	{
		return;
	}

	UEdGraphNode* PreviewedNode = Compiler.GetNodeFromSourceNode(SourcePreviewedPin->GetOwningNode());
	if (!ensure(PreviewedNode))
	{
		return;
	}

	UEdGraphPin* PreviewedPin = PreviewedNode->FindPin(SourcePreviewedPin->PinName, SourcePreviewedPin->Direction);
	if (!ensure(PreviewedPin) ||
		!ensure(PreviewedPin->Direction == EGPD_Output))
	{
		return;
	}

	UVoxelMetaGraphStructNode& ExposeDataNode = Compiler.CreateNode<UVoxelMetaGraphStructNode>(PreviewedNode);
	ExposeDataNode.Struct = FVoxelNode_ExposeData::StaticStruct();
	ExposeDataNode.CreateNewGuid();
	ExposeDataNode.PostPlacedNewNode();
	ExposeDataNode.AllocateDefaultPins();

	// Promote will invalidate pin pointers
	{
		UEdGraphPin* DataPin = ExposeDataNode.FindPin(VOXEL_PIN_NAME(FVoxelNode_ExposeData, DataPin));
		if (!ensure(DataPin))
		{
			return;
		}
		ExposeDataNode.PromotePin(*DataPin, PreviewedPin->PinType);
	}

	UEdGraphPin* NamePin = ExposeDataNode.FindPin(VOXEL_PIN_NAME(FVoxelNode_ExposeData, NamePin));
	UEdGraphPin* DataPin = ExposeDataNode.FindPin(VOXEL_PIN_NAME(FVoxelNode_ExposeData, DataPin));
	if (!ensure(NamePin) ||
		!ensure(DataPin))
	{
		return;
	}

	NamePin->DefaultValue = "Internal_Preview";
	DataPin->MakeLinkTo(PreviewedPin);
}

bool FEditorCompilerPasses::AddToArrayNodes(FEditorCompiler& Compiler)
{
	for (UEdGraphNode* Node : Compiler.GetNodesCopy())
	{
		for (UEdGraphPin* InputPin : Node->Pins)
		{
			if (InputPin->Direction != EGPD_Input ||
				!FVoxelPinType(InputPin->PinType).IsBuffer())
			{
				continue;
			}

			for (UEdGraphPin* OutputPin : MakeCopy(InputPin->LinkedTo))
			{
				if (FVoxelPinType(OutputPin->PinType).IsBuffer())
				{
					continue;
				}

				UVoxelMetaGraphStructNode& ToArrayNode = Compiler.CreateNode<UVoxelMetaGraphStructNode>(Node);
				ToArrayNode.Struct = FVoxelNode_ToArray::StaticStruct();
				ToArrayNode.CreateNewGuid();
				ToArrayNode.PostPlacedNewNode();
				ToArrayNode.AllocateDefaultPins();
				ToArrayNode.PromotePin(*ToArrayNode.GetInputPin(0), OutputPin->PinType);

				ToArrayNode.GetInputPin(0)->MakeLinkTo(OutputPin);
				ToArrayNode.GetOutputPin(0)->MakeLinkTo(InputPin);
				InputPin->BreakLinkTo(OutputPin);
			}
		}
	}

	return true;
}

bool FEditorCompilerPasses::RemoveSplitPins(FEditorCompiler& Compiler)
{
	TMap<UEdGraphPin*, UVoxelMetaGraphStructNode*> ParentPinToMakeBreakNodes;

	for (UEdGraphNode* Node : Compiler.GetNodesCopy())
	{
		const TArray<UEdGraphPin*> PinsCopy = Node->Pins;
		for (UEdGraphPin* SubPin : PinsCopy)
		{
			if (!SubPin->ParentPin)
			{
				continue;
			}

			UVoxelMetaGraphStructNode*& MakeBreakNode = ParentPinToMakeBreakNodes.FindOrAdd(SubPin->ParentPin);

			if (!MakeBreakNode)
			{
				MakeBreakNode = &Compiler.CreateNode<UVoxelMetaGraphStructNode>(Node);

				if (SubPin->Direction == EGPD_Input)
				{
					MakeBreakNode->Struct = FVoxelNodeLibrary::FindMakeNode(SubPin->ParentPin->PinType);
				}
				else
				{
					check(SubPin->Direction == EGPD_Output);
					MakeBreakNode->Struct = FVoxelNodeLibrary::FindBreakNode(SubPin->ParentPin->PinType);
				}
				check(MakeBreakNode->Struct);

				MakeBreakNode->CreateNewGuid();
				MakeBreakNode->PostPlacedNewNode();
				MakeBreakNode->AllocateDefaultPins();

				if (SubPin->Direction == EGPD_Input)
				{
					ensure(MakeBreakNode->GetOutputPins().Num() == 1);
					UEdGraphPin* Pin = MakeBreakNode->GetOutputPin(0);

					FVoxelPinTypeSet Types;
					if (MakeBreakNode->CanPromotePin(*Pin, Types) &&
						ensure(Types.Contains(SubPin->ParentPin->PinType)))
					{
						MakeBreakNode->PromotePin(*Pin, SubPin->ParentPin->PinType);
						Pin = MakeBreakNode->GetOutputPin(0);
					}

					Pin->MakeLinkTo(SubPin->ParentPin);
				}
				else
				{
					check(SubPin->Direction == EGPD_Output);
					ensure(MakeBreakNode->GetInputPins().Num() == 1);
					UEdGraphPin* Pin = MakeBreakNode->GetInputPin(0);

					FVoxelPinTypeSet Types;
					if (MakeBreakNode->CanPromotePin(*Pin, Types) &&
						ensure(Types.Contains(SubPin->ParentPin->PinType)))
					{
						MakeBreakNode->PromotePin(*Pin, SubPin->ParentPin->PinType);
						Pin = MakeBreakNode->GetInputPin(0);
					}

					Pin->MakeLinkTo(SubPin->ParentPin);
				}
			}

			UEdGraphPin* NewPin = MakeBreakNode->FindPin(SubPin->GetName());
			if (!ensure(NewPin))
			{
				return false;
			}
			ensure(NewPin->LinkedTo.Num() == 0);

			NewPin->MovePersistentDataFromOldPin(*SubPin);

			SubPin->MarkAsGarbage();
			ensure(Node->Pins.Remove(SubPin) == 1);
		}
	}

	return true;
}

bool FEditorCompilerPasses::FixupLocalVariables(FEditorCompiler& Compiler)
{
	TMap<FGuid, UVoxelMetaGraphLocalVariableDeclarationNode*> Declarations;
	TMap<UVoxelMetaGraphLocalVariableDeclarationNode*, TArray<UVoxelMetaGraphLocalVariableUsageNode*>> UsageDeclarationMapping;
	TMap<const FVoxelMetaGraphParameter*, TArray<UVoxelMetaGraphLocalVariableUsageNode*>> FreeUsages;

	for (UEdGraphNode* Node : Compiler.GetNodesCopy())
	{
		if (UVoxelMetaGraphLocalVariableDeclarationNode* Declaration = Cast<UVoxelMetaGraphLocalVariableDeclarationNode>(Node))
		{
			if (!Declaration->GetParameter())
			{
				VOXEL_MESSAGE(Error, "Invalid local variable node: {0}", Compiler.GetSourceNode(Node));
				return false;
			}

			if (Declarations.Contains(Declaration->Guid))
			{
				VOXEL_MESSAGE(Error, "Multiple local variable declarations not supported: {0} and {1}",
					Compiler.GetSourceNode(Declarations[Declaration->Guid]),
					Compiler.GetSourceNode(Node));
				return false;
			}

			if (const UVoxelGraphNode* LoopNode = Declaration->IsInLoop())
			{
				VOXEL_MESSAGE(Error, "Local variable is in loop: {0} with {1}",
					Compiler.GetSourceNode(Node),
					Compiler.GetSourceNode(LoopNode));
				return false;
			}

			Declarations.Add(Declaration->Guid, Declaration);
			UsageDeclarationMapping.FindOrAdd(Declaration);
		}
		else if (UVoxelMetaGraphLocalVariableUsageNode* Usage = Cast<UVoxelMetaGraphLocalVariableUsageNode>(Node))
		{
			const FVoxelMetaGraphParameter* Parameter = Usage->GetParameter();
			if (!Parameter)
			{
				VOXEL_MESSAGE(Error, "Invalid local variable node: {0}", Compiler.GetSourceNode(Node));
				return false;
			}

			UVoxelMetaGraphLocalVariableDeclarationNode* DeclarationNode = Usage->FindDeclaration();
			if (!DeclarationNode)
			{
				FreeUsages.FindOrAdd(Parameter).Add(Usage);
				continue;
			}

			UsageDeclarationMapping.FindOrAdd(DeclarationNode).Add(Usage);
		}
	}

	for (auto& It : UsageDeclarationMapping)
	{
		const UVoxelMetaGraphLocalVariableDeclarationNode* Declaration = It.Key;

		UEdGraphPin* InputPin = Declaration->GetInputPin(0);
		for (UEdGraphPin* LinkedToPin : Declaration->GetOutputPin(0)->LinkedTo)
		{
			LinkedToPin->CopyPersistentDataFromOldPin(*InputPin);
			LinkedToPin->DefaultValue = InputPin->DefaultValue;
			LinkedToPin->DefaultObject = InputPin->DefaultObject;
		}

		for (const UVoxelMetaGraphLocalVariableUsageNode* Usage : It.Value)
		{
			for (UEdGraphPin* LinkedToPin : Usage->GetOutputPin(0)->LinkedTo)
			{
				LinkedToPin->CopyPersistentDataFromOldPin(*InputPin);
				LinkedToPin->DefaultValue = InputPin->DefaultValue;
				LinkedToPin->DefaultObject = InputPin->DefaultObject;
			}
		}
	}

	for (auto& It : FreeUsages)
	{
		for (const UVoxelMetaGraphLocalVariableUsageNode* Usage : It.Value)
		{
			for (UEdGraphPin* LinkedToPin : Usage->GetOutputPin(0)->LinkedTo)
			{
				It.Key->DefaultValue.ApplyToPinDefaultValue(*LinkedToPin);
			}
		}
	}

	for (auto& It : UsageDeclarationMapping)
	{
		It.Key->DestroyNode();
		
		for (UVoxelMetaGraphLocalVariableUsageNode* Node : It.Value)
		{
			Node->DestroyNode();
		}
	}

	for (auto& It : FreeUsages)
	{
		for (UVoxelMetaGraphLocalVariableUsageNode* Node : It.Value)
		{
			Node->DestroyNode();
		}
	}

	return true;
}

END_VOXEL_NAMESPACE(MetaGraph)