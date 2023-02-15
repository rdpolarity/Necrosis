// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelHeightSplitterNode.h"
#include "VoxelHeightSplitterNodeImpl.ispc.generated.h"

FVoxelNode_HeightSplitter::FVoxelNode_HeightSplitter()
{
	FixupLayerPins();
}

TVoxelFunction<FVoxelFutureValue(const FVoxelQuery&)> FVoxelNode_HeightSplitter::Compile(FName PinName) const
{
	int32 Layer = -1;
	for (int32 Index = 0; Index < ResultPins.Num(); Index++)
	{
		if (FName(ResultPins[Index]) == PinName)
		{
			Layer = Index;
			break;
		}
	}
	if (Layer == -1)
	{
		VOXEL_MESSAGE(Error, "{0}: invalid node", this);
		return nullptr;
	}

	return [this, Layer, Outer = GetOuter()](const FVoxelQuery& Query) -> FVoxelFutureValue
	{
		(void)Outer;

		TVoxelPinRef<FVoxelFloatBuffer> TargetPin = ResultPins[Layer];
		VOXEL_SETUP_ON_COMPLETE(TargetPin)

		const TValue<TBufferView<float>> HeightBuffer = GetNodeRuntime().GetBufferView(HeightPin, Query);

		TArray<TValue<float>> HeightSplits;
		TArray<TValue<float>> FalloffSplits;
		for (const FLayerPin& LayerPinData : LayerPins)
		{
			HeightSplits.Add(GetNodeRuntime().Get(LayerPinData.Height, Query));
			FalloffSplits.Add(GetNodeRuntime().Get(LayerPinData.Falloff, Query));
		}

		return VOXEL_ON_COMPLETE(AsyncThread, HeightBuffer, HeightSplits, FalloffSplits, Layer)
		{
			for (int32 Index = 1; Index < LayerPins.Num(); Index++)
			{
				const float PreviousHeight = HeightSplits[Index - 1];
				const float CurrentHeight = HeightSplits[Index];
				if (PreviousHeight >= CurrentHeight)
				{
					VOXEL_MESSAGE(Error, "{0}: invalid heights at layer {1} and {2}, heights must be in ascending order",
						this,
						Index - 1,
						Index);
					return {};
				}
			}

			TVoxelArray<float> ReturnValue = FVoxelFloatBuffer::Allocate(HeightBuffer.Num());

			// Middle layers
			if (Layer > 0 &&
				Layer < NumLayerPins)
			{
				const float PreviousHeight = HeightSplits[Layer - 1];
				const float PreviousFalloff = FalloffSplits[Layer - 1];

				const float CurrentHeight = HeightSplits[Layer];
				const float CurrentFalloff = FalloffSplits[Layer];

				ispc::VoxelNode_HeightSplitter_MiddleLayer(
					HeightBuffer.GetData(),
					FVoxelBuffer::AlignNum(HeightBuffer.Num()),
					PreviousHeight,
					PreviousFalloff,
					CurrentHeight,
					CurrentFalloff,
					ReturnValue.GetData());
			}
			// Last layer
			else if (Layer > 0)
			{
				const float PreviousHeight = HeightSplits[Layer - 1];
				const float PreviousFalloff = FalloffSplits[Layer - 1];
				ispc::VoxelNode_HeightSplitter_LastLayer(
					HeightBuffer.GetData(),
					FVoxelBuffer::AlignNum(HeightBuffer.Num()),
					PreviousHeight,
					PreviousFalloff,
					ReturnValue.GetData());
			}
			// First layer
			else
			{
				const float CurrentHeight = HeightSplits[Layer];
				const float CurrentFalloff = FalloffSplits[Layer];
				ispc::VoxelNode_HeightSplitter_FirstLayer(
					HeightBuffer.GetData(),
					FVoxelBuffer::AlignNum(HeightBuffer.Num()),
					CurrentHeight,
					CurrentFalloff,
					ReturnValue.GetData());
			}

			return FVoxelFloatBuffer::MakeCpu(ReturnValue);
		};
	};
}

void FVoxelNode_HeightSplitter::PostSerialize()
{
	Super::PostSerialize();

	FixupLayerPins();
}

#if WITH_EDITOR
void FVoxelNode_HeightSplitter::GetExternalPinsData(TArray<FName>& OutPinNames, TArray<FName>& OutCategoryNames) const
{
	OutPinNames.Add("Height");
	OutPinNames.Add("LayerHeight");
	OutPinNames.Add("LayerFalloff");
	OutPinNames.Add("ResultStrength");

	OutCategoryNames.Add("Layer");
}
#endif

void FVoxelNode_HeightSplitter::FixupLayerPins()
{
	for (const TVoxelPinRef<FVoxelFloatBuffer>& ResultPin : ResultPins)
	{
		RemovePin(ResultPin);
	}
	for (const FLayerPin& Layer : LayerPins)
	{
		RemovePin(Layer.Height);
		RemovePin(Layer.Falloff);
	}

	LayerPins.Reset();
	ResultPins.Reset();

	ResultPins.Add(
		CreateOutputPin<FVoxelFloatBuffer>(
		"Strength_0",
		VOXEL_PIN_METADATA(
			DisplayName("Strength 1"), 
			Tooltip(GetExternalPinTooltip("ResultStrength"))))
	);

	for (int32 Index = 0; Index < NumLayerPins; Index++)
	{
		const FString Category = "Layer " + FString::FromInt(Index);

		LayerPins.Add({
			CreateInputPin<float>(
				FName("Height", Index + 1),
				FVoxelPinValue::Make(Index * 1000.f),
				VOXEL_PIN_METADATA(
					DisplayName("Height"),
					Tooltip(GetExternalPinTooltip("LayerHeight")),
					Category(Category),
					CategoryTooltip(GetExternalCategoryTooltip("Layer")))),

			CreateInputPin<float>(
				FName("Falloff", Index + 1),
				FVoxelPinValue::Make(100.f),
				VOXEL_PIN_METADATA(
					DisplayName("Falloff"),
					Tooltip(GetExternalPinTooltip("LayerFalloff")),
					Category(Category),
					CategoryTooltip(GetExternalCategoryTooltip("Layer"))))
		});

		ResultPins.Add(CreateOutputPin<FVoxelFloatBuffer>(
			FName("Strength", Index + 2),
			VOXEL_PIN_METADATA(
				DisplayName("Strength " + LexToString(Index + 2)),
				Tooltip(GetExternalPinTooltip("ResultStrength")))));
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString FVoxelNode_HeightSplitter::FDefinition::GetAddPinLabel() const
{
	return "Add Layer";
}

FString FVoxelNode_HeightSplitter::FDefinition::GetAddPinTooltip() const
{
	return "Adds an additional layer pin";
}

void FVoxelNode_HeightSplitter::FDefinition::AddInputPin()
{
	Node.NumLayerPins++;
	Node.FixupLayerPins();
}

bool FVoxelNode_HeightSplitter::FDefinition::CanRemoveInputPin() const
{
	return Node.NumLayerPins > 1;
}

void FVoxelNode_HeightSplitter::FDefinition::RemoveInputPin()
{
	Node.NumLayerPins--;
	Node.FixupLayerPins();
}