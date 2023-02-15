// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelNoiseNodes.h"
#include "VoxelMetaGraphSeed.h"

TVoxelFunction<FVoxelFutureValue(const FVoxelQuery&)> FVoxelNode_MakeSeeds::Compile(const FName PinName) const
{
	int32 PinIndex = -1;
	for (int32 Index = 0; Index < ResultPins.Num(); Index++)
	{
		if (FName(ResultPins[Index]) == PinName)
		{
			PinIndex = Index;
			break;
		}
	}
	if (!ensure(PinIndex != -1))
	{
		VOXEL_MESSAGE(Error, "{0}: invalid node", this);
		return nullptr;
	}

	return [this, PinIndex, Outer = GetOuter()](const FVoxelQuery& Query) -> FVoxelFutureValue
	{
		(void)Outer;

		FVoxelPinRef TargetPin = ResultPins[PinIndex];
		VOXEL_SETUP_ON_COMPLETE(TargetPin)

		if (AreMathPinsBuffers())
		{
			const TValue<TBufferView<int32>> Seeds = GetNodeRuntime().GetBufferView<int32>(SeedPin, Query);
			return VOXEL_ON_COMPLETE(AsyncThread, Seeds, PinIndex)
			{
				TVoxelArray<int32> NewSeeds = FVoxelInt32Buffer::Allocate(Seeds.Num());
				for (int32 SeedIndex = 0; SeedIndex < Seeds.Num(); SeedIndex++)
				{
					NewSeeds[SeedIndex] = FVoxelUtilities::MurmurHash(Seeds[SeedIndex], PinIndex);
				}
				return FVoxelSharedPinValue::Make(FVoxelInt32Buffer::MakeCpu(NewSeeds));
			};
		}
		else
		{
			const TValue<int32> Seed = GetNodeRuntime().Get<int32>(SeedPin, Query);
			return VOXEL_ON_COMPLETE(AsyncThread, Seed, PinIndex)
			{
				const int32 NewSeed = FVoxelUtilities::MurmurHash(Seed, PinIndex);
				return FVoxelSharedPinValue::Make(NewSeed);
			};
		}
	};
}

void FVoxelNode_MakeSeeds::FixupSeedPins()
{
	for (const FVoxelPinRef& ResultPin : ResultPins)
	{
		RemovePin(ResultPin);
	}

	ResultPins.Reset();

	const bool bIsBuffer = AreMathPinsBuffers();
	for (int32 Index = 0; Index < NumNewSeeds; Index++)
	{
		if (bIsBuffer)
		{
			ResultPins.Add(
				CreateOutputPin<FVoxelInt32Buffer>(
					FName("Seed", Index + 1),
					VOXEL_PIN_METADATA(
						DisplayName("Seed " + LexToString(Index + 1)),
						Tooltip(GetExternalPinTooltip("ResultSeed")),
						SeedPin),
					EVoxelPinFlags::MathPin)
			);
		}
		else
		{
			ResultPins.Add(
				CreateOutputPin<int32>(
					FName("Seed", Index + 1),
					VOXEL_PIN_METADATA(
						DisplayName("Seed " + LexToString(Index + 1)),
						Tooltip(GetExternalPinTooltip("ResultSeed")),
						SeedPin),
					EVoxelPinFlags::MathPin)
			);
		}
	}
}