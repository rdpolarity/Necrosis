// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelLandmassNodes.h"
#include "VoxelBrushSubsystem.h"
#include "VoxelLandmassUtilities.h"
#include "VoxelLandmassUtilities.ispc.generated.h"

DEFINE_VOXEL_NODE(FVoxelNode_FindLandmassBrushes, Brushes)
{
	FindVoxelQueryData(FVoxelBoundsQueryData, BoundsQueryData);
	const TValue<FName> LayerName = Get(LayerNamePin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, BoundsQueryData, LayerName)
	{
		FVoxelBrushSubsystem& Subsystem = GetSubsystem<FVoxelBrushSubsystem>();

		const TSharedRef<FVoxelLandmassBrushes> Brushes = MakeShared<FVoxelLandmassBrushes>();
		Brushes->Brushes = Subsystem.GetBrushes<FVoxelLandmassBrushImpl>(
			LayerName,
			BoundsQueryData->Bounds,
			Query.AllocateDependency());

		return Brushes;
	};
}

BEGIN_VOXEL_NAMESPACE(MetaGraph)

BEGIN_VOXEL_SHADER_PERMUTATION_DOMAIN(Landmass_AddBrush)
{
	class FFirstBrush : SHADER_PERMUTATION_BOOL("FIRST_BRUSH");
	class FHermiteInterpolation : SHADER_PERMUTATION_BOOL("HERMITE_INTERPOLATION");
}
END_VOXEL_SHADER_PERMUTATION_DOMAIN(Landmass_AddBrush, FFirstBrush, FHermiteInterpolation)

BEGIN_VOXEL_COMPUTE_SHADER(Landmass_AddBrush)
	VOXEL_SHADER_ALL_PARAMETERS_ARE_OPTIONAL()

	VOXEL_SHADER_PARAMETER_CST(uint32, Num)
	VOXEL_SHADER_PARAMETER_CST(FIntVector, BrushSize)
	VOXEL_SHADER_PARAMETER_CST(FMatrix44f, LocalToData)
	VOXEL_SHADER_PARAMETER_CST(float, DataToLocalScale)
	VOXEL_SHADER_PARAMETER_CST(float, DistanceOffset)
	VOXEL_SHADER_PARAMETER_CST(uint32, bSubtractive)
	VOXEL_SHADER_PARAMETER_CST(float, LayerSmoothness)

	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, DistanceField)
	VOXEL_SHADER_PARAMETER_SRV(Buffer<float2>, Normals)

	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, PositionsX)
	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, PositionsY)
	VOXEL_SHADER_PARAMETER_SRV(Buffer<float>, PositionsZ)

	VOXEL_SHADER_PARAMETER_UAV(Buffer<float>, DistanceData)
END_VOXEL_SHADER()

END_VOXEL_NAMESPACE(MetaGraph);

