// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelDensityCanvasData.h"
#include "VoxelDensityCanvasNodes.h"
#include "Nodes/VoxelPositionNodes.h"
#include "VoxelMetaGraphRuntimeUtilities.h"

BEGIN_VOXEL_NAMESPACE(DensityCanvas)

void FData::UseNode(const FVoxelNode_ApplyDensityCanvas* InNode) const
{
	if (WeakOuter.IsValid())
	{
		return;
	}

	VOXEL_FUNCTION_COUNTER();

	FVoxelScopeLock_Write Lock(CriticalSection);
	CanvasNode = InNode;
	WeakOuter = InNode->GetOuter();
}

void FData::AddDependency(const FVoxelBox& Bounds, const TSharedRef<FVoxelDependency>& Dependency) const
{
	FVoxelScopeLock Lock(DependenciesCriticalSection);
	Dependencies.Add({ FVoxelIntBox::FromFloatBox_WithPadding(Bounds / VoxelSize), Dependency });
}

bool FData::HasChunks(const FVoxelBox& Bounds) const
{
	VOXEL_FUNCTION_COUNTER();
	checkVoxelSlow(CriticalSection.IsLocked_Read_Debug());

	bool bHasChunks = false;
	Octree->TraverseBounds(FVoxelIntBox::FromFloatBox_WithPadding(Bounds / VoxelSize), [&](const FOctree::FNode& Node)
	{
		if (!Octree->HasChildren(Node) &&
			Octree->GetNodeData(Node).bHasChunks)
		{
			bHasChunks = true;
		}

		return !bHasChunks;
	});
	return bHasChunks;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FData::ClearData()
{
	VOXEL_FUNCTION_COUNTER();

	{
		FVoxelScopeLock_Write Lock(CriticalSection);
		Octree = MakeShared<FOctree>();
		Chunks.Empty();
	}

	FVoxelScopeLock DependenciesLock(DependenciesCriticalSection);

	TSet<TSharedPtr<FVoxelDependency>> DependenciesToInvalidate;
	for (const FDependencyRef& DependencyRef : Dependencies)
	{
		const TSharedPtr<FVoxelDependency> Dependency = DependencyRef.Dependency.Pin();
		if (!Dependency)
		{
			continue;
		}

		DependenciesToInvalidate.Add(Dependency);
	};
	Dependencies.Empty();

	FVoxelDependency::InvalidateDependencies(DependenciesToInvalidate);
}

void FData::SphereEdit(
	const FVector3f& Center,
	const float Radius,
	const float Falloff,
	const float Strength,
	const bool bSmooth,
	const bool bAdd)
{
	if (IsInGameThread())
	{
		if (bEditQueued)
		{
			return;
		}
		bEditQueued = true;

		Async(EAsyncExecution::ThreadPool, [=, This = AsShared()]
		{
			(void)This;

			SphereEdit(Center, Radius, Falloff, Strength, bSmooth, bAdd);
		});
		return;
	}

	VOXEL_FUNCTION_COUNTER();
	FVoxelScopeLock_Read Lock(CriticalSection);

	if (!ensure(CanvasNode))
	{
		ensure(bEditQueued);
		bEditQueued = false;

		VOXEL_MESSAGE(Error, "Density Canvas is not used by any node");
		return;
	}

	const FIntVector Min = FVoxelUtilities::FloorToInt(Center - Radius);
	const FIntVector Max = FVoxelUtilities::CeilToInt(Center + Radius);
	const FVoxelIntBox Bounds = FVoxelIntBox(Min, Max);

	TArray<FIntVector> ChunkKeys;
	TArray<TVoxelFutureValue<FVoxelFloatBufferView>> Buffers;
	{
		VOXEL_SCOPE_COUNTER("Add chunks");
		Bounds.Extend(1).DivideBigger(ChunkSize).Iterate([&](const FIntVector& ChunkKey)
		{
			if (FindChunk(ChunkKey))
			{
				return;
			}

			FVoxelQuery Query;
			Query.SetDependenciesQueue(MakeShared<FVoxelQuery::FDependenciesQueue>());
			Query.Add<FVoxelLODQueryData>().LOD = 0;
			Query.Add<FVoxelDensePositionQueryData>().Initialize(FVector3f(ChunkKey * ChunkSize) * VoxelSize, VoxelSize, FIntVector(ChunkSize));

			const FVoxelFutureValue Value = CanvasNode->GetNodeRuntime().Get(CanvasNode->InDensityPin, Query);
			if (!ensure(Value.IsValid()))
			{
				return;
			}

			ChunkKeys.Add(ChunkKey);
			Buffers.Add(FVoxelTask::New<FVoxelFloatBufferView>(
				MakeShared<FVoxelTaskStat>(),
				"GetData",
				EVoxelTaskThread::AnyThread,
				{ Value },
				[=]
				{
					return Value.Get_CheckCompleted<FVoxelFloatBuffer>().MakeView();
				}));
		});
	}

	FVoxelTask::New(
		MakeShared<FVoxelTaskStat>(),
		"FVoxelDensityCanvasData",
		EVoxelTaskThread::AsyncThread,
		ReinterpretCastArray<FVoxelFutureValue>(Buffers),
		[=, This = AsShared()]
		{
			VOXEL_USE_NAMESPACE(MetaGraph);

			(void)This;

			TSet<TSharedPtr<FVoxelDependency>> DependenciesToInvalidate;
			{
				VOXEL_SCOPE_COUNTER("Find dependencies");

				FVoxelScopeLock DependenciesLock(DependenciesCriticalSection);
				Dependencies.RemoveAllSwap([&](const FDependencyRef& DependencyRef)
				{
					const TSharedPtr<FVoxelDependency> Dependency = DependencyRef.Dependency.Pin();
					if (!Dependency)
					{
						return true;
					}

					if (!DependencyRef.Bounds.Intersect(Bounds))
					{
						return false;
					}

					DependenciesToInvalidate.Add(Dependency);
					return true;
				});
			}
			ON_SCOPE_EXIT
			{
				FVoxelDependency::InvalidateDependencies(DependenciesToInvalidate);
			};

			FVoxelScopeLock_Write LocalLock(CriticalSection);

			for (int32 ChunkIndex = 0; ChunkIndex < ChunkKeys.Num(); ChunkIndex++)
			{
				const FIntVector ChunkKey = ChunkKeys[ChunkIndex];
				if (!ensure(!Chunks.Contains(ChunkKey)))
				{
					continue;
				}

				TVoxelStaticArray<float, ChunkCount> Densities{ NoInit };

				const FVoxelFloatBufferView Buffer = Buffers[ChunkIndex].Get_CheckCompleted();
				if (Buffer.IsConstant())
				{
					FVoxelUtilities::SetAll(Densities, Buffer.GetConstant());
				}
				else
				{
					if (!ensure(Buffer.Num() == ChunkCount))
					{
						continue;
					}

					FRuntimeUtilities::UnpackData(Buffer.GetRawView(), Densities, FIntVector(ChunkSize));
				}

				const TSharedRef<FChunk> Chunk = MakeShared<FChunk>(NoInit);
				Chunks.Add(ChunkKey, Chunk);

				for (int32 Index = 0; Index < ChunkCount; Index++)
				{
					(*Chunk)[Index] = ToDensity(Densities[Index] / VoxelSize);
				}
			}

			Octree->TraverseBounds(Bounds, [&](const FOctree::FNode& Node)
			{
				FNodeData& NodeData = Octree->GetNodeData(Node);
				NodeData.bHasChunks = true;

				if (!Octree->HasChildren(Node) &&
					Octree->GetHeight(Node) > 0)
				{
					Octree->CreateChildren(Node);
				}
			});

			ParallelFor(Max.Z - Min.Z + 1, [&](int32 InZ)
			{
				const int32 Z = Min.Z + InZ;
				FIntVector LastChunkKey = FIntVector(MAX_int32);
				FChunk* Chunk = nullptr;

				for (int32 Y = Min.Y; Y <= Max.Y; Y++)
				{
					for (int32 X = Min.X; X <= Max.X; X++)
					{
						const FIntVector Position(X, Y, Z);
						const FIntVector ChunkKey = FVoxelUtilities::DivideFloor_FastLog2(Position, ChunkSizeLog2);
						if (ChunkKey != LastChunkKey)
						{
							LastChunkKey = ChunkKey;
							Chunk = FindChunk(ChunkKey);
						}
						if (!ensureVoxelSlow(Chunk))
						{
							continue;
						}

						const FIntVector LocalPosition = Position - ChunkKey * ChunkSize;
						FDensity& Density = (*Chunk)[FVoxelUtilities::Get3DIndex<int32>(ChunkSize, LocalPosition)];
						
						float Distance = FromDensity(Density);
						if (bSmooth)
						{
							const float FalloffStrength = FVoxelUtilities::GetFalloff(
								EVoxelFalloff::Smooth,
								(FVector3f(Position) - Center).Size(),
								Radius,
								Falloff);

							Distance += Strength * FalloffStrength * (bAdd ? -1 : 1);
						}
						else
						{
							const float SphereDistance = (FVector3f(Position) - Center).Size() - Radius;
							Distance = bAdd ? FMath::Min(Distance, SphereDistance) : FMath::Max(Distance, -SphereDistance);
						}
						Density = ToDensity(Distance);
					}
				}
			});

			ensure(bEditQueued);
			bEditQueued = false;
		});
}

void FData::Serialize(FArchive& Ar)
{
	VOXEL_FUNCTION_COUNTER();

	if (Ar.IsLoading())
	{
		ClearData();
	}

	using FVersion = DECLARE_VOXEL_VERSION
	(
		FirstVersion
	);

	int32 Version = FVersion::LatestVersion;
	Ar << Version;
	check(Version == FVersion::FirstVersion);

	FVoxelScopeLock_Write Lock(CriticalSection);

	if (Ar.IsSaving())
	{
		int32 DensitySize = sizeof(FDensity);
		Ar << DensitySize;

		TArray<FIntVector> Keys;
		Chunks.GenerateKeyArray(Keys);
		Ar << Keys;

		for (const FIntVector& Key : Keys)
		{
			Ar << *Chunks[Key];
		}
	}
	else
	{
		check(Ar.IsLoading());
		ensure(Chunks.Num() == 0);
		
		int32 DensitySize = 0;
		Ar << DensitySize;

		if (!ensure(DensitySize == sizeof(FDensity)))
		{
			return;
		}

		TArray<FIntVector> Keys;
		Ar << Keys;

		for (const FIntVector& Key : Keys)
		{
			const TSharedRef<FChunk> Chunk = MakeShared<FChunk>(ForceInit);
			Ar << *Chunk;
			Chunks.Add(Key, Chunk);
			
			Octree->TraverseBounds(FVoxelIntBox(Key).Scale(ChunkSize), [&](const FOctree::FNode& Node)
			{
				FNodeData& NodeData = Octree->GetNodeData(Node);
				NodeData.bHasChunks = true;

				if (!Octree->HasChildren(Node) &&
					Octree->GetHeight(Node) > 0)
				{
					Octree->CreateChildren(Node);
				}
			});
		}
	}
}

END_VOXEL_NAMESPACE(DensityCanvas)