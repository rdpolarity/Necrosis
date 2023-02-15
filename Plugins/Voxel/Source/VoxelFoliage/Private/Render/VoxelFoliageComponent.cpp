// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Render/VoxelFoliageComponent.h"
#include "VoxelFoliage.h"

DEFINE_UNIQUE_VOXEL_ID(FVoxelFoliageComponentTreeId);

UVoxelFoliageComponent::UVoxelFoliageComponent()
{
	bDisableCollision = true;
}

void UVoxelFoliageComponent::StartTreeBuild()
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(FoliageData) ||
		!GetStaticMesh())
	{
		return;
	}
	FoliageData->UpdateStats();

	if (FoliageData->BuiltData)
	{
		FinishTreeBuild(MoveTemp(*FoliageData->BuiltData));
		FoliageData->BuiltData.Reset();
		return;
	}

	QueuedTreeId = FVoxelFoliageComponentTreeId::New();

	AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [
		WeakThis = MakeWeakObjectPtr(this),
		TreeId = QueuedTreeId,
		MeshBox = GetStaticMesh()->GetBounds().GetBox(),
		InDesiredInstancesPerLeaf = DesiredInstancesPerLeaf(),
		InFoliageData = FoliageData]
	{
		const TSharedRef<FVoxelFoliageBuiltData> BuiltData = MakeShared<FVoxelFoliageBuiltData>();
		AsyncTreeBuild(*BuiltData, MeshBox, InDesiredInstancesPerLeaf, *InFoliageData);

		AsyncTask(ENamedThreads::GameThread, [=]
		{
			UVoxelFoliageComponent* This = WeakThis.Get();
			if (!This ||
				This->QueuedTreeId != TreeId)
			{
				return;
			}

			This->QueuedTreeId = {};
			This->FinishTreeBuild(MoveTemp(*BuiltData));
		});
	});
}

void UVoxelFoliageComponent::ClearInstances()
{
	VOXEL_FUNCTION_COUNTER();

	AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [FoliageDataPtr = MakeUniqueCopy(FoliageData)]
	{
		VOXEL_SCOPE_COUNTER("Delete FoliageData");
		FoliageDataPtr->Reset();
	});

	FoliageData.Reset();
	QueuedTreeId = {};

	ReleasePerInstanceRenderData();

	Super::ClearInstances();
}

void UVoxelFoliageComponent::DestroyComponent(bool bPromoteChildren)
{
	Super::DestroyComponent(bPromoteChildren);

	FoliageData.Reset();
}

int32 UVoxelFoliageComponent::GetDesiredInstancesPerLeaf(const UStaticMesh& StaticMesh)
{
	ensure(IsInGameThread());

	if (!StaticMesh.HasValidRenderData(true, 0))
	{
		return 16;
	}

	static IConsoleVariable* CVarMinVertsToSplitNode = IConsoleManager::Get().FindConsoleVariable(TEXT("foliage.MinVertsToSplitNode"));
	check(CVarMinVertsToSplitNode);

	return FMath::Clamp(CVarMinVertsToSplitNode->GetInt() / StaticMesh.GetNumVertices(0), 1, 1024);
}