DEFINE_VOXEL_NODE_GPU(FVoxelNode_SampleLandmassBrushes, Distance)
{
	const TValue<FVoxelVectorBuffer> Positions = Get(PositionPin, Query);
	const TValue<float> MaxDistance = Get(MaxDistancePin, Query);
	const TValue<bool> HermiteInterpolation = Get(HermiteInterpolationPin, Query);
	const TValue<float> Smoothness = Get(SmoothnessPin, Query);

	return VOXEL_ON_COMPLETE(RenderThread, Positions, HermiteInterpolation, Smoothness, MaxDistance)
	{
		VOXEL_USE_NAMESPACE(MetaGraph);

		if (Positions.Num() == 0)
		{
			return FVoxelFloatBuffer::Constant(FVoxelLandmassUtilities::DefaultDistance);
		}

		if (Positions.X.IsConstant() ||
			Positions.Y.IsConstant() ||
			Positions.Z.IsConstant())
		{
			VOXEL_MESSAGE(Error, "{0}: cannot run on GPU with constant positions", this);
			return {};
		}

		FVoxelBox Bounds;
		{
			const TOptional<FFloatInterval> IntervalX = Positions.X.GetInterval();
			const TOptional<FFloatInterval> IntervalY = Positions.Y.GetInterval();
			const TOptional<FFloatInterval> IntervalZ = Positions.Z.GetInterval();

			if (!IntervalX ||
				!IntervalY ||
				!IntervalZ)
			{
				VOXEL_MESSAGE(Error, "{0}: cannot run on GPU with unbounded positions", this);
				return {};
			}

			Bounds.Min.X = IntervalX->Min;
			Bounds.Min.Y = IntervalY->Min;
			Bounds.Min.Z = IntervalZ->Min;

			Bounds.Max.X = IntervalX->Max;
			Bounds.Max.Y = IntervalY->Max;
			Bounds.Max.Z = IntervalZ->Max;
		}

		Bounds = Bounds.Extend(MaxDistance);

		FVoxelQuery BrushesQuery = Query;
		BrushesQuery.Add<FVoxelBoundsQueryData>().Bounds = Bounds;
		const TValue<FVoxelLandmassBrushes> Brushes = Get(BrushesPin, BrushesQuery);

		return VOXEL_ON_COMPLETE(RenderThread, Positions, HermiteInterpolation, Smoothness, Brushes)
		{
			TVoxelArray<TSharedPtr<const FVoxelLandmassBrushImpl>> LandmassBrushes = Brushes->Brushes;
			LandmassBrushes.Sort([](const TSharedPtr<const FVoxelLandmassBrushImpl>& A, const TSharedPtr<const FVoxelLandmassBrushImpl>& B)
			{
				if (A->Brush.bInvert != B->Brush.bInvert)
				{
					return A->Brush.bInvert > B->Brush.bInvert;
				}
				
				if (A->Brush.Priority != B->Brush.Priority)
				{
					return A->Brush.Priority < B->Brush.Priority;
				}

				if (A->GetBounds().Max.Z != B->GetBounds().Max.Z) { return A->GetBounds().Max.Z < B->GetBounds().Max.Z; }
				if (A->GetBounds().Min.Z != B->GetBounds().Min.Z) { return A->GetBounds().Min.Z < B->GetBounds().Min.Z; }

				if (A->GetBounds().Min.X != B->GetBounds().Min.X) { return A->GetBounds().Min.X < B->GetBounds().Min.X; }
				if (A->GetBounds().Max.X != B->GetBounds().Max.X) { return A->GetBounds().Max.X < B->GetBounds().Max.X; }

				if (A->GetBounds().Min.Y != B->GetBounds().Min.Y) { return A->GetBounds().Min.Y < B->GetBounds().Min.Y; }
				if (A->GetBounds().Max.Y != B->GetBounds().Max.Y) { return A->GetBounds().Max.Y < B->GetBounds().Max.Y; }

				return true;
			});

			// Can't apply subtractive over nothing
			while (LandmassBrushes.Num() > 0 && LandmassBrushes[0]->Brush.bInvert)
			{
				LandmassBrushes.RemoveAt(0);
			}

			if (LandmassBrushes.Num() == 0)
			{
				return FVoxelFloatBuffer::Constant(FVoxelLandmassUtilities::DefaultDistance);
			}

			const FVoxelRDGBuffer DistanceData = MakeVoxelRDGBuffer_float(DistanceData, Positions.Num());

			for (int32 BrushIndex = 0; BrushIndex < LandmassBrushes.Num(); BrushIndex++)
			{
				const FVoxelLandmassBrushImpl& Brush = *LandmassBrushes[BrushIndex];

				BEGIN_VOXEL_SHADER_CALL(Landmass_AddBrush)
				{
					PermutationDomain.Set<FFirstBrush>(BrushIndex == 0);
					PermutationDomain.Set<FHermiteInterpolation>(HermiteInterpolation);

					Parameters.Num = Positions.Num();
					Parameters.BrushSize = Brush.Data->Size;
					Parameters.LocalToData = Brush.LocalToData;
					Parameters.DataToLocalScale = Brush.DataToLocalScale;
					Parameters.DistanceOffset = Brush.Brush.DistanceOffset;
					Parameters.bSubtractive = Brush.Brush.bInvert;
					Parameters.LayerSmoothness = Smoothness;

					Parameters.DistanceField = FVoxelRDGBuffer(Brush.Data->GetDistanceField_RenderThread());
					Parameters.Normals = FVoxelRDGBuffer(Brush.Data->GetNormals_RenderThread());

					Parameters.PositionsX = Positions.X.GetGpuBuffer();
					Parameters.PositionsY = Positions.Y.GetGpuBuffer();
					Parameters.PositionsZ = Positions.Z.GetGpuBuffer();

					Parameters.DistanceData = DistanceData;

					Execute(FComputeShaderUtils::GetGroupCount(Positions.Num(), 64));
				}
				END_VOXEL_SHADER_CALL()
			}

			FVoxelFloatBuffer Result;
			Result = FVoxelBufferData::MakeGpu(FVoxelPinType::Make<float>(), DistanceData);
			return Result;
		};
	};
}
	
