// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "SVoxelMetaGraphNode.h"
#include "VoxelNode.h"
#include "SGraphPin.h"
#include "SCommentBubble.h"
#include "SLevelOfDetailBranchNode.h"
#include "GraphEditorSettings.h"
#include "Widgets/Images/SLayeredImage.h"

VOXEL_INITIALIZE_STYLE(GraphNodeEditor)
{
	Set("Node.Overlay.Warning", new IMAGE_BRUSH("Graphs/NodeOverlay_Warning", CoreStyleConstants::Icon32x32));

	Set("Pin.Buffer.Connected", new IMAGE_BRUSH("Graphs/BufferPin_Connected", FVector2D(15, 11)));
	Set("Pin.Buffer.Disconnected", new IMAGE_BRUSH("Graphs/BufferPin_Disconnected", FVector2D(15, 11)));
	Set("Pin.Buffer.Promotable.Inner", new IMAGE_BRUSH_SVG("Graphs/BufferPin_Promotable_Inner", CoreStyleConstants::Icon14x14));
	Set("Pin.Buffer.Promotable.Outer", new IMAGE_BRUSH_SVG("Graphs/BufferPin_Promotable_Outer", CoreStyleConstants::Icon14x14));

	Set("Node.Stats.TitleGloss", new BOX_BRUSH("Graphs/Node_Stats_Gloss", FMargin(12.0f / 64.0f)));
	Set("Node.Stats.ColorSpill", new BOX_BRUSH("Graphs/Node_Stats_Color_Spill", FMargin(8.0f / 64.0f, 3.0f / 32.0f, 0.f, 0.f)));

	Set("Icons.MinusCircle", new IMAGE_BRUSH_SVG("Graphs/Minus_Circle", CoreStyleConstants::Icon16x16));
}

void SVoxelMetaGraphNode::Construct(const FArguments& Args, UVoxelGraphNode* InNode)
{
	VOXEL_FUNCTION_COUNTER();

	GraphNode = InNode;
	NodeDefinition = GetVoxelNode().GetNodeDefinition();

	SetCursor(EMouseCursor::CardinalCross);
	SetToolTipText(MakeAttributeSP(this, &SGraphNode::GetNodeTooltip));

	ContentScale.Bind(this, &SGraphNode::GetContentScale);

	UpdateGraphNode();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelMetaGraphNode::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();

	LeftNodeBox.Reset();
	RightNodeBox.Reset();

	CategoryPins.Reset();

	SetupErrorReporting();

	if (GetVoxelNode().IsCompact())
	{
		UpdateCompactNode();
	}
	else
	{
		UpdateStandardNode();
	}

	FString Type;
	FString Tooltip;
	FString ColorType;
	if (GetVoxelNode().GetOverlayInfo(Type, Tooltip, ColorType))
	{
		const FSlateBrush* ImageBrush;
		if (Type == "Warning")
		{
			ImageBrush = FVoxelEditorStyle::Get().GetBrush(TEXT("Node.Overlay.Warning"));
		}
		else if (Type == "Lighting")
		{
			ImageBrush = FEditorAppStyle::Get().GetBrush(TEXT("Graph.AnimationFastPathIndicator"));
		}
		else if (Type == "Clock")
		{
			ImageBrush = FEditorAppStyle::Get().GetBrush(TEXT("Graph.Latent.LatentIcon"));
		}
		else if (Type == "Message")
		{
			ImageBrush = FEditorAppStyle::Get().GetBrush(TEXT("Graph.Message.MessageIcon"));
		}
		else
		{
			ensure(false);
			return;
		}

		FLinearColor Color = FLinearColor::White;
		if (ColorType == "Red")
		{
			Color = FLinearColor::Red;
		}

		OverlayWidget.Widget =
			SNew(SImage)
			.Image(ImageBrush)
			.ToolTipText(FText::FromString(Tooltip))
			.Visibility(EVisibility::Visible)
			.ColorAndOpacity(Color);

		OverlayWidget.BrushSize = ImageBrush->ImageSize;
	}
}

