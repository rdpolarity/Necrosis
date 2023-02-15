// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelDistanceFieldNodes.h"
#include "Nodes/VoxelPositionNodes.h"
#include "VoxelBrushSubsystem.h"
#include "VoxelDistanceFieldNodesImpl.ispc.generated.h"

TVoxelFutureValue<FVoxelFloatBuffer> FVoxelDistanceFieldCustom::GetDistances(const FVoxelQuery& Query) const
{
	if (!ensure(Node))
	{
		return {};
	}

	return Node->GetNodeRuntime().Get(Node->DensityPin, Query);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_MakeDistanceField, DistanceField)
{
	const TValue<float> Smoothness = Get(SmoothnessPin, Query);
	const TValue<bool> Subtractive = Get(SubtractivePin, Query);
	const TValue<int32> Priority = Get(PriorityPin, Query);
	
	return VOXEL_ON_COMPLETE(AnyThread, Smoothness, Subtractive, Priority)
	{
		const TSharedRef<FVoxelDistanceFieldCustom> Result = MakeShared<FVoxelDistanceFieldCustom>();
		Result->Bounds = FVoxelBox::Infinite;
		Result->Smoothness = Smoothness;
		Result->bIsSubtractive = Subtractive;
		Result->Priority = Priority;
		Result->Node = this;
		Result->Outer = GetOuter();
		return Result;
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_MakeDistanceFieldFromBrushes, DistanceField)
{
	const TValue<FName> LayerName = Get(LayerNamePin, Query);
	
	return VOXEL_ON_COMPLETE(AnyThread, LayerName)
	{
		FindVoxelQueryData(FVoxelBoundsQueryData, BoundsQueryData);

		const TVoxelArray<TSharedPtr<const FVoxelBrushImpl>> Brushes = GetSubsystem<FVoxelBrushSubsystem>().GetBrushes<FVoxelBrushImpl>(
			LayerName,
			BoundsQueryData->Bounds,
			Query.AllocateDependency());

		const TSharedRef<FVoxelDistanceFieldArray> Array = MakeShared<FVoxelDistanceFieldArray>();
		for (const TSharedPtr<const FVoxelBrushImpl>& Brush : Brushes)
		{
			const TSharedPtr<FVoxelDistanceField> DistanceField = Brush->GetDistanceField();
			if (!DistanceField)
			{
				continue;
			}

			Array->DistanceFields.Add(DistanceField.ToSharedRef());
		}
		return Array;
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_CombineDistanceFields, DistanceField)
{
	const TArray<TValue<FVoxelDistanceField>> DistanceFields = Get(DistanceFieldsPins, Query);

	return VOXEL_ON_COMPLETE(AnyThread, DistanceFields)
	{
		const TSharedRef<FVoxelDistanceFieldArray> Result = MakeShared<FVoxelDistanceFieldArray>();
		Result->DistanceFields = DistanceFields;
		return Result;
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_ComputeDensityFromDistanceField, Density)
{
	const TValue<FVoxelDistanceField> DistanceFieldInput = Get(DistanceFieldPin, Query);
	const TValue<float> MinExactDistance = Get(MinExactDistancePin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, DistanceFieldInput, MinExactDistance)
	{
		TVoxelArray<TSharedRef<const FVoxelDistanceField>> DistanceFields;
		DistanceFields.Add(DistanceFieldInput);
		
		{
			VOXEL_SCOPE_COUNTER("Gather distance fields");
			for (int32 Index = 0; Index < DistanceFields.Num(); Index++)
			{
				// TODO
				//ensure(!DistanceFields[Index]->bIsHeightmap);

				const TSharedPtr<const FVoxelDistanceFieldArray> Array = Cast<FVoxelDistanceFieldArray>(DistanceFields[Index]);
				if (!Array)
				{
					continue;
				}

				DistanceFields.Append(Array->DistanceFields);
				DistanceFields.RemoveAtSwap(Index, 1, false);
				Index--;
			}
		}

		FindVoxelQueryData(FVoxelPositionQueryData, PositionQueryData);

		const FVoxelVectorBuffer Positions = PositionQueryData->GetPositions();
		const int32 NumPositions = Positions.Num();

		const TOptional<FVoxelBox> OptionalBounds = Positions.GetBounds();
		if (!OptionalBounds)
		{
			VOXEL_MESSAGE(Error, "{0}: cannot run with unbounded positions", this);
			return {};
		}

		const FVoxelBox Bounds = *OptionalBounds;

		float ExactDistance = 0.f;
		{
			VOXEL_SCOPE_COUNTER("Cull");

			bool bChanged = true;
			while (bChanged)
			{
				bChanged = false;

				ExactDistance = MinExactDistance;
				for (int32 Index = 1; Index < DistanceFields.Num(); Index++)
				{
					ExactDistance = FMath::Max(ExactDistance, DistanceFields[Index]->Smoothness);
				}

				DistanceFields.RemoveAllSwap([&](const TSharedRef<const FVoxelDistanceField>& DistanceField)
				{
					if (DistanceField->Bounds.Extend(ExactDistance).Intersect(Bounds))
					{
						return false;
					}

					bChanged = true;
					return true;
				});
			}
		}

		{
			VOXEL_SCOPE_COUNTER("Sort");
			DistanceFields.StableSort([](const TSharedRef<const FVoxelDistanceField>& A, const TSharedRef<const FVoxelDistanceField>& B)
			{
				if (A->Priority != B->Priority)
				{
					return A->Priority < B->Priority;
				}

				if (A->bIsSubtractive != B->bIsSubtractive)
				{
					// Process subtractive last
					return A->bIsSubtractive < B->bIsSubtractive;
				}

				if (A->Smoothness != B->Smoothness)
				{
					// Process higher smoothness last
					return A->Smoothness < B->Smoothness;
				}

				if (A->Bounds.Max.Z != B->Bounds.Max.Z) { return A->Bounds.Max.Z < B->Bounds.Max.Z; }
				if (A->Bounds.Min.Z != B->Bounds.Min.Z) { return A->Bounds.Min.Z < B->Bounds.Min.Z; }

				if (A->Bounds.Min.X != B->Bounds.Min.X) { return A->Bounds.Min.X < B->Bounds.Min.X; }
				if (A->Bounds.Max.X != B->Bounds.Max.X) { return A->Bounds.Max.X < B->Bounds.Max.X; }

				if (A->Bounds.Min.Y != B->Bounds.Min.Y) { return A->Bounds.Min.Y < B->Bounds.Min.Y; }
				if (A->Bounds.Max.Y != B->Bounds.Max.Y) { return A->Bounds.Max.Y < B->Bounds.Max.Y; }

				return false;
			});
		}
		
		{
			VOXEL_SCOPE_COUNTER("RemoveAt");
			while (DistanceFields.Num() > 0 && DistanceFields[0]->bIsSubtractive)
			{
				DistanceFields.RemoveAt(0);
			}
		}

		if (DistanceFields.Num() == 0)
		{
			return FVoxelFloatBuffer::Constant(1e6);
		}

		TArray<TValue<FVoxelFloatBuffer>> AllDistances;
		for (int32 Index = 0; Index < DistanceFields.Num(); Index++)
		{
			const TSharedRef<const FVoxelDistanceField> DistanceField = DistanceFields[Index];

			FVoxelBox BoundsToCompute;
			if (Index == 0)
			{
				BoundsToCompute = Bounds;
			}
			else
			{
				BoundsToCompute = Bounds.Overlap(DistanceField->Bounds.Extend(ExactDistance));
			}

			TValue<FVoxelFloatBuffer> Distances;
#if 0 // TODO
			if (Query.IsGpu())
			{
				Distances = VOXEL_ON_COMPLETE_CUSTOM(FVoxelFloatBuffer, "Distances", RenderThread, PositionQueryData, DistanceField, BoundsToCompute)
				{
					FVoxelQuery LocalQuery = Query;

					if (const TSharedPtr<FVoxelPositionQueryData> NewPositionQueryData = PositionQueryData->TryCull(BoundsToCompute))
					{
						LocalQuery.Add(NewPositionQueryData.ToSharedRef());
					}

					return DistanceField->GetDistances(LocalQuery);
				};
			}
			else
#endif
			{
				FVoxelQuery LocalQuery = Query;

#if 0 // TODO
				if (const TSharedPtr<FVoxelPositionQueryData> NewPositionQueryData = PositionQueryData->TryCull(BoundsToCompute))
				{
					LocalQuery.Add(NewPositionQueryData.ToSharedRef());
				}
#endif

				Distances = DistanceField->GetDistances(LocalQuery);
			}
			AllDistances.Add(Distances);
		}

		// TODO
		ensure(!Query.IsGpu());

		return VOXEL_ON_COMPLETE(AsyncThread, DistanceFields, NumPositions, AllDistances)
		{
			TArray<TValue<TBufferView<float>>> AllDistancesViews;
			for (const FVoxelFloatBuffer& Distances : AllDistances)
			{
				AllDistancesViews.Add(Distances.MakeView());
			}

			return VOXEL_ON_COMPLETE(AsyncThread, DistanceFields, NumPositions, AllDistancesViews)
			{
				TVoxelArray<float> MergedDistances = FVoxelFloatBuffer::Allocate(NumPositions);
				if (AllDistancesViews[0].IsConstant())
				{
					FVoxelUtilities::SetAll(MergedDistances, AllDistancesViews[0].GetConstant());
				}
				else
				{
					CheckVoxelBuffersNum(MergedDistances, AllDistancesViews[0]);
					FVoxelUtilities::Memcpy(MergedDistances, AllDistancesViews[0].GetRawView());
				}

				for (int32 Index = 1; Index < DistanceFields.Num(); Index++)
				{
					const TSharedRef<const FVoxelDistanceField> DistanceField = DistanceFields[Index];
					const TBufferView<float> Distances = AllDistancesViews[Index];
					CheckVoxelBuffersNum(MergedDistances, Distances);

					ispc::VoxelNode_ComputeDensityFromDistanceField_Merge(
						MergedDistances.GetData(),
						Distances.GetData(),
						Distances.IsConstant(),
						DistanceField->bIsSubtractive,
						DistanceField->Smoothness,
						NumPositions);
				}

				return FVoxelFloatBuffer::MakeCpu(MergedDistances);
			};
		};
	};
}