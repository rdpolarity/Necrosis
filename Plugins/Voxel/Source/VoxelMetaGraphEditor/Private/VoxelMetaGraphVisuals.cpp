// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphVisuals.h"
#include "VoxelEdGraph.h"
#include "VoxelExecNode.h"
#include "VoxelMetaGraphSeed.h"
#include "VoxelMetaGraphSchema.h"
#include "VoxelMetaGraphToolkit.h"
#include "Nodes/VoxelMetaGraphKnotNode.h"
#include "Widgets/SVoxelMetaGraphNode.h"
#include "Widgets/SVoxelMetaGraphPinSeed.h"
#include "Widgets/SVoxelMetaGraphPinEnum.h"
#include "Widgets/SVoxelMetaGraphPinObject.h"
#include "Widgets/SVoxelMetaGraphNodeKnot.h"
#include "Widgets/SVoxelMetaGraphNodeVariable.h"

#include "EdGraphUtilities.h"
#include "Styling/SlateIconFinder.h"
#include "ConnectionDrawingPolicy.h"

#include "KismetPins/SGraphPinNum.h"
#include "KismetPins/SGraphPinExec.h"
#include "KismetPins/SGraphPinBool.h"
#include "KismetPins/SGraphPinInteger.h"
#include "KismetPins/SGraphPinColor.h"
#include "KismetPins/SGraphPinString.h"
#include "KismetPins/SGraphPinVector.h"
#include "KismetPins/SGraphPinVector2D.h"

FSlateIcon FVoxelMetaGraphVisuals::GetPinIcon(const FVoxelPinType& InType)
{
	static const FSlateIcon VariableIcon(UE_501_SWITCH(FEditorStyle::GetStyleSetName(), FAppStyle::GetAppStyleSetName()), "Kismet.VariableList.TypeIcon");
	const FVoxelPinType Type = InType.GetExposedType();

	if (Type.IsObject())
	{
		if (const UClass* Class = Type.GetClass())
		{
			return FSlateIconFinder::FindIconForClass(Class);
		}
		else
		{
			return VariableIcon;
		}
	}

	if (InType.IsBuffer())
	{
		static const FSlateIcon BufferIcon(FVoxelEditorStyle::GetStyleSetName(), "Pill.Buffer");
		return BufferIcon;
	}

	return VariableIcon;
}

FLinearColor FVoxelMetaGraphVisuals::GetPinColor(const FVoxelPinType& InType)
{
	const FVoxelPinType Type = InType.GetExposedType();
	
	const UGraphEditorSettings* Settings = GetDefault<UGraphEditorSettings>();
	
	if (Type.IsDerivedFrom<FVoxelExecBase>())
	{
		return Settings->ExecutionPinTypeColor;
	}
	else if (Type.IsWildcard())
	{
		return Settings->WildcardPinTypeColor;
	}
	else if (Type.Is<bool>())
	{
		return Settings->BooleanPinTypeColor;
	}
	else if (Type.Is<float>())
	{
		if (Type.GetTag() == STATIC_FNAME("Density"))
		{
			return FLinearColor(0.0144f, 0.244792f, 0.f);
		}

		return Settings->FloatPinTypeColor;
	}
	else if (Type.Is<int32>())
	{
		return Settings->IntPinTypeColor;
	}
	else if (Type.Is<FName>())
	{
		return Settings->StringPinTypeColor;
	}
	else if (Type.IsEnum())
	{
		return Settings->BytePinTypeColor;
	}
	else if (Type.Is<FVector>())
	{
		return Settings->VectorPinTypeColor;
	}
	else if (
		Type.Is<FRotator>() ||
		Type.Is<FQuat>())
	{
		return Settings->RotatorPinTypeColor;
	}
	else if (Type.Is<FTransform>())
	{
		return Settings->TransformPinTypeColor;
	}
	else if (Type.Is<FVoxelMetaGraphSeed>())
	{
		return Settings->NamePinTypeColor;
	}
	else if (Type.IsStruct())
	{
		return Settings->StructPinTypeColor;
	}
	else if (Type.IsObject())
	{
		return Settings->ObjectPinTypeColor;
	}
	else
	{
		ensure(false);
		return Settings->DefaultPinTypeColor;
	}
}

