// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelLandmassMeshBrush.h"
#include "VoxelLandmassBrushData.h"
#include "VoxelLandmassSettings.h"
#include "MeshDescription.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "VoxelRuntime/VoxelRuntimeUtilities.h"

#if WITH_EDITOR
#include "AssetToolsModule.h"
#include "EditorReimportHandler.h"
#include "DerivedDataCacheInterface.h"
#endif

DEFINE_VOXEL_FACTORY(UVoxelLandmassMeshBrush);

float UVoxelLandmassMeshBrush::GetVoxelSize() const
{
	return GetDefault<UVoxelLandmassSettings>()->BaseVoxelSize * VoxelSizeMultiplier;
}

TSharedPtr<FVoxelLandmassBrushData> UVoxelLandmassMeshBrush::GetBrushData()
{
	if (BrushData)
	{
		if (BrushData->VoxelSize != GetVoxelSize() ||
			BrushData->MaxSmoothness != MaxSmoothness ||
			BrushData->VoxelizerSettings != VoxelizerSettings)
		{
			BrushData.Reset();
		}
	}

	if (!BrushData)
	{
		TryCreateBrushData();
	}

	return BrushData;
}

#if WITH_EDITOR
VOXEL_RUN_ON_STARTUP_GAME(RegisterVoxelLandmassMeshBrushOnReimport)
{
	FReimportManager::Instance()->OnPostReimport().AddLambda([](UObject* Asset, bool bSuccess)
	{
		UVoxelLandmassMeshBrush::OnReimport();
	});
}

void UVoxelLandmassMeshBrush::OnReimport()
{
	for (UVoxelLandmassMeshBrush* Brush : TObjectRange<UVoxelLandmassMeshBrush>())
	{
		Brush->BrushData.Reset();
	}

	FVoxelRuntimeUtilities::ForeachRuntime(nullptr, [](const FVoxelRuntime& Runtime)
	{
		FVoxelRuntimeUtilities::RecreateRuntime(Runtime);
	});
}
#endif

void UVoxelLandmassMeshBrush::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	
	bool bCooked = Ar.IsCooking();
	Ar << bCooked;

	if (bCooked && !IsTemplate() && !Ar.IsCountingMemory())
	{	
		if (Ar.IsLoading())
		{
			BrushData = MakeShared<FVoxelLandmassBrushData>();
			BrushData->Serialize(Ar);
		}
#if WITH_EDITOR
		else if (Ar.IsSaving())
		{
			if (!BrushData)
			{
				TryCreateBrushData();
			}
			if (!BrushData)
			{
				BrushData = MakeShared<FVoxelLandmassBrushData>();
			}
			BrushData->Serialize(Ar);
	}
#endif
	}
}

#if WITH_EDITOR
void UVoxelLandmassMeshBrush::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	VOXEL_FUNCTION_COUNTER();

	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive)
	{
		return;
	}

	// In case mesh changed
	BrushData.Reset();
	
	FVoxelRuntimeUtilities::ForeachRuntime(nullptr, [](const FVoxelRuntime& Runtime)
	{
		FVoxelRuntimeUtilities::RecreateRuntime(Runtime);
	});
}
#endif

