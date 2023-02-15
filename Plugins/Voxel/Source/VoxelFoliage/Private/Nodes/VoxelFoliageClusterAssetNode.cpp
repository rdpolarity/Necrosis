// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelFoliageClusterAssetNode.h"
#include "VoxelFoliageUtilities.h"
#include "Nodes/VoxelPositionNodes.h"
#include "VoxelFoliageRandomGenerator.h"

DEFINE_VOXEL_NODE(FVoxelNode_GenerateClusterAssetFoliageData, ChunkData)
{
	FindVoxelQueryData(FVoxelBoundsQueryData, BoundsQueryData);

	TValue<FVoxelFoliageClusterTemplateData> Asset = Get(AssetPin, Query);
	const TValue<int32> Seed = Get(SeedPin, Query);
	const TValue<float> Step = Get(GradientStepPin, Query);
	const TValue<float> DistanceMultiplier = Get(DistanceMultiplierPin, Query);
	const TValue<float> MaxHeightDifferenceMultiplier = Get(MaxHeightDifferenceMultiplierPin, Query);
	const TValue<float> MaskOccupancyMultiplier = Get(MaskOccupancyMultiplierPin, Query);
	const FVector3f WorldUp = FVector3f::UpVector;
	
	return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, Asset, Seed, Step, DistanceMultiplier, MaxHeightDifferenceMultiplier, MaskOccupancyMultiplier, WorldUp)
	{
		TSharedPtr<TVoxelArray<float>> TotalValues;
		if (!CalculateTotalValues(*Asset, TotalValues))
		{
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
		TValue<FVoxelFloatBuffer> ClustersHeight = Get(HeightPin, PositionQuery);

		return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, Asset, Seed, Step, MaxHeightDifferenceMultiplier, MaskOccupancyMultiplier, WorldUp, HaltonPositionsBuffer, ClustersHeight, TotalValues)
		{
			CheckVoxelBuffersNum(HaltonPositionsBuffer, ClustersHeight);

			FVoxelVectorBuffer ClustersPositionBuffer;
			ClustersPositionBuffer.X = HaltonPositionsBuffer.X;
			ClustersPositionBuffer.Y = HaltonPositionsBuffer.Y;
			ClustersPositionBuffer.Z = ClustersHeight;

			const TValue<TBufferView<FVector>> ClustersPositionView = ClustersPositionBuffer.MakeView();

			return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, Asset, Seed, Step, MaxHeightDifferenceMultiplier, MaskOccupancyMultiplier, WorldUp, HaltonPositionsBuffer, ClustersHeight, ClustersPositionBuffer, ClustersPositionView, TotalValues)
			{
				const TValue<TBufferView<float>> ClustersGradientHeight = FVoxelFoliageUtilities::SplitGradientsBuffer(GetNodeRuntime(), Query, HeightPin, ClustersPositionView, Step);

				FVoxelQuery PositionsQuery = Query;
				PositionsQuery.Add<FVoxelSparsePositionQueryData>().Initialize(ClustersPositionBuffer);
				TValue<TBufferView<float>> ClustersMask = GetBufferView(MaskPin, PositionsQuery);

				return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, Asset, Seed, Step, MaxHeightDifferenceMultiplier, MaskOccupancyMultiplier, WorldUp, ClustersHeight, ClustersGradientHeight, ClustersPositionView, ClustersMask, TotalValues)
				{
					const FVoxelVectorBufferView ClustersGradient = FVoxelFoliageUtilities::CollapseGradient(ClustersGradientHeight, ClustersPositionView, Step);

					CheckVoxelBuffersNum(ClustersGradient, ClustersPositionView, ClustersMask);

					const uint64 SeedHash = FVoxelUtilities::MurmurHash64(Seed);
					const int32 InstancesCount = CalculateInstancesCount(*Asset, SeedHash, ClustersPositionView, ClustersGradient, ClustersMask);
					if (InstancesCount == 0)
					{
						return {};
					}

					TVoxelArray<float> InstancesPositionsX = FVoxelFloatBuffer::Allocate(InstancesCount);
					TVoxelArray<float> InstancesPositionsY = FVoxelFloatBuffer::Allocate(InstancesCount);
					TSharedRef<TVoxelArray<int32>> InstancesEntryIndexes = MakeShared<TVoxelArray<int32>>(FVoxelInt32Buffer::Allocate(InstancesCount));
					TSharedRef<TVoxelArray<int32>> InstancesClusterPositionIndexes = MakeShared<TVoxelArray<int32>>(FVoxelInt32Buffer::Allocate(InstancesCount));
					TSharedRef<TVoxelArray<int32>> InstancesCountPerCluster = MakeShared<TVoxelArray<int32>>(FVoxelInt32Buffer::Allocate(ClustersPositionView.Num()));

					TSharedRef<TVoxelArray<int32>> InstancesCountPerEntry = MakeShared<TVoxelArray<int32>>(FVoxelInt32Buffer::Allocate(Asset->TemplateData->Entries.Num()));
					for (int32 Index = 0; Index < Asset->TemplateData->Entries.Num(); Index++)
					{
						(*InstancesCountPerEntry)[Index] = 0;
					}

					int32 GlobalInstanceIndex = 0;
					const auto AddInstance = [&](const FVector3f& RelativePosition, int32 EntryIndex, int32 ClusterPositionIndex)
					{
						InstancesPositionsX[GlobalInstanceIndex] = RelativePosition.X + ClustersPositionView[ClusterPositionIndex].X;
						InstancesPositionsY[GlobalInstanceIndex] = RelativePosition.Y + ClustersPositionView[ClusterPositionIndex].Y;
						(*InstancesEntryIndexes)[GlobalInstanceIndex] = EntryIndex;
						(*InstancesClusterPositionIndexes)[GlobalInstanceIndex] = ClusterPositionIndex;
						GlobalInstanceIndex++;
					};

					for (int32 Index = 0; Index < ClustersPositionView.Num(); Index++)
					{
						FVoxelFoliageRandomGenerator RandomGenerator(SeedHash, ClustersPositionView[Index]);

						if (RandomGenerator.GetMaskRestrictionFraction() > ClustersMask[Index] ||
							!Asset->TemplateData->SpawnRestriction.CanSpawn(RandomGenerator, ClustersPositionView[Index], ClustersGradient[Index], WorldUp))
						{
							continue;
						}

						int32 ClusterInstancesCount = 0;
						for (int32 EntryIndex = 0; EntryIndex < Asset->TemplateData->Entries.Num(); EntryIndex++)
						{
							const FVoxelFoliageClusterEntryProxy& Entry = Asset->TemplateData->Entries[EntryIndex];

							int32 EntryInstancesCount = 0;

							if (FMath::IsNearlyZero(Entry.SpawnRadius.Max))
							{
								AddInstance(FVector3f::ZeroVector, EntryIndex, Index);
								EntryInstancesCount++;
							}
							else
							{
								const int32 NumberOfInstances = Entry.InstancesCount.Interpolate(RandomGenerator.GetInstancesCountFraction());

								float RadialOffsetRadians = FMath::DegreesToRadians(Entry.RadialOffset);
								FVoxelFloatInterval RadialOffset = FVoxelFloatInterval(RadialOffsetRadians * -0.5f, RadialOffsetRadians * 0.5f);

								for (int32 InstanceIndex = 0; InstanceIndex < NumberOfInstances; InstanceIndex++)
								{
									const float AngleInRadians = float(InstanceIndex) / float(NumberOfInstances) * 2.f * PI + RadialOffset.Interpolate(RandomGenerator.GetRadialOffsetFraction());

									const float CurrentSpawnRadius = Entry.SpawnRadius.Interpolate(RandomGenerator.GetSpawnRadiusFraction());
									const FVector2f RelativePosition = FVector2f(FMath::Cos(AngleInRadians), FMath::Sin(AngleInRadians)) * CurrentSpawnRadius;

									AddInstance(FVector3f(RelativePosition, 0.f), EntryIndex, Index);
									EntryInstancesCount++;
								}
							}

							(*InstancesCountPerEntry)[EntryIndex] += EntryInstancesCount;
							ClusterInstancesCount += EntryInstancesCount;
						}

						(*InstancesCountPerCluster)[Index] = ClusterInstancesCount;
					}

					ensure(GlobalInstanceIndex == InstancesCount);

					FVoxelVectorBuffer InstancesFlatPositionsBuffer;
					InstancesFlatPositionsBuffer.X = FVoxelFloatBuffer::MakeCpu(InstancesPositionsX);
					InstancesFlatPositionsBuffer.Y = FVoxelFloatBuffer::MakeCpu(InstancesPositionsY);
					InstancesFlatPositionsBuffer.Z = FVoxelFloatBuffer::Constant(0.f);

					FVoxelQuery InstancesFlatPositionsQuery = Query;
					InstancesFlatPositionsQuery.Add<FVoxelSparsePositionQueryData>().Initialize(InstancesFlatPositionsBuffer);
					TValue<FVoxelFloatBuffer> InstancesHeight = Get(HeightPin, InstancesFlatPositionsQuery);

					return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, Asset, SeedHash, Step, MaxHeightDifferenceMultiplier, MaskOccupancyMultiplier, WorldUp, ClustersPositionView, InstancesFlatPositionsBuffer, InstancesHeight, InstancesEntryIndexes, InstancesClusterPositionIndexes, InstancesCountPerEntry, InstancesCountPerCluster, TotalValues)
					{
						CheckVoxelBuffersNum(InstancesFlatPositionsBuffer, InstancesHeight);

						FVoxelVectorBuffer InstancesPositionsBuffer;
						InstancesPositionsBuffer.X = InstancesFlatPositionsBuffer.X;
						InstancesPositionsBuffer.Y = InstancesFlatPositionsBuffer.Y;
						InstancesPositionsBuffer.Z = InstancesHeight;

						FVoxelQuery InstancesPositionsQuery = Query;
						InstancesPositionsQuery.Add<FVoxelSparsePositionQueryData>().Initialize(InstancesPositionsBuffer);

						TValue<TBufferView<float>> InstancesMask;
						bool bCheckInstancesMasks = false;
						for (int32 EntryIndex = 0; EntryIndex < Asset->TemplateData->Entries.Num(); EntryIndex++)
						{
							if (Asset->TemplateData->Entries[EntryIndex].bCheckInstancesMasks)
							{
								InstancesMask = GetBufferView(MaskPin, InstancesPositionsQuery);
								bCheckInstancesMasks = true;
								break;
							}
						}

						const TValue<TBufferView<FVector>> InstancesPositionsView = InstancesPositionsBuffer.MakeView();

						return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, Asset, SeedHash, Step, MaxHeightDifferenceMultiplier, MaskOccupancyMultiplier, WorldUp, ClustersPositionView, InstancesFlatPositionsBuffer, InstancesHeight, InstancesEntryIndexes, InstancesClusterPositionIndexes, InstancesCountPerEntry, InstancesCountPerCluster, InstancesPositionsView, InstancesMask, bCheckInstancesMasks, TotalValues)
						{
							const TValue<TBufferView<float>> InstancesGradientsHeight = FVoxelFoliageUtilities::SplitGradientsBuffer(GetNodeRuntime(), Query, HeightPin, InstancesPositionsView, Step);

							return VOXEL_ON_COMPLETE(AsyncThread, BoundsQueryData, Asset, SeedHash, Step, MaxHeightDifferenceMultiplier, MaskOccupancyMultiplier, WorldUp, ClustersPositionView, InstancesPositionsView, InstancesGradientsHeight, InstancesMask, bCheckInstancesMasks, InstancesEntryIndexes, InstancesClusterPositionIndexes, InstancesCountPerEntry, InstancesCountPerCluster, TotalValues)
							{
								const FVoxelVectorBufferView InstancesGradient = FVoxelFoliageUtilities::CollapseGradient(InstancesGradientsHeight, InstancesPositionsView, Step);
								FVoxelFloatBufferView InstancesMaskView = bCheckInstancesMasks ? InstancesMask : FVoxelFloatBuffer::Constant(0).MakeView().Get_CheckCompleted();

								CheckVoxelBuffersNum(InstancesGradient, InstancesPositionsView, InstancesMaskView);

								struct FMeshData
								{
									int32 Index;
									float Strength;
									TSharedPtr<TVoxelArray<FTransform3f>> Transforms;
								};
								TMap<int32, TVoxelArray<FMeshData>> MappedStrengths;

								for (int32 EntryIndex = 0; EntryIndex < Asset->TemplateData->Entries.Num(); EntryIndex++)
								{
									const FVoxelFoliageClusterEntryProxy& Entry = Asset->TemplateData->Entries[EntryIndex];

									float CheckTotalValue = 0.f;
									for (int32 Index = 0; Index < Entry.Instance.Meshes.Num(); Index++)
									{
										if (!Entry.Instance.Meshes[Index].StaticMesh.IsValid())
										{
											continue;
										}

										CheckTotalValue += Entry.Instance.Meshes[Index].Strength / (*TotalValues)[EntryIndex];

										TSharedRef<TVoxelArray<FTransform3f>> Transforms = MakeShared<TVoxelArray<FTransform3f>>();
										Transforms->Reserve((*InstancesCountPerEntry)[EntryIndex]);

										MappedStrengths.FindOrAdd(EntryIndex).Add({
											Index,
											CheckTotalValue,
											Transforms
										});
									}

									ensure(FMath::IsNearlyEqual(CheckTotalValue, 1.f, KINDA_SMALL_NUMBER));

									if (auto EntryMappedStrengthsPtr = MappedStrengths.Find(EntryIndex))
									{
										EntryMappedStrengthsPtr->Last().Strength = 1.01f;
									}
								}

								struct FInstanceData
								{
									int32 FailedInstancesCount;
									int32 ValidInstancesCount;
									TVoxelArray<int32> PositionIndexes;
									FVoxelFoliageRandomGenerator RandomGenerator;

									FInstanceData() = default;
									explicit FInstanceData(int32 InstancesCount, const uint64 SeedHash, const FVector3f& ClusterPosition)
										: FailedInstancesCount(0)
										, ValidInstancesCount(0)
										, PositionIndexes(FVoxelFloatBuffer::Allocate(InstancesCount))
										, RandomGenerator(SeedHash, ClusterPosition)
									{
									}
								};
								TMap<int32, FInstanceData> ClustersData;

								float MaxHeightDifference = Asset->TemplateData->bEnableHeightDifferenceRestriction ? Asset->TemplateData->MaxHeightDifference * MaxHeightDifferenceMultiplier : FLT_MAX;

								for (int32 Index = 0; Index < InstancesPositionsView.Num(); Index++)
								{
									const FVoxelFoliageClusterEntryProxy& Entry = Asset->TemplateData->Entries[(*InstancesEntryIndexes)[Index]];
									const int32 ClusterIndex = (*InstancesClusterPositionIndexes)[Index];

									FInstanceData& ClusterData = ClustersData.FindOrAdd(ClusterIndex, FInstanceData((*InstancesCountPerCluster)[ClusterIndex], SeedHash, ClustersPositionView[ClusterIndex]));

									if (Entry.bCheckInstancesMasks &&
										ClusterData.RandomGenerator.GetMaskRestrictionFraction() > InstancesMaskView[Index])
									{
										ClusterData.FailedInstancesCount++;
										continue;
									}

									const float HeightDifference = FMath::Abs((InstancesPositionsView[Index] * WorldUp - ClustersPositionView[ClusterIndex] * WorldUp).Z);
									if (!Entry.SpawnRestriction.CanSpawn(ClusterData.RandomGenerator, InstancesPositionsView[Index], InstancesGradient[Index], WorldUp) ||
										HeightDifference >= MaxHeightDifference)
									{
										continue;
									}

									ClusterData.PositionIndexes[ClusterData.ValidInstancesCount++] = Index;
								}

								int32 InstancesCount = 0;
								float MaskOccupancy = Asset->TemplateData->MaskOccupancy * MaskOccupancyMultiplier / 100.f;
								for (auto& It : ClustersData)
								{
									if (float(It.Value.ValidInstancesCount) / float(It.Value.FailedInstancesCount + It.Value.ValidInstancesCount) < MaskOccupancy)
									{
										continue;
									}

									for (int32 Index = 0; Index < It.Value.ValidInstancesCount; Index++)
									{
										const int32 PositionIndex = It.Value.PositionIndexes[Index];
										const int32 EntryIndex = (*InstancesEntryIndexes)[PositionIndex];

										const float MeshStrength = It.Value.RandomGenerator.GetMeshSelectionFraction();
										TVoxelArray<FMeshData>& EntryMappedStrength = MappedStrengths[EntryIndex];

										int32 MeshIndex = 0;
										for (int32 CheckMeshIndex = 0; CheckMeshIndex < EntryMappedStrength.Num(); CheckMeshIndex++)
										{
											if (EntryMappedStrength[CheckMeshIndex].Strength >= MeshStrength)
											{
												MeshIndex = EntryMappedStrength[CheckMeshIndex].Index;
												break;
											}
										}

										const FVoxelFoliageClusterEntryProxy& Entry = Asset->TemplateData->Entries[EntryIndex];
										const FMatrix44f Matrix = Entry.Instance.GetTransform(
											EntryMappedStrength[MeshIndex].Index,
											It.Value.RandomGenerator,
											InstancesPositionsView[PositionIndex] - FVector3f(BoundsQueryData->Bounds.Min),
											InstancesGradient[PositionIndex],
											WorldUp,
											Entry.ScaleMultiplier);
										EntryMappedStrength[MeshIndex].Transforms->Add(FTransform3f(Matrix));
										InstancesCount++;
									}
								}

								const TSharedRef<FVoxelFoliageChunkData> Result = MakeShared<FVoxelFoliageChunkData>();
								Result->ChunkPosition = BoundsQueryData->Bounds.Min;
								Result->InstancesCount = InstancesCount;

								for (int32 EntryIndex = 0; EntryIndex < MappedStrengths.Num(); EntryIndex++)
								{
									TVoxelArray<FMeshData>& EntryMappedStrengths = MappedStrengths[EntryIndex];
									const FVoxelFoliageClusterEntryProxy& Entry = Asset->TemplateData->Entries[EntryIndex];
									for (int32 Index = 0; Index < EntryMappedStrengths.Num(); Index++)
									{
										FMeshData& Data = EntryMappedStrengths[Index];
										const FVoxelFoliageMeshProxy& FoliageMesh = Entry.Instance.Meshes[Data.Index];

										if (Data.Transforms->Num() == 0)
										{
											continue;
										}

										Data.Transforms->Shrink();

										FVoxelFoliageSettings InstanceSettings = FoliageMesh.InstanceSettings;
										InstanceSettings.CullDistance = FVoxelInt32Interval(InstanceSettings.CullDistance.Min * Entry.CullDistanceMultiplier, InstanceSettings.CullDistance.Max * Entry.CullDistanceMultiplier);

										const TSharedRef<FVoxelFoliageChunkMeshData> ResultMeshData = MakeShared<FVoxelFoliageChunkMeshData>();
										ResultMeshData->StaticMesh = FoliageMesh.StaticMesh;
										ResultMeshData->FoliageSettings = FoliageMesh.InstanceSettings;
										ResultMeshData->BodyInstance = FoliageMesh.BodyInstance;
										ResultMeshData->Transforms = Data.Transforms;
										Result->Data.Add(ResultMeshData);
									}
								}

								return Result;
							};
						};
					};
				};
			};
		};
	};
}

