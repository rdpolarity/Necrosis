// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelFoliageEditorUtilities.h"

#include "PreviewScene.h"
#include "VoxelFoliageClusterTemplate.h"
#include "VoxelFoliageRandomGenerator.h"
#include "VoxelFoliageInstanceTemplate.h"
#include "Toolkits/VoxelFoliageClusterTemplateEditorToolkit.h"

void FVoxelFoliageEditorUtilities::RenderInstanceTemplate(
	const UVoxelFoliageInstanceTemplate& InstanceTemplate,
	FPreviewScene& PreviewScene,
	TArray<TWeakObjectPtr<UStaticMeshComponent>>& OutStaticMeshComponents)
{
	FVoxelFoliageRandomGenerator RandomGenerator;

	float OverallOffset  = 0.f;
	for (int32 Index = 0; Index < InstanceTemplate.Meshes.Num(); Index++)
	{
		const UVoxelFoliageMesh_New* FoliageMesh = InstanceTemplate.Meshes[Index];
		if (!FoliageMesh ||
			!FoliageMesh->StaticMesh)
		{
			continue;
		}

		const FVoxelFoliageOffsetSettings& OffsetSettings = FVoxelFoliageOffsetSettings(InstanceTemplate.OffsetSettings, FoliageMesh->OffsetSettings);
		const FVoxelFoliageScaleSettings& ScaleSettings = FoliageMesh->bOverrideScaleSettings ? FoliageMesh->ScaleSettings : InstanceTemplate.ScaleSettings;
		OverallOffset += FoliageMesh->StaticMesh->GetBoundingBox().GetSize().X * ScaleSettings.GetScale(RandomGenerator).X + OffsetSettings.GlobalPositionOffset.X + OffsetSettings.LocalRotationOffset.RotateVector(OffsetSettings.LocalPositionOffset).X;
	}

	RandomGenerator.Reset();

	FVector Offset(OverallOffset / -2.f, 0.f, 0.f);

	for (int32 Index = 0; Index < InstanceTemplate.Meshes.Num(); Index++)
	{
		if (!OutStaticMeshComponents.IsValidIndex(Index))
		{
			UStaticMeshComponent* StaticMeshComponent = NewObject<UStaticMeshComponent>();
			OutStaticMeshComponents.Add(StaticMeshComponent);
			PreviewScene.AddComponent(StaticMeshComponent, FTransform::Identity, true);
		}
		UStaticMeshComponent* StaticMeshComponent = OutStaticMeshComponents[Index].Get();

		const UVoxelFoliageMesh_New* FoliageMesh = InstanceTemplate.Meshes[Index];

		if (!FoliageMesh)
		{
			continue;
		}

		StaticMeshComponent->SetStaticMesh(FoliageMesh->StaticMesh);

		for (int32 MaterialIndex = 0; MaterialIndex < FoliageMesh->InstanceSettings.MaterialOverrides.Num(); MaterialIndex++)
		{
			StaticMeshComponent->SetMaterial(MaterialIndex, FoliageMesh->InstanceSettings.MaterialOverrides[MaterialIndex]);
		}

		if (!FoliageMesh->StaticMesh)
		{
			continue;
		}

		const FVoxelFoliageOffsetSettings& OffsetSettings = FVoxelFoliageOffsetSettings(InstanceTemplate.OffsetSettings, FoliageMesh->OffsetSettings);
		const FVoxelFoliageScaleSettings& ScaleSettings = FoliageMesh->bOverrideScaleSettings ? FoliageMesh->ScaleSettings : InstanceTemplate.ScaleSettings;
		const FVector3f Scale = ScaleSettings.GetScale(RandomGenerator);

		const FVector LocalOffset = OffsetSettings.LocalRotationOffset.RotateVector(OffsetSettings.LocalPositionOffset);
		Offset.Y = OffsetSettings.GlobalPositionOffset.Z;
		Offset.Z = OffsetSettings.GlobalPositionOffset.Z;
		Offset.X += FoliageMesh->StaticMesh->GetBoundingBox().GetSize().X * Scale.X / 2.f + OffsetSettings.GlobalPositionOffset.X / 2.f + LocalOffset.X / 2.f;

		StaticMeshComponent->SetRelativeRotation(OffsetSettings.LocalRotationOffset);
		StaticMeshComponent->SetRelativeLocation(FVector(Offset.X, Offset.Y + LocalOffset.Y, Offset.Z + LocalOffset.Z));

		StaticMeshComponent->SetRelativeScale3D(FVector(Scale));

		Offset.X += FoliageMesh->StaticMesh->GetBoundingBox().GetSize().X * Scale.X / 2.f + OffsetSettings.GlobalPositionOffset.X / 2.f + LocalOffset.X / 2.f;
	}

	for (int32 Index = InstanceTemplate.Meshes.Num(); Index < OutStaticMeshComponents.Num(); Index++)
	{
		OutStaticMeshComponents[Index]->SetStaticMesh(nullptr);
	}
}

