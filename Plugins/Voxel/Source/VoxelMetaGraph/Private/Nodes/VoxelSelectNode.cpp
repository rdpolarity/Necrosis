// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelSelectNode.h"
#include "VoxelBufferUtilities.h"

FVoxelNode_Select::FVoxelNode_Select()
{
	GetPin(IndexPin).SetType(FVoxelPinType::Make<bool>());
	FixupValuePins();
}

DEFINE_VOXEL_NODE(FVoxelNode_Select, Result)
{
	const FVoxelPinType IndexType = GetPin(IndexPin).GetType();

	if (IndexType.Is<bool>() ||
		IndexType.Is<int32>())
	{
		const FVoxelFutureValue IndexValue = Get(IndexPin, Query);

		return VOXEL_ON_COMPLETE(AnyThread, IndexType, IndexValue)
		{
			int32 Index;
			if (IndexType.Is<bool>())
			{
				Index = IndexValue.Get<bool>() ? 1 : 0;
			}
			else
			{
				Index = IndexValue.Get<int32>();
			}

			if (!ValuePins.IsValidIndex(Index))
			{
				return {};
			}

			return Get(ValuePins[Index], Query);
		};
	}
	else if (
		IndexType.Is<FVoxelBoolBuffer>() ||
		IndexType.Is<FVoxelInt32Buffer>())
	{
		const TValue<FVoxelBufferView> IndexValue = GetBufferView(IndexPin, Query);

		return VOXEL_ON_COMPLETE(AsyncThread, IndexType, IndexValue)
		{
			TVoxelArray<int32> Indices;
			if (IndexType.Is<FVoxelBoolBuffer>())
			{
				const TBufferView<bool> BoolView = CastChecked<TBufferView<bool>>(*IndexValue);

				FVoxelUtilities::SetNumFast(Indices, BoolView.Num());
				for (int32 Index = 0; Index < BoolView.Num(); Index++)
				{
					Indices[Index] = BoolView[Index] ? 1 : 0;
				}
			}
			else
			{
				Indices = TVoxelArray<int32>(CastChecked<TBufferView<int32>>(*IndexValue).GetRawView());
			}

			if (Indices.Num() == 0)
			{
				return {};
			}

			if (Indices.Num() == 1)
			{
				const int32 Index = Indices[0];

				if (!ValuePins.IsValidIndex(Index))
				{
					return {};
				}

				return Get(ValuePins[Index], Query);
			}

			TArray<TValue<FVoxelBuffer>> Buffers;
			for (const FVoxelPinRef& Pin : ValuePins)
			{
				Buffers.Add(Get<FVoxelBuffer>(Pin, Query));
			}

			const TSharedRef<TVoxelArray<int32>> IndicesData = MakeSharedCopy(MoveTemp(Indices));

			return VOXEL_ON_COMPLETE(AnyThread, Buffers, IndicesData)
			{
				if (Query.IsGpu())
				{
					ensure(false);
					return {};
				}
				else
				{
					TArray<TValue<FVoxelBufferView>> BufferViews;
					for (const TSharedRef<const FVoxelBuffer>& Buffer : Buffers)
					{
						BufferViews.Add(Buffer->MakeGenericView());
					}
					
					return VOXEL_ON_COMPLETE(AsyncThread, Buffers, BufferViews, IndicesData)
					{
						TArray<const FVoxelBuffer*> BufferPointers;
						for (const TSharedRef<const FVoxelBuffer>& Buffer : Buffers)
						{
							BufferPointers.Add(&Buffer.Get());
						}

						FVoxelPinValue Result = FVoxelPinValue(GetPin(ResultPin).GetType());
						Result.Get<FVoxelBuffer>().ForeachBufferArray(BufferPointers, [&](FVoxelTerminalBuffer& Buffer, const TConstVoxelArrayView<const FVoxelTerminalBuffer*> Others)
						{
							TArray<const FVoxelTerminalBufferView*> OtherViews;
							for (const FVoxelTerminalBuffer* Other : Others)
							{
								OtherViews.Add(&Other->MakeView().Get_CheckCompleted());
							}

							Buffer = FVoxelBufferUtilities::Select_Cpu(*IndicesData, OtherViews);
						});
						return FVoxelSharedPinValue(Result);
					};
				}
			};
		};
	}
	else
	{
		ensure(false);
		return {};
	}
}