TSharedPtr<SGraphPin> SVoxelMetaGraphNode::CreatePinWidget(UEdGraphPin* Pin) const
{
	const TSharedPtr<SGraphPin> GraphPin = SGraphNode::CreatePinWidget(Pin);
	if (!ensure(GraphPin))
	{
		return nullptr;
	}

	{
		SLayeredImage& PinImage = static_cast<SLayeredImage&>(*GraphPin->GetPinImageWidget());

		static const FSlateBrush* PromotableOuterIcon = FEditorAppStyle::GetBrush("Kismet.VariableList.PromotableTypeOuterIcon");
		static const FSlateBrush* PromotableInnerIcon = FEditorAppStyle::GetBrush("Kismet.VariableList.PromotableTypeInnerIcon");

		static const FSlateBrush* BufferConnectedIcon = FVoxelEditorStyle::GetBrush("Pin.Buffer.Connected");
		static const FSlateBrush* BufferDisconnectedIcon = FVoxelEditorStyle::GetBrush("Pin.Buffer.Disconnected");
		static const FSlateBrush* BufferPromotableInnerIcon = FVoxelEditorStyle::GetBrush("Pin.Buffer.Promotable.Inner");
		static const FSlateBrush* BufferPromotableOuterIcon = FVoxelEditorStyle::GetBrush("Pin.Buffer.Promotable.Outer");

		ensure(PinImage.GetNumLayers() == 2);

		struct SGraphPinHack : SGraphPin
		{
			using SGraphPin::GetPinIcon;
		};
		const SGraphPinHack* PinForIcon = static_cast<SGraphPinHack*>(GraphPin.Get());

		const bool bIsPromotable = GetVoxelNode().ShowAsPromotableWildcard(*Pin);
		const bool bIsBuffer = FVoxelPinType(Pin->PinType).IsBuffer();

		PinImage.SetLayerBrush(0, MakeAttributeLambda([=]() -> const FSlateBrush*
		{
			if (!ensure(Pin))
			{
				return nullptr;
			}

			if (bIsBuffer)
			{
				if (bIsPromotable)
				{
					return BufferPromotableOuterIcon;
				}

				if (PinForIcon->IsConnected())
				{
					return BufferConnectedIcon;
				}
				else
				{
					return BufferDisconnectedIcon;
				}
			}

			if (bIsPromotable)
			{
				return PromotableOuterIcon;
			}

			return PinForIcon->GetPinIcon();
		}));

		PinImage.SetLayerBrush(1, MakeAttributeLambda([=]() -> const FSlateBrush*
		{
			if (!ensure(Pin))
			{
				return nullptr;
			}

			if (bIsBuffer)
			{
				return BufferPromotableInnerIcon;
			}

			return PromotableInnerIcon;
		}));

		PinImage.SetLayerColor(1, MakeAttributeLambda([=]() -> FSlateColor
		{
			if (!ensure(Pin))
			{
				return FLinearColor::Transparent;
			}

			if (bIsPromotable)
			{
				// Set the inner image to be wildcard color, which is grey by default
				return GetDefault<UGraphEditorSettings>()->WildcardPinTypeColor;
			}

			return FLinearColor::Transparent;
		}));
	};

	INLINE_LAMBDA
	{
		if (!GetVoxelNode().ShouldHideConnectorPin(*Pin))
		{
			return;
		}

		const TSharedPtr<SHorizontalBox> HorizontalRow = GraphPin->GetFullPinHorizontalRowWidget().Pin();
		if (!ensure(HorizontalRow) ||
			!ensure(HorizontalRow->GetChildren()->NumSlot() > 0))
		{
			return;
		}

		SHorizontalBox::FSlot& ConnectorSlot = HorizontalRow->GetSlot(0);
		ConnectorSlot.GetWidget()->SetVisibility(EVisibility::Collapsed);
		ConnectorSlot.SetPadding(0.f);

		const TSharedPtr<SWrapBox> LabelWidget = GraphPin->GetLabelAndValue();
		if (!ensure(LabelWidget) ||
			!ensure(LabelWidget->GetChildren()->NumSlot() > 0))
		{
			return;
		}

		const FSlotBase& LabelSlot = LabelWidget->GetChildren()->GetSlotAt(0);
		LabelSlot.GetWidget()->SetVisibility(EVisibility::Collapsed);
	};

	return GraphPin;
}

void SVoxelMetaGraphNode::CreateOutputSideAddButton(TSharedPtr<SVerticalBox> OutputBox)
{
	const TSharedRef<SWidget> Button = AddPinButtonContent(FText::FromString(NodeDefinition->GetAddPinLabel()), {});
	Button->SetCursor(EMouseCursor::Default);

	FMargin Padding = Settings->GetOutputPinPadding();
	Padding.Top += 6.0f;

	OutputBox->AddSlot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.Padding(Padding)
		[
			SNew(SBox)
			.Padding(0)
			.Visibility(this, &SVoxelMetaGraphNode::IsAddPinButtonVisible)
			.ToolTipText(FText::FromString(NodeDefinition->GetAddPinTooltip()))
			[
				Button
			]
		];
}

