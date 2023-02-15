// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphNode.h"
#include "VoxelGraphSchema.h"
#include "VoxelNodeDefinition.h"

TSharedRef<SWidget> UVoxelMetaGraphNode::MakeStatWidget() const
{
	const FLinearColor Color = FLinearColor(FColor::Orange) * 0.6f;

	return
		SNew(SOverlay)
		.Visibility_Lambda([=]
		{
			if (!FVoxelTaskStat::bStaticRecordStats)
			{
				return EVisibility::Collapsed;
			}

			for (int32 Index = 0; Index < FVoxelTaskStat::Count; Index++)
			{
				if (Times[Index].Inclusive > 0)
				{
					return EVisibility::Visible;
				}
			}
			return EVisibility::Collapsed;
		})
		+ SOverlay::Slot()
		[
			SNew(SImage)
			.Image(FVoxelEditorStyle::GetBrush("Node.Stats.TitleGloss"))
			.ColorAndOpacity(Color)
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(FVoxelEditorStyle::GetBrush("Node.Stats.ColorSpill"))
			.Padding(FMargin(10.f, 5.f, 20.f, 3.f))
			.BorderBackgroundColor(Color)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.Padding(FMargin(0.f, 0.f, 0.f, 3.f))
				[
					SNew(SHorizontalBox)
					.Visibility_Lambda([=]
					{
						return GetCpuTime().Get() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
					})
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Top)
					.Padding(FMargin(0.f, 0.f, 4.f, 0.f))
					.AutoWidth()
					[
						SNew(SImage)
						.Image(FEditorAppStyle::GetBrush("GraphEditor.Timeline_16x"))
						.ColorAndOpacity(Color)
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SBox)
						.MinDesiredWidth(55.f)
						[
							SNew(STextBlock)
							.ColorAndOpacity(FCoreStyle::Get().GetColor("ErrorReporting.ForegroundColor"))
							.Text_Lambda([=]
							{
								if (!FVoxelTaskStat::bStaticDetailedStats)
								{
									return FText::Format(VOXEL_LOCTEXT("CPU {0}"), FVoxelUtilities::ConvertToTimeText(GetCpuTime().Get()));
								}

								FString String = "CPU";
								if (Times[FVoxelTaskStat::CpuGameThread].Get() > 0)
								{
									String += " Game " + FVoxelUtilities::ConvertToTimeText(Times[FVoxelTaskStat::CpuGameThread].Get()).ToString();
								}
								if (Times[FVoxelTaskStat::CpuRenderThread].Get() > 0)
								{
									String += " Render " + FVoxelUtilities::ConvertToTimeText(Times[FVoxelTaskStat::CpuRenderThread].Get()).ToString();
								}
								if (Times[FVoxelTaskStat::CpuAsyncThread].Get() > 0)
								{
									String += " Async " + FVoxelUtilities::ConvertToTimeText(Times[FVoxelTaskStat::CpuAsyncThread].Get()).ToString();
								}
								return FText::FromString(String);
							})
						]
					]
				]
				+ SVerticalBox::Slot()
				.Padding(FMargin(0.f, 0.f, 0.f, 3.f))
				[
					SNew(SHorizontalBox)
					.Visibility_Lambda([=]
					{
						return GetGpuTime().Get() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
					})
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Top)
					.Padding(FMargin(0.f, 0.f, 4.f, 0.f))
					.AutoWidth()
					[
						SNew(SImage)
						.Image(FEditorAppStyle::GetBrush("GraphEditor.Macro.DoN_16x"))
						.ColorAndOpacity(Color)
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SBox)
						.MinDesiredWidth(190.f)
						[
							SNew(STextBlock)
							.ColorAndOpacity(FCoreStyle::Get().GetColor("ErrorReporting.ForegroundColor"))
							.Text_Lambda([=]
							{
								return FText::Format(VOXEL_LOCTEXT("GPU {0}"), FVoxelUtilities::ConvertToTimeText(GetGpuTime().Get()));
							})
						]
					]
				]
				+ SVerticalBox::Slot()
				.Padding(FMargin(0.f, 0.f, 0.f, 3.f))
				[
					SNew(SHorizontalBox)
					.Visibility_Lambda([=]
					{
						return GetCopyTime().Get() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
					})
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Top)
					.Padding(FMargin(0.f, 0.f, 4.f, 0.f))
					.AutoWidth()
					[
						SNew(SImage)
						.Image(FEditorAppStyle::GetBrush("GraphEditor.Timeline_16x"))
						.ColorAndOpacity(Color)
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SBox)
						.MinDesiredWidth(55.f)
						[
							SNew(STextBlock)
							.ColorAndOpacity(FCoreStyle::Get().GetColor("ErrorReporting.ForegroundColor"))
							.Text_Lambda([=]
							{
								if (!FVoxelTaskStat::bStaticDetailedStats)
								{
									return FText::Format(VOXEL_LOCTEXT("Copy {0}"), FVoxelUtilities::ConvertToTimeText(GetCopyTime().Get()));
								}

								FString String = "Copy";
								if (Times[FVoxelTaskStat::CopyCpuToGpu].Get() > 0)
								{
									String += " CPU to GPU " + FVoxelUtilities::ConvertToTimeText(Times[FVoxelTaskStat::CopyCpuToGpu].Get()).ToString();
								}
								if (Times[FVoxelTaskStat::CopyGpuToCpu].Get() > 0)
								{
									String += " GPU to CPU " + FVoxelUtilities::ConvertToTimeText(Times[FVoxelTaskStat::CopyGpuToCpu].Get()).ToString();
								}
								return FText::FromString(String);
							})
						]
					]
				]
			]
		];
}

void UVoxelMetaGraphNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	if (!FromPin)
	{
		return;
	}

	const UVoxelGraphSchema* Schema = GetSchema();

	// Check non-promotable and same type (ignore container types) pins first
	for (UEdGraphPin* Pin : Pins)
	{
		const FPinConnectionResponse Response = Schema->CanCreateConnection(FromPin, Pin);

		if (Response.Response == CONNECT_RESPONSE_MAKE ||
			(Response.Response == CONNECT_RESPONSE_MAKE_WITH_PROMOTION && FVoxelPinType(FromPin->PinType).GetInnerType() == FVoxelPinType(Pin->PinType).GetInnerType()))
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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<IVoxelNodeDefinition> UVoxelMetaGraphNode::GetNodeDefinition()
{
	return MakeShared<IVoxelNodeDefinition>();
}

bool UVoxelMetaGraphNode::TryMigratePin(UEdGraphPin* OldPin, UEdGraphPin* NewPin) const
{
	if (!FVoxelPinType(OldPin->PinType).IsWildcard() &&
		!FVoxelPinType(NewPin->PinType).IsWildcard() &&
		FVoxelPinType(OldPin->PinType) != FVoxelPinType(NewPin->PinType))
	{
		return false;
	}

	NewPin->MovePersistentDataFromOldPin(*OldPin);
	return true;
}

bool UVoxelMetaGraphNode::TryMigrateDefaultValue(const UEdGraphPin* OldPin, UEdGraphPin* NewPin) const
{
	const FVoxelPinValue OldValue = FVoxelPinValue::MakeFromPinDefaultValue(*OldPin);

	FVoxelPinValue NewValue(FVoxelPinType(NewPin->PinType).GetExposedType());
	if (!NewValue.ImportFromUnrelated(OldValue))
	{
		return false;
	}

	NewPin->DefaultValue = NewValue.ExportToString();
	return true;
}

void UVoxelMetaGraphNode::PostReconstructNode()
{
	// Sanitize default values
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->bOrphanedPin)
		{
			continue;
		}

		const FVoxelPinType Type = FVoxelPinType(Pin->PinType).GetExposedType();
		if (Type.IsObject())
		{
			if (!Pin->DefaultValue.IsEmpty() &&
				!Pin->DefaultObject)
			{
				Pin->DefaultObject = LoadObject<UObject>(nullptr, *Pin->DefaultValue);
			}
			Pin->DefaultValue.Reset();
			continue;
		}
		Pin->DefaultObject = nullptr;

		if (Type.IsWildcard() ||
			!ensure(Type.IsValid()))
		{
			Pin->DefaultValue.Reset();
			continue;
		}

		if (!FVoxelPinValue(Type).ImportFromString(Pin->DefaultValue))
		{
			Pin->DefaultValue = Pin->AutogeneratedDefaultValue;
		}
	}
}