bool FVoxelNode_GenerateClusterAssetFoliageData::CalculateTotalValues(const FVoxelFoliageClusterTemplateData& Asset, TSharedPtr<TVoxelArray<float>>& Values) const
{
	if (!Asset.TemplateData)
	{
		VOXEL_MESSAGE(Error, "{0}: Foliage Instance Template is null", this);
		return false;
	}

	float OverallTotalValue = 0.f;
	bool bHasValidMeshes = false;

	TVoxelArray<float> ValuesArray = FVoxelFloatBuffer::Allocate(Asset.TemplateData->Entries.Num());

	for (int32 EntryIndex = 0; EntryIndex < Asset.TemplateData->Entries.Num(); EntryIndex++)
	{
		ValuesArray[EntryIndex] = 0.f;

		const FVoxelFoliageClusterEntryProxy& Entry = Asset.TemplateData->Entries[EntryIndex];
		for (const FVoxelFoliageMeshProxy& FoliageMesh : Entry.Instance.Meshes)
		{
			if (!FoliageMesh.StaticMesh.IsValid())
			{
				continue;
			}

			bHasValidMeshes = true;
			ValuesArray[EntryIndex] += FoliageMesh.Strength;
		}

		OverallTotalValue += ValuesArray[EntryIndex];
	}

	if (!bHasValidMeshes)
	{
		VOXEL_MESSAGE(Warning, "{0}: Foliage Cluster Template entries doesn't have valid meshes.", this);
		return false;
	}

	if (FMath::IsNearlyZero(OverallTotalValue))
	{
		VOXEL_MESSAGE(Warning, "{0}: All meshes have strength equal to 0.", this);
		return false;
	}

	Values = MakeShared<TVoxelArray<float>>(ValuesArray);

	return true;
}