void UVoxelLandmassMeshBrush::TryCreateBrushData()
{
	VOXEL_FUNCTION_COUNTER();
	ensure(!BrushData);

	UStaticMesh* LoadedMesh = Mesh.LoadSynchronous();
	if (!LoadedMesh)
	{
		return;
	}

	float VoxelSize = GetDefault<UVoxelLandmassSettings>()->BaseVoxelSize * VoxelSizeMultiplier;

#if WITH_EDITOR
	FString KeySuffix;

	const FStaticMeshSourceModel& SourceModel = LoadedMesh->GetSourceModel(0);
	if (ensure(SourceModel.GetMeshDescriptionBulkData()))
	{
		KeySuffix += "MD";
		KeySuffix += SourceModel.GetMeshDescriptionBulkData()->GetIdString();
	}

	{
		FBufferArchive Writer;
		Writer << VoxelSize;
		Writer << MaxSmoothness;
		Writer << VoxelizerSettings;

		KeySuffix += "_" + FString::FromHexBlob(Writer.GetData(), Writer.Num());
	}

	const FString DerivedDataKey = FDerivedDataCacheInterface::BuildCacheKey(
		TEXT("VOXELLANDMASSMESHBRUSH"),
		TEXT("08F933C446024945833DF1C52EF6806E"),
		*KeySuffix);

	TArray<uint8> DerivedData;
	if (GetDerivedDataCacheRef().GetSynchronous(*DerivedDataKey, DerivedData, GetPathName()))
	{
		FMemoryReader Ar(DerivedData);

		BrushData = MakeShared<FVoxelLandmassBrushData>();
		BrushData->Serialize(Ar);
	}
	else
	{
		BrushData = FVoxelLandmassBrushData::VoxelizeMesh(*LoadedMesh, VoxelSize, MaxSmoothness, VoxelizerSettings);
		if (!BrushData)
		{
			return;
		}
		
		FBufferArchive Writer;
		BrushData->Serialize(Writer);
		GetDerivedDataCacheRef().Put(*DerivedDataKey, Writer, GetPathName());
	}

	DataSize = BrushData->Size;
	MemorySizeInMB = BrushData->GetAllocatedSize() / double(1 << 20);
#else
	BrushData = FVoxelLandmassBrushData::VoxelizeMesh(*LoadedMesh, VoxelSize, MaxSmoothness, VoxelizerSettings);
#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UVoxelLandmassMeshBrush* UVoxelLandmassMeshBrushAssetUserData::GetBrush(UStaticMesh& Mesh)
{
	UVoxelLandmassMeshBrushAssetUserData* AssetUserData = Mesh.GetAssetUserData<UVoxelLandmassMeshBrushAssetUserData>();
	if (AssetUserData &&
		AssetUserData->Brush.LoadSynchronous() &&
		AssetUserData->Brush->Mesh != &Mesh)
	{
		AssetUserData = nullptr;
	}

	if (AssetUserData && AssetUserData->Brush.LoadSynchronous())
	{
		return AssetUserData->Brush.LoadSynchronous();
	}

	UVoxelLandmassMeshBrush* Brush = nullptr;

	// Try to find an existing one
	{
		TArray<FAssetData> AssetDatas;
		FARFilter Filter;
#if VOXEL_ENGINE_VERSION < 501
		Filter.ClassNames.Add(GetClassFName<UVoxelLandmassMeshBrush>());
#else
		Filter.ClassPaths.Add(UVoxelLandmassMeshBrush::StaticClass()->GetClassPathName());
#endif
		Filter.TagsAndValues.Add(GET_MEMBER_NAME_CHECKED(UVoxelLandmassMeshBrush, Mesh), Mesh.GetPathName());

		const IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
		AssetRegistry.GetAssets(Filter, AssetDatas);

		if (AssetDatas.Num() > 1)
		{
			TArray<UObject*> Assets;
			for (const FAssetData& AssetData : AssetDatas)
			{
				Assets.Add(AssetData.GetAsset());
			}
			VOXEL_MESSAGE(Warning, "More than 1 landmass brush asset for mesh {0} found: {1}", Mesh, Assets);
		}

		for (const FAssetData& AssetData : AssetDatas)
		{
			UObject* Asset = AssetData.GetAsset();
			if (!ensure(Asset) || !ensure(Asset->IsA<UVoxelLandmassMeshBrush>()))
			{
				continue;
			}

			Brush = CastChecked<UVoxelLandmassMeshBrush>(Asset);
			break;
		}

		if (!Brush && AssetRegistry.IsLoadingAssets())
		{
			// Otherwise new assets are created for the same mesh
			VOXEL_MESSAGE(Error, "Asset registry is still loading assets - please refresh the voxel world once assets are loaded");
			return nullptr;
		}
	}
	
#if WITH_EDITOR
	if (!Brush)
	{
		// Create a new brush asset

		FString PackageName = FPackageName::ObjectPathToPackageName(Mesh.GetPathName());
		if (!PackageName.StartsWith("/Game/"))
		{
			// Don't create assets in the engine
			PackageName = "/Game/VoxelLandmassBrushes/" + Mesh.GetName();
		}

		Brush = FVoxelObjectUtilities::CreateNewAsset_Direct<UVoxelLandmassMeshBrush>(PackageName, "_LandmassBrush");

		if (!Brush)
		{
			return nullptr;
		}

		Brush->Mesh = &Mesh;
	}
#endif

	if (!Brush)
	{
		return nullptr;
	}

	AssetUserData = NewObject<UVoxelLandmassMeshBrushAssetUserData>(&Mesh);
	AssetUserData->Brush = Brush;
	ensure(Brush->Mesh == &Mesh);

	Mesh.AddAssetUserData(AssetUserData);
	Mesh.MarkPackageDirty();

	if (!AssetUserData)
	{
		return nullptr;
	}

	return AssetUserData->Brush.LoadSynchronous();
}