EVisibility SVoxelMetaGraphNode::IsAddPinButtonVisible() const
{
	return GetButtonVisibility(NodeDefinition->CanAddInputPin());
}

FReply SVoxelMetaGraphNode::OnAddPin()
{
	const FVoxelTransaction Transaction(GetVoxelNode(), "Add input pin");
	NodeDefinition->AddInputPin();
	return FReply::Handled();
}

void SVoxelMetaGraphNode::RequestRenameOnSpawn()
{
	if (GetVoxelNode().CanRenameOnSpawn())
	{
		RequestRename();
	}
}

TArray<FOverlayWidgetInfo> SVoxelMetaGraphNode::GetOverlayWidgets(bool bSelected, const FVector2D& WidgetSize) const
{
	TArray<FOverlayWidgetInfo> Widgets;

	if (OverlayWidget.Widget)
	{
		FOverlayWidgetInfo Info;
		Info.OverlayOffset = OverlayWidget.GetLocation(WidgetSize);
		Info.Widget = OverlayWidget.Widget;

		Widgets.Add(Info);
	}

	return Widgets;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelMetaGraphNode::UpdateStandardNode()
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedRef<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);

	const FSlateBrush* IconBrush;
	if (GraphNode &&
		GraphNode->ShowPaletteIconOnNode())
	{
		IconColor = FLinearColor::White;
		IconBrush = GraphNode->GetIconAndTint(IconColor).GetOptionalIcon();
	}
	else
	{
		IconColor = FLinearColor::White;
		IconBrush = nullptr;
	}

	TSharedPtr<SHorizontalBox> TitleBox;

	const TSharedRef<SWidget> TitleAreaWidget =
		SNew(SLevelOfDetailBranchNode)
		.UseLowDetailSlot(this, &SVoxelMetaGraphNode::UseLowDetailNodeTitles)
		.LowDetail()
		[
			SNew(SBorder)
			.BorderImage(FEditorAppStyle::GetBrush("Graph.Node.ColorSpill"))
			.Padding(FMargin(75.0f, 22.0f)) // Saving enough space for a 'typical' title so the transition isn't quite so abrupt
			.BorderBackgroundColor(this, &SGraphNode::GetNodeTitleColor)
		]
		.HighDetail()
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image(FEditorAppStyle::GetBrush("Graph.Node.TitleGloss"))
				.ColorAndOpacity(this, &SGraphNode::GetNodeTitleIconColor)
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			[
				SAssignNew(TitleBox, SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.AutoWidth()
				[
					SNew(SBorder)
					.BorderImage(FEditorAppStyle::GetBrush("Graph.Node.ColorSpill"))
					// The extra margin on the right is for making the color spill stretch well past the node title
					.Padding(FMargin(10, 5, 30, 3))
					.BorderBackgroundColor(this, &SGraphNode::GetNodeTitleColor)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Top)
						.Padding(FMargin(0.f, 0.f, 4.f, 0.f))
						.AutoWidth()
						[
							SNew(SImage)
							.Image(IconBrush)
							.ColorAndOpacity(this, &SGraphNode::GetNodeTitleIconColor)
						]
						+ SHorizontalBox::Slot()
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								CreateTitleWidget(NodeTitle)
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								NodeTitle
							]
						]
					]
				]
			]
			+ SOverlay::Slot()
			.VAlign(VAlign_Top)
			[
				SNew(SBorder)
				.Visibility(EVisibility::HitTestInvisible)
				.BorderImage(FEditorAppStyle::GetBrush("Graph.Node.TitleHighlight"))
				.BorderBackgroundColor(this, &SGraphNode::GetNodeTitleIconColor)
				[
					SNew(SSpacer)
					.Size(FVector2D(20, 20))
				]
			]
		];

	if (TitleBox)
	{
		CreateStandardNodeTitleButtons(TitleBox);
	}

	GetOrAddSlot(ENodeZone::Center)
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			.Padding(Settings->GetNonPinNodeBodyPadding())
			[
				SNew(SImage)
				.Image(FEditorAppStyle::GetBrush("Graph.Node.Body"))
				.ColorAndOpacity(this, &SVoxelMetaGraphNode::GetNodeBodyColor)
			]
			+ SOverlay::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Top)
				.Padding(Settings->GetNonPinNodeBodyPadding())
				[
					TitleAreaWidget
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Top)
				[
					CreateNodeContentArea()
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(Settings->GetNonPinNodeBodyPadding())
				[
					MakeStatWidget()
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					ErrorReporting->AsWidget()
				]
			]
		]
	];

	const TSharedRef<SCommentBubble> CommentBubble = 
		SNew(SCommentBubble)
		.GraphNode(GraphNode)
		.Text(this, &SGraphNode::GetNodeComment)
		.OnTextCommitted(this, &SGraphNode::OnCommentTextCommitted)
		.OnToggled(this, &SGraphNode::OnCommentBubbleToggled)
		.ColorAndOpacity(GetDefault<UGraphEditorSettings>()->DefaultCommentNodeTitleColor)
		.AllowPinning(true)
		.EnableTitleBarBubble(true)
		.EnableBubbleCtrls(true)
		.GraphLOD(this, &SGraphNode::GetCurrentLOD)
		.IsGraphNodeHovered(this, &SGraphNode::IsHovered);

	GetOrAddSlot(ENodeZone::TopCenter)
	.SlotOffset(MakeAttributeSP(&CommentBubble.Get(), &SCommentBubble::GetOffset))
	.SlotSize(MakeAttributeSP(&CommentBubble.Get(), &SCommentBubble::GetSize))
	.AllowScaling(MakeAttributeSP(&CommentBubble.Get(), &SCommentBubble::IsScalingAllowed))
	.VAlign(VAlign_Top)
	[
		CommentBubble
	];

	if (NodeDefinition->OverridePinsOrder())
	{
		CreateCategorizedPinWidgets();
	}
	else
	{
		CreatePinWidgets();
	}

	CreateInputSideAddButton(LeftNodeBox);
}

