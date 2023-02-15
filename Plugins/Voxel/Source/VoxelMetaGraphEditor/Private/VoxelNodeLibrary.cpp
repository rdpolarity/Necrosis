// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelNodeLibrary.h"
#include "Nodes/VoxelPassthroughNodes.h"

VOXEL_RUN_ON_STARTUP_EDITOR(RegisterVoxelNodeLibrary)
{
	(void)FVoxelNodeLibrary::GetNodes();
}

FVoxelNodeLibrary::FVoxelNodeLibrary()
{
	VOXEL_FUNCTION_COUNTER();

	for (UScriptStruct* Struct : GetDerivedStructs<FVoxelNode>())
	{
		if (Struct->HasMetaData("Abstract"))
		{
			continue;
		}

		const TVoxelInstancedStruct<FVoxelNode>& Node = Nodes.Add(Struct, Struct);

		if (Struct->HasMetaData(STATIC_FNAME("Autocast")))
		{
			TVoxelInstancedStruct<FVoxelNode> NodeCopy = Node;

			FVoxelPin* FirstInputPin = nullptr;
			FVoxelPin* FirstOutputPin = nullptr;
			for (FVoxelPin& Pin : NodeCopy->GetPins())
			{
				if (Pin.bIsInput && !FirstInputPin)
				{
					FirstInputPin = &Pin;
				}
				if (!Pin.bIsInput && !FirstOutputPin)
				{
					FirstOutputPin = &Pin;
				}
			}
			check(FirstInputPin);
			check(FirstOutputPin);

			FVoxelPinTypeSet InputTypes;
			if (FirstInputPin->IsPromotable())
			{
				InputTypes = NodeCopy->GetPromotionTypes(*FirstInputPin);
				ensure(!InputTypes.IsAll());
			}
			else
			{
				InputTypes.Add(FirstInputPin->GetType());
			}

			FVoxelPinTypeSet OutputTypes;
			if (FirstOutputPin->IsPromotable())
			{
				OutputTypes = NodeCopy->GetPromotionTypes(*FirstOutputPin);
				ensure(!OutputTypes.IsAll());
			}
			else
			{
				OutputTypes.Add(FirstOutputPin->GetType());
			}

			TSet<TPair<FVoxelPinType, FVoxelPinType>> Pairs;
			for (const FVoxelPinType& InputType : InputTypes.GetTypes())
			{
				if (FirstInputPin->IsPromotable())
				{
					NodeCopy->PromotePin(*FirstInputPin, InputType);
				}

				for (const FVoxelPinType& OutputType : OutputTypes.GetTypes())
				{
					if (FirstOutputPin->IsPromotable())
					{
						NodeCopy->PromotePin(*FirstOutputPin, OutputType);
					}

					Pairs.Add({ FirstInputPin->GetType(), FirstOutputPin->GetType() });
				}
			}

			for (const TPair<FVoxelPinType, FVoxelPinType>& Pair : Pairs)
			{
				ensure(!CastNodes.Contains(Pair));
				CastNodes.Add(Pair, Struct);
			}
		}

		if (Struct->IsChildOf(FVoxelNode_Make::StaticStruct()))
		{
			const FVoxelPin& OutputPin = Node->GetUniqueOutputPin();

			FVoxelPinTypeSet Types;
			if (OutputPin.IsPromotable())
			{
				Types = Node->GetPromotionTypes(OutputPin);
				ensure(!Types.IsAll());
			}
			else
			{
				Types.Add(OutputPin.GetType());
			}

			for (const FVoxelPinType& Type : Types.GetTypes())
			{
				ensure(!MakeNodes.Contains(Type));
				MakeNodes.Add(Type, Struct);
			}
		}

		if (Struct->IsChildOf(FVoxelNode_Break::StaticStruct()))
		{
			const FVoxelPin& InputPin = Node->GetUniqueInputPin();

			FVoxelPinTypeSet Types;
			if (InputPin.IsPromotable())
			{
				Types = Node->GetPromotionTypes(InputPin);
				ensure(!Types.IsAll());
			}
			else
			{
				Types.Add(InputPin.GetType());
			}
			
			for (const FVoxelPinType& Type : Types.GetTypes())
			{
				ensure(!BreakNodes.Contains(Type));
				BreakNodes.Add(Type, Struct);
			}
		}
	}

	{
		TMap<FVoxelPinType, int32> Map;
		for (const auto& It : Nodes)
		{
			if (It.Key->HasMetaData("Internal"))
			{
				continue;
			}

			for (const FVoxelPin& Pin : It.Value->GetPins())
			{
				if (!Pin.IsPromotable())
				{
					Map.FindOrAdd(Pin.GetType())++;
					continue;
				}

				const FVoxelPinTypeSet Types = It.Value->GetPromotionTypes(Pin);
				if (Types.IsAll())
				{
					continue;
				}

				for (const FVoxelPinType& Type : Types.GetTypes())
				{
					Map.FindOrAdd(Type)++;
				}
			}
		}

		Map.Remove(FVoxelPinType::MakeWildcard());

		Map.GenerateKeyArray(ParameterTypes);

		ParameterTypes.Sort([&](const FVoxelPinType& A, const FVoxelPinType& B)
		{
			if (Map[A] != Map[B])
			{
				return Map[A] > Map[B];
			}

			return A.ToString() < B.ToString();
		});
	}
}

const FVoxelNodeLibrary& FVoxelNodeLibrary::Get()
{
	static FVoxelNodeLibrary Library;
	return Library;
}