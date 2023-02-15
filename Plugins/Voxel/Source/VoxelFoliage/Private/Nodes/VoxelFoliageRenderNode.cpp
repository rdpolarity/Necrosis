// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelFoliageRenderNode.h"
#include "Nodes/VoxelPositionNodes.h"
#include "Render/VoxelFoliageComponent.h"

void FVoxelChunkExecObject_CreateFoliageMeshComponent::Create(FVoxelRuntime& Runtime) const
{
	if (!ensure(TemplatesData.Num() > 0) ||
		!ensure(FoliageIds.Num() == 0))
	{
		return;
	}

	FVoxelFoliageRenderer& FoliageRenderer = Runtime.GetSubsystem<FVoxelFoliageRenderer>();

	for (const FTemplateData& Data : TemplatesData)
	{
		if (Data.FoliageData->Transforms->Num() == 0)
		{
			continue;
		}

		FoliageIds.Add(FoliageRenderer.CreateMesh(
			Position,
			Data.StaticMesh.Get(),
			Data.FoliageSettings,
			Data.FoliageData.ToSharedRef()));
	}
}

void FVoxelChunkExecObject_CreateFoliageMeshComponent::Destroy(FVoxelRuntime& Runtime) const
{
	FVoxelFoliageRenderer& FoliageRenderer = Runtime.GetSubsystem<FVoxelFoliageRenderer>();

	for (FVoxelFoliageRendererId& Id : FoliageIds)
	{
		ensure(Id.IsValid());
		FoliageRenderer.DestroyMesh(Id);
	}

	FoliageIds = {};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelFutureValue<FVoxelChunkExecObject> FVoxelChunkExecNode_CreateFoliageMeshComponent::Execute(const FVoxelQuery& Query) const
{
	const TValue<FVoxelFoliageChunkData> ChunkData = GetNodeRuntime().Get(ChunkDataPin, Query);

	return VOXEL_ON_COMPLETE(AsyncThread, ChunkData)
	{
		if (ChunkData->Data.Num() == 0 ||
			ChunkData->InstancesCount == 0)
		{
			return {};
		}

		
		if (GetArrayPins(CustomDataPins).Num() == 0)
		{
			const TSharedRef<FVoxelChunkExecObject_CreateFoliageMeshComponent> Object = MakeShared<FVoxelChunkExecObject_CreateFoliageMeshComponent>();
			Object->Position = ChunkData->ChunkPosition;

			for (const TSharedPtr<FVoxelFoliageChunkMeshData>& MeshData : ChunkData->Data)
			{
				if (MeshData->Transforms->Num() == 0)
				{
					continue;
				}

				const TSharedPtr<FVoxelFoliageData> FoliageData = MakeShared<FVoxelFoliageData>();
				FoliageData->Transforms = MeshData->Transforms;

				FoliageData->UpdateStats();

				Object->TemplatesData.Add({
					MeshData->StaticMesh,
					MeshData->FoliageSettings,
					FoliageData
				});
			}

			return RenderFoliage(Query, Object);
		}

		TVoxelArray<float> PositionsX = FVoxelFloatBuffer::Allocate(ChunkData->InstancesCount);
		TVoxelArray<float> PositionsY = FVoxelFloatBuffer::Allocate(ChunkData->InstancesCount);
		TVoxelArray<float> PositionsZ = FVoxelFloatBuffer::Allocate(ChunkData->InstancesCount);
		TSharedRef<TVoxelArray<int32>> MappedMeshesIndexes = MakeSharedCopy(FVoxelInt32Buffer::Allocate(ChunkData->InstancesCount));
		TSharedRef<TVoxelArray<int32>> MappedInstancesIndexes = MakeSharedCopy(FVoxelInt32Buffer::Allocate(ChunkData->InstancesCount));

		int32 InstanceIndex = 0;
		for (int32 MeshIndex = 0; MeshIndex < ChunkData->Data.Num(); MeshIndex++)
		{
			const TSharedPtr<FVoxelFoliageChunkMeshData>& MeshData = ChunkData->Data[MeshIndex];
			for (int32 MeshInstanceIndex = 0; MeshInstanceIndex < MeshData->Transforms->Num(); MeshInstanceIndex++)
			{
				const FVector3f& InstancePosition = (*MeshData->Transforms)[MeshInstanceIndex].GetLocation() + FVector3f(ChunkData->ChunkPosition);

				PositionsX[InstanceIndex] = InstancePosition.X;
				PositionsY[InstanceIndex] = InstancePosition.Y;
				PositionsZ[InstanceIndex] = InstancePosition.Z;
	
				(*MappedMeshesIndexes)[InstanceIndex] = MeshIndex;
				(*MappedInstancesIndexes)[InstanceIndex] = MeshInstanceIndex;

				InstanceIndex++;
			}
		}

		const FVoxelVectorBuffer PositionsBuffer = FVoxelVectorBuffer::MakeCpu(PositionsX, PositionsY, PositionsZ);
		FVoxelQuery PositionQuery = Query;
		PositionQuery.Add<FVoxelSparsePositionQueryData>().Initialize(PositionsBuffer);

		TArray<TValue<TBufferView<float>>> CustomDataFutures = GetNodeRuntime().GetBufferView(CustomDataPins, PositionQuery);
		return VOXEL_ON_COMPLETE(AsyncThread, ChunkData, CustomDataFutures, MappedMeshesIndexes, MappedInstancesIndexes)
		{
			TVoxelArray<TSharedPtr<FVoxelFoliageData>> FoliageDatas;
			for (const TSharedPtr<FVoxelFoliageChunkMeshData>& MeshData : ChunkData->Data)
			{
				if (MeshData->Transforms->Num() == 0)
				{
					continue;
				}

				const TSharedPtr<FVoxelFoliageData> FoliageData = MakeShared<FVoxelFoliageData>();
				FoliageData->Transforms = MeshData->Transforms;

				for (int32 CustomDataIndex = 0; CustomDataIndex < CustomDataFutures.Num(); CustomDataIndex++)
				{
					FoliageData->CustomDatas.Add(FVoxelFloatBuffer::Allocate(MeshData->Transforms->Num()));
				}

				FoliageDatas.Add(FoliageData);
			}

			for (int32 CustomDataIndex = 0; CustomDataIndex < CustomDataFutures.Num(); CustomDataIndex++)
			{
				const FVoxelFloatBufferView& CustomData = CustomDataFutures[CustomDataIndex];
				if (CustomData.Num() != 1 && CustomData.Num() != ChunkData->InstancesCount)
				{
					FVoxelNodeHelpers::RaiseBufferError(*this);
					return {};
				}

				for (int32 Index = 0; Index < ChunkData->InstancesCount; Index++)
				{
					FoliageDatas[(*MappedMeshesIndexes)[Index]]->CustomDatas[CustomDataIndex][(*MappedInstancesIndexes)[Index]] = CustomData[Index];
				}
			}

			const TSharedRef<FVoxelChunkExecObject_CreateFoliageMeshComponent> Object = MakeShared<FVoxelChunkExecObject_CreateFoliageMeshComponent>();
			Object->Position = ChunkData->ChunkPosition;

			for (int32 Index = 0; Index < ChunkData->Data.Num(); Index++)
			{
				FoliageDatas[Index]->UpdateStats();

				const TSharedPtr<FVoxelFoliageChunkMeshData> MeshData = ChunkData->Data[Index];

				Object->TemplatesData.Add({
					MeshData->StaticMesh,
					MeshData->FoliageSettings,
					FoliageDatas[Index]
				});
			}

			return RenderFoliage(Query, Object);
		};
	};
}

TVoxelFutureValue<FVoxelChunkExecObject> FVoxelChunkExecNode_CreateFoliageMeshComponent::RenderFoliage(const FVoxelQuery& Query, const TSharedRef<FVoxelChunkExecObject_CreateFoliageMeshComponent>& Object) const
{
	if (Object->TemplatesData.Num() == 0)
	{
		return {};
	}

	return VOXEL_ON_COMPLETE(GameThread, Object)
	{
		struct FFoliageMeshData
		{
			FBox MeshBox;
			int32 DesiredInstancesPerLeaf;
		};
		using FMap = TMap<TSharedPtr<const FVoxelFoliageData>, FFoliageMeshData>;

		const TSharedRef<FMap> MeshDatas = MakeShared<FMap>();
		for (const FVoxelChunkExecObject_CreateFoliageMeshComponent::FTemplateData& TemplateData : Object->TemplatesData)
		{
			const UStaticMesh* Mesh = TemplateData.StaticMesh.Get();
			if (!ensure(Mesh))
			{
				return {};
			}

			MeshDatas->FindOrAdd(TemplateData.FoliageData,
			{
				Mesh->GetBoundingBox(),
				UVoxelFoliageComponent::GetDesiredInstancesPerLeaf(*Mesh)
			});
		}

		return VOXEL_ON_COMPLETE(AsyncThread, Object, MeshDatas)
		{
			for (const FVoxelChunkExecObject_CreateFoliageMeshComponent::FTemplateData& TemplateData : Object->TemplatesData)
			{
				const TSharedRef<FVoxelFoliageBuiltData> BuiltData = MakeShared<FVoxelFoliageBuiltData>();

				UVoxelFoliageComponent::AsyncTreeBuild(
					*BuiltData, 
					MeshDatas->FindChecked(TemplateData.FoliageData).MeshBox, 
					MeshDatas->FindChecked(TemplateData.FoliageData).DesiredInstancesPerLeaf,
					*TemplateData.FoliageData);

				TemplateData.FoliageData->BuiltData = BuiltData;
			}

			return Object;
		};
	};
}