void SVoxelMetaGraphNode::UpdateCompactNode()
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedRef<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);
	
	constexpr float MinNodePadding = 55.f;
	constexpr float MaxNodePadding = 180.0f;
	constexpr float PaddingIncrementSize = 20.0f;

	const float PinPaddingRight = FMath::Clamp(MinNodePadding + NodeTitle->GetHeadTitle().ToString().Len() * PaddingIncrementSize, MinNodePadding, MaxNodePadding);

	const TSharedRef<SOverlay> NodeOverlay = 
		SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(0.f, 0.f, PinPaddingRight, 0.f)
		[
			// LEFT
			SAssignNew(LeftNodeBox, SVerticalBox)
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(45.f, 0.f, 45.f, 0.f)
		[
			// MIDDLE
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Center)
			.AutoHeight()
			[
				SNew(STextBlock)
				.TextStyle(FEditorAppStyle::Get(), "Graph.CompactNode.Title")
				.Text(&NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
				.WrapTextAt(128.0f)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				NodeTitle
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		.Padding(55.f, 0.f, 0.f, 0.f)
		[
			// RIGHT
			SAssignNew(RightNodeBox, SVerticalBox)
		];

	this->GetOrAddSlot(ENodeZone::Center)
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image(FEditorAppStyle::GetBrush("Graph.VarNode.Body"))
			]
			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image(FEditorAppStyle::GetBrush("Graph.VarNode.Gloss"))
			]
			+ SOverlay::Slot()
			.Padding(FMargin(0.f, 3.f, 0.f, 0.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.Padding(FMargin(0.f, 0.f, 0.f, 4.f))
				[
					NodeOverlay
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakeStatWidget()
				]
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			ErrorReporting->AsWidget()
		]
	];

	CreatePinWidgets();

	// Hide pin labels
	for (const TSharedRef<SGraphPin>& Pin : InputPins)
	{
		if (!Pin->GetPinObj()->ParentPin)
		{
			Pin->SetShowLabel(false);
		}
	}
	for (const TSharedRef<SGraphPin>& Pin : OutputPins)
	{
		if (!Pin->GetPinObj()->ParentPin)
		{
			Pin->SetShowLabel(false);
		}
	}

	const TSharedRef<SCommentBubble> CommentBubble =
		SNew(SCommentBubble)
		.GraphNode(GraphNode)
		.Text(this, &SGraphNode::GetNodeComment)
		.OnTextCommitted(this, &SGraphNode::OnCommentTextCommitted)
		.ColorAndOpacity(GetDefault<UGraphEditorSettings>()->DefaultCommentNodeTitleColor)
		.AllowPinning(true)
		.EnableTitleBarBubble(true)
		.EnableBubbleCtrls(true)
		.GraphLOD(this, &SGraphNode::GetCurrentLOD)
		.IsGraphNodeHovered(this, &SVoxelMetaGraphNode::IsHovered);

	GetOrAddSlot(ENodeZone::TopCenter)
	.SlotOffset(MakeAttributeSP(&CommentBubble.Get(), &SCommentBubble::GetOffset))
	.SlotSize(MakeAttributeSP(&CommentBubble.Get(), &SCommentBubble::GetSize))
	.AllowScaling(MakeAttributeSP(&CommentBubble.Get(), &SCommentBubble::IsScalingAllowed))
	.VAlign(VAlign_Top)
	[
		CommentBubble
	];

	CreateInputSideAddButton(LeftNodeBox);
	CreateOutputSideAddButton(RightNodeBox);
}