DEFINE_VOXEL_NODE_CPU(FVoxelNode_SampleLandmassBrushes, Distance)
{
	const TValue<TBufferView<FVector>> Positions = GetBufferView(PositionPin, Query);
	const TValue<float> MaxDistance = Get(MaxDistancePin, Query);
	const TValue<bool> HermiteInterpolation = Get(HermiteInterpolationPin, Query);
	const TValue<float> Smoothness = Get(SmoothnessPin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, Positions, HermiteInterpolation, Smoothness, MaxDistance)
	{
		if (Positions.Num() == 0)
		{
			return FVoxelFloatBuffer::Constant(FVoxelLandmassUtilities::DefaultDistance);
		}

		FVoxelBox Bounds;
		{
			VOXEL_SCOPE_COUNTER("Bounds");

			const FFloatInterval X = FVoxelUtilities::GetMinMax(Positions.X.GetRawView());
			const FFloatInterval Y = FVoxelUtilities::GetMinMax(Positions.Y.GetRawView());
			const FFloatInterval Z = FVoxelUtilities::GetMinMax(Positions.Z.GetRawView());

			Bounds.Min.X = X.Min;
			Bounds.Max.X = X.Max;

			Bounds.Min.Y = Y.Min;
			Bounds.Max.Y = Y.Max;

			Bounds.Min.Z = Z.Min;
			Bounds.Max.Z = Z.Max;
		}

		Bounds = Bounds.Extend(MaxDistance);

		FVoxelQuery BrushesQuery = Query;
		BrushesQuery.Add<FVoxelBoundsQueryData>().Bounds = Bounds;
		const TValue<FVoxelLandmassBrushes> Brushes = Get(BrushesPin, BrushesQuery);

		return VOXEL_ON_COMPLETE(AsyncThread, Positions, HermiteInterpolation, Smoothness, Brushes)
		{
			TVoxelArray<TSharedPtr<const FVoxelLandmassBrushImpl>> LandmassBrushes = Brushes->Brushes;
			LandmassBrushes.Sort([](const TSharedPtr<const FVoxelLandmassBrushImpl>& A, const TSharedPtr<const FVoxelLandmassBrushImpl>& B)
			{
				if (A->Brush.bInvert != B->Brush.bInvert)
				{
					return A->Brush.bInvert > B->Brush.bInvert;
				}
				
				if (A->Brush.Priority != B->Brush.Priority)
				{
					return A->Brush.Priority < B->Brush.Priority;
				}

				if (A->GetBounds().Max.Z != B->GetBounds().Max.Z) { return A->GetBounds().Max.Z < B->GetBounds().Max.Z; }
				if (A->GetBounds().Min.Z != B->GetBounds().Min.Z) { return A->GetBounds().Min.Z < B->GetBounds().Min.Z; }

				if (A->GetBounds().Min.X != B->GetBounds().Min.X) { return A->GetBounds().Min.X < B->GetBounds().Min.X; }
				if (A->GetBounds().Max.X != B->GetBounds().Max.X) { return A->GetBounds().Max.X < B->GetBounds().Max.X; }

				if (A->GetBounds().Min.Y != B->GetBounds().Min.Y) { return A->GetBounds().Min.Y < B->GetBounds().Min.Y; }
				if (A->GetBounds().Max.Y != B->GetBounds().Max.Y) { return A->GetBounds().Max.Y < B->GetBounds().Max.Y; }

				return true;
			});

			// Can't apply subtractive over nothing
			while (LandmassBrushes.Num() > 0 && LandmassBrushes[0]->Brush.bInvert)
			{
				LandmassBrushes.RemoveAt(0);
			}

			if (LandmassBrushes.Num() == 0)
			{
				return FVoxelFloatBuffer::Constant(FVoxelLandmassUtilities::DefaultDistance);
			}

			VOXEL_SCOPE_COUNTER_FORMAT("NumBrushes=%d Num=%d, bHermiteInterpolation=%s",
				LandmassBrushes.Num(),
				Positions.Num(),
				*LexToString(HermiteInterpolation));

			TVoxelArray<float> Buffer = FVoxelFloatBuffer::Allocate(Positions.Num());
			for (int32 BrushIndex = 0; BrushIndex < LandmassBrushes.Num(); BrushIndex++)
			{
				const FVoxelLandmassBrushImpl& Brush = *LandmassBrushes[BrushIndex];
				VOXEL_SCOPE_COUNTER("Landmass_AddBrush");

				const FIntVector Size = Brush.Data->Size;
				check(Brush.Data->DistanceField.Num() == Size.X * Size.Y * Size.Z);
				check(Brush.Data->Normals.Num() == Size.X * Size.Y * Size.Z);

				ispc::Landmass_AddBrush(
					Positions.X.GetData(),
					Positions.Y.GetData(),
					Positions.Z.GetData(),
					Positions.X.IsConstant(),
					Positions.Y.IsConstant(),
					Positions.Z.IsConstant(),
					Positions.Num(),
					GetISPCValue(Size),
					Brush.Data->DistanceField.GetData(),
					Brush.Data->Normals.GetData(),
					GetISPCValue(Brush.LocalToData),
					Brush.DataToLocalScale,
					Brush.Brush.DistanceOffset,
					Brush.Brush.bInvert,
					Buffer.GetData(),
					BrushIndex == 0,
					HermiteInterpolation,
					Smoothness);
			}
			return FVoxelFloatBuffer::MakeCpu(Buffer);
		};
	};
}