int32 FVoxelNode_GenerateClusterAssetFoliageData::CalculateInstancesCount(
	const FVoxelFoliageClusterTemplateData& Asset,
	const uint64 SeedHash,
	const FVoxelVectorBufferView& ClustersPositionView,
	const FVoxelVectorBufferView& ClustersGradient,
	const FVoxelFloatBufferView& ClustersMaskView) const
{
	int32 InstancesCount = 0;
	for (int32 Index = 0; Index < ClustersPositionView.Num(); Index++)
	{
		FVoxelFoliageRandomGenerator RandomGenerator(SeedHash, ClustersPositionView[Index]);

		if (RandomGenerator.GetMaskRestrictionFraction() > ClustersMaskView[Index] ||
			!Asset.TemplateData->SpawnRestriction.CanSpawn(RandomGenerator, ClustersPositionView[Index], ClustersGradient[Index], FVector3f::UpVector))
		{
			continue;
		}
					
		for (int32 EntryIndex = 0; EntryIndex < Asset.TemplateData->Entries.Num(); EntryIndex++)
		{
			const FVoxelFoliageClusterEntryProxy& Entry = Asset.TemplateData->Entries[EntryIndex];
			if (FMath::IsNearlyZero(Entry.SpawnRadius.Max))
			{
				InstancesCount++;
			}
			else
			{
				InstancesCount += Entry.InstancesCount.Interpolate(RandomGenerator.GetInstancesCountFraction());
			}
		}
	}

	return InstancesCount;
}