///////////////////////////////////////////////////////////////////////////////

void SVoxelMetaGraphNode::CreateCategorizedPinWidgets()
{
	TArray<UEdGraphPin*> Pins = GraphNode->Pins;

	TMap<FName, TArray<UEdGraphPin*>> MappedSplitPins;
	for (UEdGraphPin* Pin : Pins)
	{
		if (!Pin->ParentPin)
		{
			continue;
		}

		MappedSplitPins.FindOrAdd(Pin->ParentPin->PinName, {}).Add(Pin);
	}

	for (const auto& It : MappedSplitPins)
	{
		for (UEdGraphPin* Pin : It.Value)
		{
			Pins.RemoveSwap(Pin);
		}
	}

	if (const TSharedPtr<const IVoxelNodeDefinition::FNode> Inputs = NodeDefinition->GetInputs())
	{
		for (const TSharedRef<IVoxelNodeDefinition::FNode>& Node : Inputs->Children)
		{
			CreateCategoryPinWidgets(Node, Pins, MappedSplitPins, LeftNodeBox, true);
		}
	}

	if (const TSharedPtr<const IVoxelNodeDefinition::FNode> Outputs = NodeDefinition->GetOutputs())
	{
		for (const TSharedRef<IVoxelNodeDefinition::FNode>& Node : Outputs->Children)
		{
			CreateCategoryPinWidgets(Node, Pins, MappedSplitPins, RightNodeBox, false);
		}
	}

	for (UEdGraphPin* Pin : Pins)
	{
		ensure(Pin->bOrphanedPin);
		AddStandardNodePin(
			Pin,
			{},
			{},
			Pin->Direction == EGPD_Input ? LeftNodeBox : RightNodeBox);
	}
}

void SVoxelMetaGraphNode::CreateCategoryPinWidgets(const TSharedRef<IVoxelNodeDefinition::FNode>& Node, TArray<UEdGraphPin*>& Pins, TMap<FName, TArray<UEdGraphPin*>>& MappedSplitPins, const TSharedPtr<SVerticalBox>& TargetContainer, const bool bInput)
{
	if (Node->NodeState == IVoxelNodeDefinition::ENodeState::Pin)
	{
		if (const auto* PinPtr = Pins.FindByPredicate([&Node](const UEdGraphPin* Pin)
		{
			return Node->Name == Pin->PinName;
		}))
		{
			if (UEdGraphPin* Pin = *PinPtr)
			{
				AddStandardNodePin(Pin, Node->GetFullPath(), Node->Path, TargetContainer);
				Pins.RemoveSwap(Pin);
			}
		}
		if (const auto SplitPinsPtr = MappedSplitPins.Find(Node->Name))
		{
			for (UEdGraphPin* SplitPin : *SplitPinsPtr)
			{
				AddStandardNodePin(SplitPin, Node->GetFullPath(), Node->Path, TargetContainer);
			}

			MappedSplitPins.Remove(Node->Name);
		}
		return;
	}

	const TSharedPtr<SVerticalBox> PinsBox = CreateCategoryWidget(Node->Name, Node->GetFullPath(), Node->Path, Node->Children.Num(), bInput, Node->NodeState == IVoxelNodeDefinition::ENodeState::ArrayCategory);
	TargetContainer->AddSlot()
	.Padding(bInput ? FMargin(Settings->GetInputPinPadding().Left, 0.f, 0.f, 0.f) : FMargin(0.f, 0.f, Settings->GetOutputPinPadding().Right, 0.f))
	.AutoHeight()
	.HAlign(bInput ? HAlign_Left : HAlign_Right)
	.VAlign(VAlign_Center)
	[
		PinsBox.ToSharedRef()
	];

	for (const TSharedRef<IVoxelNodeDefinition::FNode>& InnerNode : Node->Children)
	{
		CreateCategoryPinWidgets(InnerNode, Pins, MappedSplitPins, PinsBox, bInput);
	}
}