void FVoxelFoliageEditorUtilities::RenderClusterTemplate(const UVoxelFoliageClusterTemplate& ClusterTemplate, FPreviewScene& PreviewScene, FClusterRenderData& Data, TArray<TWeakObjectPtr<UInstancedStaticMeshComponent>>& OutMeshComponents)
{
	const FVoxelFoliageRandomGenerator RandomGenerator(Data.Seed, FVector3f::ZeroVector);

	TMap<UVoxelFoliageMesh_New*, TArray<FTransform>> MeshesToSpawn;

	constexpr int32 MaxInstances = 10000;

	for (const UVoxelFoliageClusterEntry* Entry : ClusterTemplate.Entries)
	{
		if (!Entry->Instance)
		{
			continue;
		}
		const TSharedRef<FVoxelFoliageInstanceTemplateProxy> InstanceProxy = MakeShared<FVoxelFoliageInstanceTemplateProxy>(*Entry->Instance);

		float TotalValue = 0.f;
		bool bHasValidMeshes = false;
		for (const FVoxelFoliageMeshProxy& FoliageMesh : InstanceProxy->Meshes)
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
			continue;
		}

		if (FMath::IsNearlyZero(TotalValue))
		{
			continue;
		}

		struct FMeshData
		{
			int32 Index;
			float Strength;
		};

		TVoxelArray<FMeshData> MappedStrengths;
		float CheckTotalValue = 0.f;
		for (int32 Index = 0; Index < InstanceProxy->Meshes.Num(); Index++)
		{
			if (!InstanceProxy->Meshes[Index].StaticMesh.IsValid())
			{
				continue;
			}

			Data.MeshComponentsCount++;
			CheckTotalValue += InstanceProxy->Meshes[Index].Strength / TotalValue;

			MappedStrengths.Add({
				Index,
				CheckTotalValue
			});
		}

		const auto AddTransform = [&](const FVector3f& RelativeLocation)
		{
			const float MeshStrength = RandomGenerator.GetMeshSelectionFraction();
			int32 MeshIndex = 0;
			for (int32 CheckMeshIndex = 0; CheckMeshIndex < MappedStrengths.Num(); CheckMeshIndex++)
			{
				if (MappedStrengths[CheckMeshIndex].Strength >= MeshStrength)
				{
					MeshIndex = MappedStrengths[CheckMeshIndex].Index;
					break;
				}
			}
			const FMatrix44f Transform = InstanceProxy->GetTransform(MeshIndex, RandomGenerator, RelativeLocation, FVector3f::UpVector, FVector3f::UpVector, Entry->ScaleMultiplier);
			MeshesToSpawn.FindOrAdd(Entry->Instance->Meshes[MeshIndex]).Add(FTransform(FMatrix(Transform)));
		};

		Data.FloorScale = FMath::Max3(Data.FloorScale, Entry->SpawnRadius.Max / 500.f, 0.1f);

		if (FMath::IsNearlyZero(Entry->SpawnRadius.Max))
		{
			AddTransform(FVector3f::ZeroVector);
			
			Data.TotalNumInstances++;
			Data.MinOverallInstances++;
			Data.MaxOverallInstances++;
		}
		else
		{
			int32 NumberOfInstances;
			switch (Data.PreviewInstancesCount)
			{
			default: check(false);
			case EPreviewInstancesCount::Min: NumberOfInstances = Entry->InstancesCount.Min; break;
			case EPreviewInstancesCount::Random: NumberOfInstances = Entry->InstancesCount.Interpolate(RandomGenerator.GetInstancesCountFraction()); break;
			case EPreviewInstancesCount::Max: NumberOfInstances = Entry->InstancesCount.Max; break;
			}

			Data.MinOverallInstances += Entry->InstancesCount.Min;
			Data.MaxOverallInstances += Entry->InstancesCount.Max;

			float RadialOffsetRadians = FMath::DegreesToRadians(Entry->RadialOffset);
			FVoxelFloatInterval RadialOffset = FVoxelFloatInterval(RadialOffsetRadians * -0.5f, RadialOffsetRadians * 0.5f);

			for (int32 InstanceIndex = 0; InstanceIndex < NumberOfInstances; InstanceIndex++)
			{
				const float AngleInRadians = float(InstanceIndex) / float(NumberOfInstances) * 2.f * PI + RadialOffset.Interpolate(RandomGenerator.GetRadialOffsetFraction());

				const float CurrentSpawnRadius = Entry->SpawnRadius.Interpolate(RandomGenerator.GetSpawnRadiusFraction());
				const FVector2f RelativePosition = FVector2f(FMath::Cos(AngleInRadians), FMath::Sin(AngleInRadians)) * CurrentSpawnRadius;

				AddTransform(FVector3f(RelativePosition, 0.f));
				Data.TotalNumInstances++;

				if (Data.TotalNumInstances > MaxInstances)
				{
					break;
				}
			}
		}
	}

	if (Data.TotalNumInstances > MaxInstances)
	{
		VOXEL_MESSAGE(Error, "More than {0} foliage instances previewed! Check your settings", MaxInstances);
		for (const TWeakObjectPtr<UInstancedStaticMeshComponent> MeshComponent : OutMeshComponents)
		{
			MeshComponent->ClearInstances();
		}
		return;
	}

	TArray<TWeakObjectPtr<UInstancedStaticMeshComponent>> MeshComponentsQueue = OutMeshComponents;

	for (auto& It : MeshesToSpawn)
	{
		UInstancedStaticMeshComponent* MeshComponent;
		if (MeshComponentsQueue.Num() > 0)
		{
			MeshComponent = MeshComponentsQueue.Pop().Get();
		}
		else
		{
			MeshComponent = NewObject<UInstancedStaticMeshComponent>();
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			OutMeshComponents.Add(MeshComponent);
			PreviewScene.AddComponent(MeshComponent, FTransform::Identity);
		}

		MeshComponent->SetStaticMesh(It.Key->StaticMesh);
		for (int32 MaterialIndex = 0; MaterialIndex < It.Key->InstanceSettings.MaterialOverrides.Num(); MaterialIndex++)
		{
			MeshComponent->SetMaterial(MaterialIndex, It.Key->InstanceSettings.MaterialOverrides[MaterialIndex]);
		}

		MeshComponent->ClearInstances();
		MeshComponent->PreAllocateInstancesMemory(It.Value.Num());
		for (FTransform Transform : It.Value)
		{
			Transform.SetLocation(Transform.GetLocation());
			MeshComponent->AddInstance(Transform);
		}
	}
	
	for (const TWeakObjectPtr<UInstancedStaticMeshComponent> MeshComponent : MeshComponentsQueue)
	{
		MeshComponent->ClearInstances();
	}
}