// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelFoliageInstanceAssetNode.h"
#include "VoxelFoliageUtilities.h"
#include "Nodes/VoxelPositionNodes.h"
#include "VoxelFoliageRandomGenerator.h"

DEFINE_VOXEL_NODE(FVoxelNode_GenerateInstanceAssetFoliageData, ChunkData)
{
	FindVoxelQueryData(FVoxelBoundsQueryData, BoundsQueryData);

	TValue<FVoxelFoliageInstanceTemplateData> Asset = Get(AssetPin, Query);
	const TValue<int32> Seed = Get(SeedPin, Query);
	const TValue<float> Step = Get(GradientStepPin, Query);
	const TValue<float> DistanceMultiplier = Get(DistanceMultiplierPin, Query);
	
	return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, Asset, Seed, Step, DistanceMultiplier)
	{
		if (!Asset->TemplateData)
		{
			VOXEL_MESSAGE(Error, "{0}: Foliage Instance Template is null", this);
			return {};
		}

		FVoxelVectorBuffer HaltonPositionsBuffer;
		HaltonPositionsBuffer.Z = FVoxelFloatBuffer::Constant(0.f);

		if (!FVoxelFoliageUtilities::GenerateHaltonPositions(this, BoundsQueryData->Bounds, Asset->TemplateData->DistanceBetweenPoints * DistanceMultiplier, Seed, HaltonPositionsBuffer.X, HaltonPositionsBuffer.Y))
		{
			return {};
		}

		FVoxelQuery PositionQuery = Query;
		PositionQuery.Add<FVoxelSparsePositionQueryData>().Initialize(HaltonPositionsBuffer);
		TValue<FVoxelFloatBuffer> Height = Get(HeightPin, PositionQuery);

		return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, Asset, Seed, Step, HaltonPositionsBuffer, Height)
		{
			CheckVoxelBuffersNum(HaltonPositionsBuffer, Height);

			FVoxelVectorBuffer PositionsBuffer;
			PositionsBuffer.X = HaltonPositionsBuffer.X;
			PositionsBuffer.Y = HaltonPositionsBuffer.Y;
			PositionsBuffer.Z = Height;

			TValue<TBufferView<FVector>> Positions = PositionsBuffer.MakeView();

			return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, Asset, Seed, Step, HaltonPositionsBuffer, Height, PositionsBuffer, Positions)
			{
				FVoxelQuery PositionsQuery = Query;
				PositionsQuery.Add<FVoxelSparsePositionQueryData>().Initialize(PositionsBuffer);

				TValue<TBufferView<float>> Mask = GetBufferView(MaskPin, PositionsQuery);
				TValue<TBufferView<float>> GradientHeight = FVoxelFoliageUtilities::SplitGradientsBuffer(GetNodeRuntime(), Query, HeightPin, Positions, Step);

				return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, Asset, Seed, Step, Height, GradientHeight, Mask, Positions)
				{
					const FVoxelVectorBufferView Gradient = FVoxelFoliageUtilities::CollapseGradient(GradientHeight, Positions, Step);
					CheckVoxelBuffersNum(Gradient, Positions, Mask);

					float TotalValue = 0.f;
					bool bHasValidMeshes = false;
					for (const FVoxelFoliageMeshProxy& FoliageMesh : Asset->TemplateData->Meshes)
					{
						if (!FoliageMesh.StaticMesh.IsValid())
						{
							continue;
						}

						bHasValidMeshes = true;
						TotalValue += FoliageMesh.Strength;
					}

					if (!bHasValidMeshes)
					{
						VOXEL_MESSAGE(Warning, "{0}: Foliage Instance Template doesn't have valid meshes.", this);
						return {};
					}

					if (FMath::IsNearlyZero(TotalValue))
					{
						VOXEL_MESSAGE(Warning, "{0}: All meshes have strength equal to 0.", this);
						return {};
					}

					struct FMeshData
					{
						int32 Index;
						float Strength;
						TSharedPtr<TVoxelArray<FTransform3f>> Transforms;
					};

					TVoxelArray<FMeshData> MappedStrengths;
					float CheckTotalValue = 0.f;
					for (int32 Index = 0; Index < Asset->TemplateData->Meshes.Num(); Index++)
					{
						if (!Asset->TemplateData->Meshes[Index].StaticMesh.IsValid())
						{
							continue;
						}

						CheckTotalValue += Asset->TemplateData->Meshes[Index].Strength / TotalValue;

						TSharedRef<TVoxelArray<FTransform3f>> Transforms = MakeShared<TVoxelArray<FTransform3f>>();
						Transforms->Reserve(Positions.Num());

						MappedStrengths.Add({
							Index,
							CheckTotalValue,
							Transforms
						});
					}

					ensure(FMath::IsNearlyEqual(CheckTotalValue, 1.f, KINDA_SMALL_NUMBER));
					MappedStrengths.Last().Strength = 1.01f;

					const uint64 SeedHash = FVoxelUtilities::MurmurHash64(Seed);
					int32 InstancesCount = 0;
					for (int32 Index = 0; Index < Positions.Num(); Index++)
					{
						FVoxelFoliageRandomGenerator RandomGenerator(SeedHash, Positions[Index]);

						if (RandomGenerator.GetMaskRestrictionFraction() > Mask[Index] ||
							!Asset->TemplateData->SpawnRestriction.CanSpawn(RandomGenerator, Positions[Index], Gradient[Index], FVector3f::UpVector))
						{
							continue;
						}

						const float MeshStrength = RandomGenerator.GetMeshSelectionFraction();
						int32 MeshIndex = 0;
						for (int32 CheckMeshIndex = 0; CheckMeshIndex < MappedStrengths.Num(); CheckMeshIndex++)
						{
							if (MappedStrengths[CheckMeshIndex].Strength >= MeshStrength)
							{
								MeshIndex = CheckMeshIndex;
								break;
							}
						}

						FMatrix44f Matrix = Asset->TemplateData->GetTransform(
							MappedStrengths[MeshIndex].Index,
							RandomGenerator,
							Positions[Index] - FVector3f(BoundsQueryData->Bounds.Min),
							Gradient[Index],
							FVector3f::UpVector,
							1.f);
						MappedStrengths[MeshIndex].Transforms->Add(FTransform3f(Matrix));
						InstancesCount++;
					}

					const TSharedRef<FVoxelFoliageChunkData> Result = MakeShared<FVoxelFoliageChunkData>();
					Result->ChunkPosition = BoundsQueryData->Bounds.Min;
					Result->InstancesCount = InstancesCount;

					for (int32 Index = 0; Index < MappedStrengths.Num(); Index++)
					{
						FMeshData& Data = MappedStrengths[Index];

						if (Data.Transforms->Num() == 0)
						{
							continue;
						}

						Data.Transforms->Shrink();

						const FVoxelFoliageMeshProxy& FoliageMesh = Asset->TemplateData->Meshes[Data.Index];

						const TSharedRef<FVoxelFoliageChunkMeshData> ResultMeshData = MakeShared<FVoxelFoliageChunkMeshData>();
						ResultMeshData->StaticMesh = FoliageMesh.StaticMesh;
						ResultMeshData->FoliageSettings = FoliageMesh.InstanceSettings;
						ResultMeshData->BodyInstance = FoliageMesh.BodyInstance;
						ResultMeshData->Transforms = Data.Transforms;
						Result->Data.Add(ResultMeshData);
					}

					return Result;
				};
			};
		};
	};
}