void SVoxelMetaGraphNode::AddStandardNodePin(UEdGraphPin* PinToAdd, const FName FullPath, const TArray<FName>& Path, const TSharedPtr<SVerticalBox>& TargetContainer)
{
	if (!ensure(PinToAdd))
	{
		return;
	}

	// ShouldPinBeShown
	if (!ShouldPinBeHidden(PinToAdd))
	{
		return;
	}

	const TSharedPtr<SGraphPin> NewPin = CreatePinWidget(PinToAdd);
	if (!ensure(NewPin.IsValid()))
	{
		return;
	}
	const TSharedRef<SGraphPin> PinWidget = NewPin.ToSharedRef();

	PinWidget->SetOwner(SharedThis(this));

	if (Path.Num() > 0)
	{
		CategoryPins.FindOrAdd(FullPath).Add(PinWidget);
		PinWidget->SetVisibility(MakeAttributeLambda([=]
		{
			if (PinToAdd->bHidden)
			{
				return EVisibility::Collapsed;
			}

			const TSet<FName>& CollapsedCategories = PinToAdd->Direction == EGPD_Input ? GetVoxelNode().CollapsedInputCategories : GetVoxelNode().CollapsedOutputCategories;
			bool bCollapsedCategoriesContains = false;

			FString Category = Path[0].ToString();
			bCollapsedCategoriesContains |= CollapsedCategories.Contains(FName(Category));

			for (int32 Index = 1; Index < Path.Num() && !bCollapsedCategoriesContains; Index++)
			{
				Category += "|" + Path[Index].ToString();

				bCollapsedCategoriesContains |= CollapsedCategories.Contains(FName(Category));
			}

			return
				bCollapsedCategoriesContains &&
				PinToAdd->LinkedTo.Num() == 0
				? EVisibility::Collapsed
				: EVisibility::Visible;
		}));
	}

	TargetContainer->AddSlot()
		.AutoHeight()
		.HAlign(PinToAdd->Direction == EGPD_Input ? HAlign_Left : HAlign_Right)
		.VAlign(VAlign_Center)
		.Padding(PinToAdd->Direction == EGPD_Input ? Settings->GetInputPinPadding() : Settings->GetOutputPinPadding())
		[
			PinWidget
		];
	(PinToAdd->Direction == EGPD_Input ? InputPins : OutputPins).Add(PinWidget);
}