void UVoxelFoliageComponent::FinishTreeBuild(FVoxelFoliageBuiltData&& BuiltData)
{
	VOXEL_FUNCTION_COUNTER();

	if (PerInstanceRenderData.IsValid())
	{
		VOXEL_SCOPE_COUNTER("ReleasePerInstanceRenderData");
		ReleasePerInstanceRenderData();
	}

	const int32 NumInstances = BuiltData.InstanceBuffer->GetNumInstances();

	{
		VOXEL_SCOPE_COUNTER("InitPerInstanceRenderData");
		constexpr bool bRequireCPUAccess = false;
		InitPerInstanceRenderData(true, BuiltData.InstanceBuffer.Get(), bRequireCPUAccess);
	}

	{
		VOXEL_SCOPE_COUNTER("AcceptPrebuiltTree");
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		AcceptPrebuiltTree(UE_501_ONLY(BuiltData.InstanceDatas,) BuiltData.ClusterTree, BuiltData.OcclusionLayerNum, NumInstances);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

	if (FoliageData->CustomDatas.Num() == 0)
	{
		// AcceptPrebuiltTree does not move data for non nanite proxies
		if (!ShouldCreateNaniteProxy())
		{
			PerInstanceSMData = {};
		}

		NumCustomDataFloats = 0;
		PerInstanceSMCustomData = {};
		InstanceReorderTable = {};
	}
	else
	{
		// AcceptPrebuiltTree does not move data for non nanite proxies
		if (!ShouldCreateNaniteProxy())
		{
			PerInstanceSMData = MoveTemp(BuiltData.InstanceDatas);
		}

		NumCustomDataFloats = FoliageData->CustomDatas.Num();
		PerInstanceSMCustomData = MoveTemp(BuiltData.CustomDatas);
		InstanceReorderTable = MoveTemp(BuiltData.InstanceReorderTable);
	}
}

void UVoxelFoliageComponent::AsyncTreeBuild(
	FVoxelFoliageBuiltData& OutBuiltData,
	const FBox& MeshBox,
	int32 InDesiredInstancesPerLeaf,
	const FVoxelFoliageData& InFoliageData)
{
	VOXEL_FUNCTION_COUNTER();

	const int32 NumInstances = InFoliageData.Transforms->Num();
	check(NumInstances > 0);
	
	OutBuiltData.InstanceBuffer = MakeUnique<FStaticMeshInstanceData>(false);
	{
		VOXEL_SCOPE_COUNTER("AllocateInstances");
		OutBuiltData.InstanceBuffer->AllocateInstances(
			NumInstances,
			InFoliageData.CustomDatas.Num(),
			EResizeBufferFlags::None,
			true);
	}

	TVoxelArray<FMatrix> Matrices;
	FVoxelUtilities::SetNumFast(Matrices, NumInstances);
	{
		VOXEL_SCOPE_COUNTER("SetInstances");
		for (int32 Index = 0; Index < NumInstances; Index++)
		{
			FTransform3f Transform = (*InFoliageData.Transforms)[Index];
			Transform.NormalizeRotation();
			const FMatrix44f Matrix = Transform.ToMatrixWithScale();

			const uint64 Seed = FVoxelUtilities::MurmurHash(Transform.GetTranslation());
			const FRandomStream RandomStream(Seed);

			Matrices[Index] = FMatrix(Matrix);
			OutBuiltData.InstanceBuffer->SetInstance(Index, Matrix, RandomStream.GetFraction());
		}
	}

	OutBuiltData.InstanceDatas = ReinterpretCastVoxelArray<FInstancedStaticMeshInstanceData>(Matrices);

	FVoxelUtilities::SetNumFast(OutBuiltData.CustomDatas, InFoliageData.CustomDatas.Num() * NumInstances);

	{
		VOXEL_SCOPE_COUNTER("Set Custom Data");
		for (int32 CustomDataIndex = 0; CustomDataIndex < InFoliageData.CustomDatas.Num(); CustomDataIndex++)
		{
			const TVoxelArray<float>& CustomData = InFoliageData.CustomDatas[CustomDataIndex];
			if (!ensure(CustomData.Num() == NumInstances))
			{
				continue;
			}

			for (int32 InstanceIndex = 0; InstanceIndex < NumInstances; InstanceIndex++)
			{
				OutBuiltData.CustomDatas[InstanceIndex * InFoliageData.CustomDatas.Num() + CustomDataIndex] = CustomData[InstanceIndex];
				OutBuiltData.InstanceBuffer->SetInstanceCustomData(InstanceIndex, CustomDataIndex, CustomData[InstanceIndex]);
			}
		}
	}

	TVoxelArray<int32> SortedInstances;
	TVoxelArray<int32> InstanceReorderTable;
	{
		VOXEL_SCOPE_COUNTER("BuildTreeAnyThread");
		TArray<float> CustomDataFloats;
		// TODO
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		BuildTreeAnyThread(
			Matrices,
			// Done manually
			CustomDataFloats,
			0,
			MeshBox,
			OutBuiltData.ClusterTree,
			SortedInstances,
			InstanceReorderTable,
			OutBuiltData.OcclusionLayerNum,
			InDesiredInstancesPerLeaf,
			false
		);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

	OutBuiltData.InstanceReorderTable = InstanceReorderTable;

	// In-place sort the instances
	{
		VOXEL_SCOPE_COUNTER("Sort Instances");
		for (int32 FirstUnfixedIndex = 0; FirstUnfixedIndex < NumInstances; FirstUnfixedIndex++)
		{
			const int32 LoadFrom = SortedInstances[FirstUnfixedIndex];
			if (LoadFrom == FirstUnfixedIndex)
			{
				continue;
			}

			check(LoadFrom > FirstUnfixedIndex);
			OutBuiltData.InstanceBuffer->SwapInstance(FirstUnfixedIndex, LoadFrom);
			const int32 SwapGoesTo = InstanceReorderTable[FirstUnfixedIndex];
			checkVoxelSlow(SwapGoesTo > FirstUnfixedIndex);
			checkVoxelSlow(SortedInstances[SwapGoesTo] == FirstUnfixedIndex);
			SortedInstances[SwapGoesTo] = LoadFrom;
			InstanceReorderTable[LoadFrom] = SwapGoesTo;
			InstanceReorderTable[FirstUnfixedIndex] = FirstUnfixedIndex;
			SortedInstances[FirstUnfixedIndex] = FirstUnfixedIndex;
		}
	}
}