TSharedPtr<SGraphPin> FVoxelMetaGraphVisuals::GetPinWidget(UEdGraphPin* Pin)
{
	const FVoxelPinType Type = FVoxelPinType(Pin->PinType).GetExposedType();
	
#define SNewPerf(Name, Pin) VOXEL_INLINE_COUNTER(#Name, SNew(Name, Pin))
	
	if (Type.IsDerivedFrom<FVoxelExecBase>())
	{
		return SNew(SGraphPinExec, Pin);
	}
	else if (Type.IsWildcard())
	{
		return SNewPerf(SGraphPin, Pin);
	}
	else if (Type.Is<bool>())
	{
		return SNewPerf(SGraphPinBool, Pin);
	}
	else if (Type.Is<float>())
	{
		return SNewPerf(SGraphPinNum<float>, Pin);
	}
	else if (Type.Is<int32>())
	{
		return SNewPerf(SGraphPinInteger, Pin);
	}
	else if (Type.Is<FName>())
	{
		return SNewPerf(SGraphPinString, Pin);
	}
	else if (Type.IsEnum())
	{
		return SNewPerf(SVoxelMetaGraphPinEnum, Pin);
	}
	else if (Type.Is<FVector2D>())
	{
		return SNewPerf(SGraphPinVector2D UE_501_ONLY(<float>), Pin);
	}
	else if (Type.Is<FVector>() || Type.Is<FRotator>())
	{
		return SNewPerf(SGraphPinVector UE_501_ONLY(<float>), Pin);
	}
	else if (Type.Is<FLinearColor>())
	{
		return SNewPerf(SGraphPinColor, Pin);
	}
	else if (Type.Is<FVoxelMetaGraphSeed>())
	{
		return SNewPerf(SVoxelMetaGraphPinSeed, Pin);
	}
	else if (Type.IsStruct())
	{
		return SNewPerf(SGraphPin, Pin);
	}
	else if (Type.IsObject())
	{
		return SNewPerf(SVoxelMetaGraphPinObject, Pin);
	}
	else
	{
		ensure(false);
		return nullptr;
	}

#undef SNewPerf
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class FVoxelMetaGraphConnectionDrawingPolicy : public FConnectionDrawingPolicy
{
public:
	const UVoxelEdGraph& Graph;
	const TSharedPtr<FVoxelMetaGraphEditorToolkit> Toolkit;

	FVoxelMetaGraphConnectionDrawingPolicy(
		int32 InBackLayerID,
		int32 InFrontLayerID,
		float ZoomFactor,
		const FSlateRect& InClippingRect,
		FSlateWindowElementList& InDrawElements,
		const UVoxelEdGraph& Graph)
		: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements)
		, Graph(Graph)
		, Toolkit(StaticCastSharedPtr<FVoxelMetaGraphEditorToolkit>(Graph.GetGraphToolkit()))
	{
		ArrowImage = nullptr;
		ArrowRadius = FVector2D::ZeroVector;
	}

	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FConnectionParams& Params) override
	{
		FConnectionDrawingPolicy::DetermineWiringStyle(OutputPin, InputPin, Params);

		if (HoveredPins.Contains(InputPin) &&
			HoveredPins.Contains(OutputPin))
		{
			Params.WireThickness *= 5;
		}

		if ((InputPin && InputPin->bOrphanedPin) ||
			(OutputPin && OutputPin->bOrphanedPin))
		{
			Params.WireColor = FLinearColor::Red;
			return;
		}

		if (!ensure(OutputPin))
		{
			return;
		}

		Params.WireColor = FVoxelMetaGraphVisuals::GetPinColor(OutputPin->PinType);

		if (Toolkit->GetAssetAs<UVoxelMetaGraph>().PreviewedPin.Get() == OutputPin)
		{
			Params.bDrawBubbles = true;
			Params.WireThickness = FMath::Max(Params.WireThickness, 4.f);
		}
	}
};

struct FVoxelMetaGraphConnectionDrawingPolicyFactory : public FGraphPanelPinConnectionFactory
{
public:
	virtual FConnectionDrawingPolicy* CreateConnectionPolicy(const UEdGraphSchema* Schema, int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj) const override
	{
		if (!Schema->IsA<UVoxelMetaGraphSchema>())
		{
			return nullptr;
		}

		return new FVoxelMetaGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, *CastChecked<UVoxelEdGraph>(InGraphObj));
	}
};

class FVoxelMetaGraphPanelPinFactory : public FGraphPanelPinFactory
{
	virtual TSharedPtr<SGraphPin> CreatePin(UEdGraphPin* Pin) const override
	{
		VOXEL_FUNCTION_COUNTER();

		const UVoxelMetaGraphSchema* Schema = Cast<UVoxelMetaGraphSchema>(Pin->GetSchema());
		if (!Schema)
		{
			return nullptr;
		}

		return FVoxelMetaGraphVisuals::GetPinWidget(Pin);
	}
};

class FVoxelMetaGraphNodeFactory : public FGraphPanelNodeFactory
{
	virtual TSharedPtr<SGraphNode> CreateNode(UEdGraphNode* InNode) const override
	{
		VOXEL_FUNCTION_COUNTER();

		const UVoxelMetaGraphSchema* Schema = Cast<UVoxelMetaGraphSchema>(InNode->GetSchema());
		if (!Schema)
		{
			return nullptr;
		}

		UVoxelMetaGraphNode* Node = Cast<UVoxelMetaGraphNode>(InNode);
		if (!Node)
		{
			return nullptr;
		}

		if (UVoxelMetaGraphKnotNode* Knot = Cast<UVoxelMetaGraphKnotNode>(Node))
		{
			return SNew(SVoxelMetaGraphNodeKnot, Knot);
		}

		if (UVoxelMetaGraphParameterNodeBase* Parameter = Cast<UVoxelMetaGraphParameterNodeBase>(Node))
		{
			return SNew(SVoxelMetaGraphNodeVariable, Parameter);
		}

		return SNew(SVoxelMetaGraphNode, Node);
	}
};

VOXEL_RUN_ON_STARTUP_EDITOR(RegisterMetaGraphConnectionDrawingPolicyFactory)
{
	FEdGraphUtilities::RegisterVisualNodeFactory(MakeShared<FVoxelMetaGraphNodeFactory>());
	FEdGraphUtilities::RegisterVisualPinFactory(MakeShared<FVoxelMetaGraphPanelPinFactory>());
	FEdGraphUtilities::RegisterVisualPinConnectionFactory(MakeShared<FVoxelMetaGraphConnectionDrawingPolicyFactory>());
}