TSharedRef<SVerticalBox> SVoxelMetaGraphNode::CreateCategoryWidget(const FName Name, const FName FullPath, const TArray<FName>& Path, const int32 ArrayNum, const bool bIsInput, const bool bIsArrayCategory) const
{
	const TSharedRef<SButton> Button =
		SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
		.ContentPadding(0)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.ClickMethod(EButtonClickMethod::MouseDown)
		.OnClicked_Lambda([=]
		{
			TSet<FName>& CollapsedCategories = bIsInput ? GetVoxelNode().CollapsedInputCategories : GetVoxelNode().CollapsedOutputCategories;
			if (CollapsedCategories.Contains(FullPath))
			{
				CollapsedCategories.Remove(FullPath);
			}
			else
			{
				CollapsedCategories.Add(FullPath);
			}

			return FReply::Handled();
		});

	Button->SetContent(
		SNew(SImage)
		.Image_Lambda([=, ButtonPtr = &Button.Get()]() -> const FSlateBrush*
		{
			const TSet<FName>& CollapsedCategories = bIsInput ? GetVoxelNode().CollapsedInputCategories : GetVoxelNode().CollapsedOutputCategories;
			if (CollapsedCategories.Contains(FullPath))
			{
				if (ButtonPtr->IsHovered())
				{
					return FAppStyle::Get().GetBrush(STATIC_FNAME("TreeArrow_Collapsed_Hovered"));
				}
				else
				{
					return FAppStyle::Get().GetBrush(STATIC_FNAME("TreeArrow_Collapsed"));
				}
			}
			else
			{
				if (ButtonPtr->IsHovered())
				{
					return FAppStyle::Get().GetBrush(STATIC_FNAME("TreeArrow_Expanded_Hovered"));
				}
				else
				{
					return FAppStyle::Get().GetBrush(STATIC_FNAME("TreeArrow_Expanded"));
				}
			}
		})
		.ColorAndOpacity(FSlateColor::UseForeground()));

	const FString CategoryDisplayName = FName::NameToDisplayString(Name.ToString(), false);
	const TSharedRef<SHorizontalBox> CategoryNameWidget =
		SNew(SHorizontalBox)
		.Cursor(EMouseCursor::Default);
	if (bIsInput)
	{
		CategoryNameWidget->AddSlot()
		.AutoWidth()
		[
			Button
		];
		CategoryNameWidget->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SVoxelDetailText)
			.Text(FText::FromString(CategoryDisplayName))
		];
	}

	TSharedRef<SVerticalBox> Result = SNew(SVerticalBox)
	+ SVerticalBox::Slot()
	.HAlign(bIsInput ? HAlign_Left : HAlign_Right)
	[
		CategoryNameWidget
	];

	bool bAddVisibilityCheck = true;
	if (bIsArrayCategory)
	{
		CategoryNameWidget->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(3.f, 0.f, 0.f, 0.f)
		[
			SNew(SVoxelDetailText)
			.Text(FText::FromString("[" + LexToString(ArrayNum) + "]"))
			.ColorAndOpacity(FLinearColor(0.2f, 0.2f, 0.2f))
		];

		CategoryNameWidget->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(5.f, 0.f, 0.f, 0.f)
		[
			SNew(SButton)
			.ContentPadding(0.0f)
			.Cursor(EMouseCursor::Default)
			.ToolTipText(VOXEL_LOCTEXT("Remove pin"))
			.ButtonStyle(FEditorAppStyle::Get(), "NoBorder")
			.IsEnabled_Lambda([=]
			{
				return IsNodeEditable() && NodeDefinition->CanRemoveFromCategory(Name);
			})
			.OnClicked_Lambda([=]
			{
				const FVoxelTransaction Transaction(GetVoxelNode(), "Remove pin");
				NodeDefinition->RemoveFromCategory(Name);
				return FReply::Handled();
			})
			.Visibility_Lambda([=]
			{
				return GetButtonVisibility(NodeDefinition->CanAddToCategory(Name) || NodeDefinition->CanRemoveFromCategory(Name));
			})
			[
				SNew(SImage)
				.Image(FVoxelEditorStyle::GetBrush(TEXT("Icons.MinusCircle")))
			]
		];

		CategoryNameWidget->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(2.f, 0.f, 0.f, 0.f)
		[
			SNew(SButton)
			.ContentPadding(0.0f)
			.Cursor(EMouseCursor::Default)
			.ToolTipText(VOXEL_LOCTEXT("Add pin"))
			.ButtonStyle(FEditorAppStyle::Get(), "NoBorder")
			.IsEnabled_Lambda([=]
			{
				return IsNodeEditable() && NodeDefinition->CanAddToCategory(Name);
			})
			.OnClicked_Lambda([=]
			{
				const FVoxelTransaction Transaction(GetVoxelNode(), "Add pin");
				NodeDefinition->AddToCategory(Name);
				return FReply::Handled();
			})
			.Visibility_Lambda([=]
			{
				return GetButtonVisibility(NodeDefinition->CanAddToCategory(Name) || NodeDefinition->CanRemoveFromCategory(Name));
			})
			[
				SNew(SImage)
				.Image(FEditorAppStyle::GetBrush(TEXT("Icons.PlusCircle")))
			]
		];

		if (ArrayNum == 0)
		{
			const FMargin Padding = Settings->GetInputPinPadding();

			Result->AddSlot()
			.Padding(Padding + FMargin(Padding.Left, 0.f, 0.f, 0.f))
			.AutoHeight()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(SVoxelDetailText)
				.Visibility_Lambda([=]
				{
					return GetVoxelNode().CollapsedInputCategories.Contains(FullPath) ? EVisibility::Collapsed : EVisibility::Visible;
				})
				.Text(VOXEL_LOCTEXT("No entries"))
				.ColorAndOpacity(FLinearColor(0.2f, 0.2f, 0.2f))
			];

			if (Path.Num() > 1)
			{
				bAddVisibilityCheck = false;
				Result->SetVisibility(MakeAttributeLambda([=]
				{
					const TSet<FName>& CollapsedCategories = bIsInput ? GetVoxelNode().CollapsedInputCategories : GetVoxelNode().CollapsedOutputCategories;

					FString Category = Path[0].ToString();
					if (CollapsedCategories.Contains(FName(Category)))
					{
						return EVisibility::Collapsed;
					}

					for (int32 Index = 1; Index < Path.Num() - 1; Index++)
					{
						Category += "|" + Path[Index].ToString();
						if (CollapsedCategories.Contains(FName(Category)))
						{
							return EVisibility::Collapsed;
						}
					}

					return EVisibility::Visible;
				}));
			}
		}
	}

	if (bAddVisibilityCheck &&
		Path.Num() > 1)
	{
		Result->SetVisibility(MakeAttributeLambda([=]
		{
			const TSet<FName>& CollapsedCategories = bIsInput ? GetVoxelNode().CollapsedInputCategories : GetVoxelNode().CollapsedOutputCategories;

			bool bCollapsedCategoriesContains = false;

			FString Category = Path[0].ToString();
			bCollapsedCategoriesContains |= CollapsedCategories.Contains(FName(Category));

			for (int32 Index = 1; Index < Path.Num() - 1 && !bCollapsedCategoriesContains; Index++)
			{
				Category += "|" + Path[Index].ToString();

				bCollapsedCategoriesContains |= CollapsedCategories.Contains(FName(Category));
			}

			if (!bCollapsedCategoriesContains)
			{
				return EVisibility::Visible;
			}

			const TArray<TWeakPtr<SGraphPin>>* PinsPtr = CategoryPins.Find(FullPath);
			if (!ensure(PinsPtr))
			{
				return EVisibility::Collapsed;
			}
			else
			{
				for (const TWeakPtr<SGraphPin>& Pin : *PinsPtr)
				{
					const TSharedPtr<SGraphPin> PinnedPin = Pin.Pin();
					if (!PinnedPin)
					{
						continue;
					}

					const UEdGraphPin* PinObject = PinnedPin->GetPinObj();
					if (!PinObject)
					{
						continue;
					}

					if (PinObject->LinkedTo.Num() > 0)
					{
						return EVisibility::Visible;
					}
				}

				return EVisibility::Collapsed;
			}
		}));
	}

	if (!bIsInput)
	{
		CategoryNameWidget->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SVoxelDetailText)
			.Text(FText::FromString(CategoryDisplayName))
		];
		CategoryNameWidget->AddSlot()
		.AutoWidth()
		[
			Button
		];
	}

	FString CategoryTooltip;
	if (bIsArrayCategory)
	{
		CategoryTooltip = NodeDefinition->GetPinTooltip(Name);
	}
	else
	{
		CategoryTooltip = NodeDefinition->GetCategoryTooltip(FullPath);
	}

	if (!CategoryTooltip.IsEmpty())
	{
		CategoryNameWidget->SetToolTipText(FText::FromString(CategoryTooltip));
	}

	return Result;
}