FVoxelPinTypeSet FVoxelNode_Select::GetPromotionTypes(const FVoxelPin& Pin) const
{
	if (Pin.Name == IndexPin)
	{
		FVoxelPinTypeSet OutTypes;

		OutTypes.Add<bool>();
		OutTypes.Add<FVoxelBoolBuffer>();

		OutTypes.Add<int32>();
		OutTypes.Add<FVoxelInt32Buffer>();

		return OutTypes;
	}
	else
	{
		return FVoxelPinTypeSet::All();
	}
}

void FVoxelNode_Select::PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	if (Pin.Name == IndexPin)
	{
		GetPin(IndexPin).SetType(NewType);

		if (GetPin(IndexPin).GetType().IsBuffer() &&
			!GetPin(ResultPin).GetType().IsBuffer())
		{
			const FVoxelPinType BufferType = GetPin(ResultPin).GetType().GetBufferType();
			if (BufferType.IsBuffer())
			{
				GetPin(ResultPin).SetType(BufferType);
			}
			else
			{
				GetPin(ResultPin).SetType(FVoxelPinType::MakeWildcard());
			}
		}
	}
	else
	{
		GetPin(ResultPin).SetType(NewType);

		for (FVoxelPinRef& ValuePin : ValuePins)
		{
			GetPin(ValuePin).SetType(NewType);
		}

		if (GetPin(IndexPin).GetType().IsBuffer() &&
			!GetPin(ResultPin).GetType().IsBuffer())
		{
			GetPin(IndexPin).SetType(GetPin(IndexPin).GetType().GetInnerType());
		}
	}

	FixupValuePins();
}

void FVoxelNode_Select::PreSerialize()
{
	Super::PreSerialize();

	SerializedIndexType = GetPin(IndexPin).GetType();
}

void FVoxelNode_Select::PostSerialize()
{
	GetPin(IndexPin).SetType(SerializedIndexType);
	FixupValuePins();

	Super::PostSerialize();
}

#if WITH_EDITOR
void FVoxelNode_Select::GetExternalPinsData(TArray<FName>& OutPinNames, TArray<FName>& OutCategoryNames) const
{
	OutPinNames.Add("Index");
	OutPinNames.Add("True");
	OutPinNames.Add("False");
	OutPinNames.Add("Option");
	OutPinNames.Add("Result");
}
#endif

void FVoxelNode_Select::FixupValuePins()
{
	for (const FVoxelPinRef& Pin : ValuePins)
	{
		RemovePin(Pin);
	}
	ValuePins.Reset();

	const FVoxelPinType IndexType = GetPin(IndexPin).GetType();

	if (IndexType.IsWildcard())
	{
		return;
	}

	if (IndexType.Is<bool>() ||
		IndexType.Is<FVoxelBoolBuffer>())
	{
		ValuePins.Add(CreateInputPin(FVoxelPinType::MakeWildcard(), "False", {}));
		ValuePins.Add(CreateInputPin(FVoxelPinType::MakeWildcard(), "True", {}));
	}
	else if (
		IndexType.Is<int32>() ||
		IndexType.Is<FVoxelInt32Buffer>())
	{
		for (int32 Index = 0; Index < NumIntegerOptions; Index++)
		{
			ValuePins.Add(CreateInputPin(
				FVoxelPinType::MakeWildcard(),
				FName("Option", Index + 1),
				{},
				VOXEL_PIN_METADATA(
					DisplayName("Option " + FString::FromInt(Index)),
					Tooltip(GetExternalPinTooltip("Option")))));
		}
	}
	else
	{
		ensure(false);
	}

	for (FVoxelPinRef& ValuePin : ValuePins)
	{
		GetPin(ValuePin).SetType(GetPin(ResultPin).GetType());
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString FVoxelNode_Select::FDefinition::GetAddPinLabel() const
{
	return "Add Option";
}

FString FVoxelNode_Select::FDefinition::GetAddPinTooltip() const
{
	return "Adds a new option to the node";
}

FString FVoxelNode_Select::FDefinition::GetRemovePinTooltip() const
{
	return "Removes last option from the node";
}

bool FVoxelNode_Select::FDefinition::CanAddInputPin() const
{
	return Node.GetPin(Node.IndexPin).GetType().GetInnerType().Is<int32>();
}

void FVoxelNode_Select::FDefinition::AddInputPin()
{
	Node.NumIntegerOptions++;
	Node.FixupValuePins();
}

bool FVoxelNode_Select::FDefinition::CanRemoveInputPin() const
{
	return
		Node.GetPin(Node.IndexPin).GetType().GetInnerType().Is<int32>() &&
		Node.NumIntegerOptions > 2;
}

void FVoxelNode_Select::FDefinition::RemoveInputPin()
{
	Node.NumIntegerOptions--;
	Node.FixupValuePins();
}