///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> SVoxelMetaGraphNode::MakeStatWidget() const
{
	return GetVoxelNode().MakeStatWidget();
}

EVisibility SVoxelMetaGraphNode::GetButtonVisibility(bool bVisible) const
{
	if (SGraphNode::IsAddPinButtonVisible() == EVisibility::Collapsed)
	{
		// LOD is too low
		return EVisibility::Collapsed;
	}

	return bVisible ? EVisibility::Visible : EVisibility::Collapsed;
}

void SVoxelMetaGraphNode::CreateStandardNodeTitleButtons(const TSharedPtr<SHorizontalBox>& TitleBox)
{
	TitleBox->AddSlot()
	.FillWidth(1.f)
	.HAlign(HAlign_Right)
	.VAlign(VAlign_Center)
	.Padding(0.f, 1.f, 7.f, 0.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.Padding(0.f, 0.f, 2.f, 0.f)
		[
			SNew(SButton)
			.ContentPadding(0.0f)
			.Cursor(EMouseCursor::Default)
			.IsEnabled_Lambda([=]
			{
				return IsNodeEditable() && NodeDefinition->CanRemoveInputPin();
			})
			.ButtonStyle(FEditorAppStyle::Get(), "NoBorder")
			.ToolTipText(FText::FromString(NodeDefinition->GetRemovePinTooltip()))
			.OnClicked_Lambda([=]
			{
				const FVoxelTransaction Transaction(GetVoxelNode(), "Remove input pin");
				NodeDefinition->RemoveInputPin();
				return FReply::Handled();
			})
			.Visibility_Lambda([=]
			{
				return GetButtonVisibility(NodeDefinition->CanAddInputPin() || NodeDefinition->CanRemoveInputPin());
			})
			[
				SNew(SImage)
				.Image(FVoxelEditorStyle::GetBrush(TEXT("Icons.MinusCircle")))
			]
		]
		+ SHorizontalBox::Slot()
		[
			SNew(SButton)
			.ContentPadding(0.0f)
			.Cursor(EMouseCursor::Default)
			.IsEnabled_Lambda([=]
			{
				return IsNodeEditable() && NodeDefinition->CanAddInputPin();
			})
			.ButtonStyle(FEditorAppStyle::Get(), "NoBorder")
			.ToolTipText(FText::FromString(NodeDefinition->GetAddPinTooltip()))
			.OnClicked_Lambda([=]
			{
				const FVoxelTransaction Transaction(GetVoxelNode(), "Add input pin");
				NodeDefinition->AddInputPin();
				return FReply::Handled();
			})
			.Visibility_Lambda([=]
			{
				return GetButtonVisibility(NodeDefinition->CanAddInputPin() || NodeDefinition->CanRemoveInputPin());
			})
			[
				SNew(SImage)
				.Image(FEditorAppStyle::GetBrush(TEXT("Icons.PlusCircle")))
			]
		]
	];
}