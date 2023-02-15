// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelBlockUtilities.h"
#include "VoxelBlockAsset.h"

bool FVoxelBlockUtilities::BuildTextures(
	const TMap<FVoxelBlockId, TObjectPtr<UVoxelBlockAsset>>& Assets,
	TMap<FVoxelBlockId, int32>& OutScalarIds,
	TMap<FVoxelBlockId, FVoxelBlockFaceIds>& OutFacesIds,
	TMap<FName, TObjectPtr<UTexture2D>>& InOutTextures,
	TMap<FName, TObjectPtr<UTexture2DArray>>& InOutTextureArrays)
{
	VOXEL_FUNCTION_COUNTER();
	ensure(IsInGameThread());

	ensure(OutScalarIds.Num() == 0);
	ensure(OutFacesIds.Num() == 0);

	if (Assets.Num() == 0)
	{
		InOutTextures.Reset();
		InOutTextureArrays.Reset();
		return true;
	}

	const UVoxelBlockAsset* FirstAsset = Assets.CreateConstIterator().Value();
	check(FirstAsset);

	const int32 TextureSize = FirstAsset->TextureSize;
	if (!FMath::IsPowerOfTwo(TextureSize))
	{
		VOXEL_MESSAGE(Error, "{0}: TextureSize is not a power of two!", FirstAsset);
		return false;
	}
	
	TArray<FName> TextureNames;
	TArray<FName> ScalarNames;
	for (const auto& AssetIt : Assets)
	{
		const UVoxelBlockAsset* Asset = AssetIt.Value;
		if (Asset->TextureSize != TextureSize)
		{
			// TODO Allow this
			VOXEL_MESSAGE(Error, "{0}: TextureSize is not the same as {1}'s TextureSize!", Asset, FirstAsset);
			return false;
		}

		for (const FProperty& Property : GetStructProperties<FVoxelBlockFaceTextures>(Asset->GetClass()))
		{
			TextureNames.AddUnique(Property.GetFName());
		}

		for (const FProperty& Property : GetStructProperties(Asset->GetClass()))
		{
			if (!Property.GetName().StartsWith(MaterialParamPrefix))
			{
				continue;
			}

			if (!Property.IsA<FFloatProperty>() &&
				!Property.IsA<FDoubleProperty>())
			{
				continue;
			}

			ScalarNames.AddUnique(Property.GetFName());
		}
	}

	TArray<FScalarSet> ScalarSets;
	FindScalars(ScalarNames, Assets, OutScalarIds, ScalarSets);

	TArray<FTextureSet> TextureSets;
	FindTextures(TextureSize, TextureNames, Assets, OutFacesIds, TextureSets);

	if (ScalarSets.Num() != 0)
	{
		BuildScalarTextures(ScalarNames, ScalarSets, InOutTextures);
	}

	if (TextureSets.Num() != 0)
	{
		BuildTextureArrays(TextureSize, TextureNames, TextureSets, InOutTextureArrays);
	}

	return true;
}

FVoxelMaterialParameters FVoxelBlockUtilities::GetInstanceParameters(
	const TMap<FName, TObjectPtr<UTexture2D>>& Textures,
	const TMap<FName, TObjectPtr<UTexture2DArray>>& TextureArrays)
{
	VOXEL_FUNCTION_COUNTER();
	ensure(IsInGameThread());

	FVoxelMaterialParameters Parameters;

	for (const auto& It : Textures)
	{
		if (!ensure(It.Value))
		{
			continue;
		}
		Parameters.TextureParameters.Add(It.Key, It.Value);
		Parameters.ScalarParameters.Add(It.Key + "_Size", It.Value->GetSizeX());
	}

	for (const auto& It : TextureArrays)
	{
		if (!ensure(It.Value))
		{
			continue;
		}
		Parameters.TextureParameters.Add(It.Key, It.Value);
		Parameters.ScalarParameters.Add(It.Key + "_Size", It.Value->GetSizeX());
	}

	return Parameters;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelBlockUtilities::FindScalars(
	const TArray<FName>& ScalarNames,
	const TMap<FVoxelBlockId, TObjectPtr<UVoxelBlockAsset>>& Assets,
	TMap<FVoxelBlockId, int32>& OutScalarIds,
	TArray<FScalarSet>& OutScalarSets)
{
	VOXEL_FUNCTION_COUNTER();
	
	TMap<FScalarSet, int32> ScalarIds;
	for (const auto& AssetIt : Assets)
	{
		const UVoxelBlockAsset* Asset = AssetIt.Value;

		FScalarSet ScalarSet;
		ScalarSet.Scalars.SetNumZeroed(ScalarNames.Num());
		
		for (const FProperty& Property : GetStructProperties(Asset->GetClass()))
		{
			if (!Property.GetName().StartsWith(MaterialParamPrefix))
			{
				continue;
			}

			if (!Property.IsA<FFloatProperty>() &&
				!Property.IsA<FDoubleProperty>())
			{
				continue;
			}
		
			const int32 ScalarIndex = ScalarNames.IndexOfByKey(Property.GetFName());
			check(ScalarIndex != -1);

			ScalarSet.Scalars[ScalarIndex] = Property.IsA<FFloatProperty>()
				? CastFieldChecked<FFloatProperty>(Property).GetPropertyValue_InContainer(Asset)
				: CastFieldChecked<FDoubleProperty>(Property).GetPropertyValue_InContainer(Asset);
		}

		const int32* Id = ScalarIds.Find(ScalarSet);
		if (!Id)
		{
			Id = &ScalarIds.Add(ScalarSet, OutScalarSets.Add(ScalarSet));
		}

		OutScalarIds.Add(AssetIt.Key, *Id);
	}
}

void FVoxelBlockUtilities::FindTextures(
	int32 TextureSize,
	const TArray<FName>& TextureNames,
	const TMap<FVoxelBlockId, TObjectPtr<UVoxelBlockAsset>>& Assets,
	TMap<FVoxelBlockId, FVoxelBlockFaceIds>& OutFacesIds,
	TArray<FTextureSet>& OutTextureSets)
{
	VOXEL_FUNCTION_COUNTER();

	TMap<FTextureSet, int32> TextureIds;
	for (const auto& AssetIt : Assets)
	{
		const UVoxelBlockAsset* Asset = AssetIt.Value;

		FVoxelBlockFaceIds FaceIds;
		for (const EVoxelBlockFace Face : TEnumRange<EVoxelBlockFace>())
		{
			FTextureSet FaceTextureSet;
			FaceTextureSet.Textures.SetNum(TextureNames.Num());
			
			for (auto& It : CreatePropertyValueIterator<FVoxelBlockFaceTextures>(Asset))
			{
				const FProperty& Property = It.Key();
				const FVoxelBlockFaceTextures& FaceTextures = It.Value();
			
				const int32 TextureIndex = TextureNames.IndexOfByKey(Property.GetFName());
				check(TextureIndex != -1);

				UTexture2D* Texture = FaceTextures.GetTexture(Face);
				if (!Texture)
				{
					// Will be black, nothing to do
					continue;
				}
				FVoxelTextureUtilities::FullyLoadTexture(Texture);

				const FString TextureDebugName = FString::Printf(TEXT(".%s.%s"), *Property.GetName(), *UEnum::GetDisplayValueAsText(Face).ToString());
				
				if (Texture->GetSizeX() != TextureSize ||
					Texture->GetSizeY() != TextureSize)
				{
					VOXEL_MESSAGE(Error, "{0}{1} texture size is {2}x{3} instead of {4}x{4}. Change {5}.TextureSize if you want to use a different texture size",
						Asset,
						TextureDebugName,
						Texture->GetSizeX(),
						Texture->GetSizeY(),
						TextureSize,
						Asset->GetClass());

					continue;
				}

				if (!Texture->GetPlatformData() ||
					Texture->GetPlatformData()->Mips.Num() == 0)
				{
					VOXEL_MESSAGE(Error, "{0}{1} is invalid!", Asset, TextureDebugName);
					continue;
				}

				if (Asset->bAutomaticallyFixTextures && GIsEditor)
				{
					const TextureFilter Filter = Asset->bDisableTextureFiltering ? TF_Nearest : TF_Default;

					if (Texture->CompressionSettings != TC_EditorIcon ||
						Texture->Filter != Filter)
					{
						Texture->CompressionSettings = TC_EditorIcon;
						Texture->Filter = Filter;
						Texture->UpdateResource();
						Texture->MarkPackageDirty();
					}
				}

				if (Texture->CompressionSettings != TC_EditorIcon)
				{
					VOXEL_MESSAGE(Error, "{0}{1} compression settings is not UserInterface2D!", Asset, TextureDebugName);
					continue;
				}

				FaceTextureSet.Textures[TextureIndex] = Texture;
			}

			const int32* Id = TextureIds.Find(FaceTextureSet);
			if (!Id)
			{
				Id = &TextureIds.Add(FaceTextureSet, OutTextureSets.Add(FaceTextureSet));
			}

			FaceIds.GetId(Face) = *Id;
		}
		OutFacesIds.Add(AssetIt.Key, FaceIds);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelBlockUtilities::BuildScalarTextures(
	const TArray<FName>& ScalarNames,
	const TArray<FScalarSet>& ScalarSets,
	TMap<FName, TObjectPtr<UTexture2D>>& InOutTextures)
{
	VOXEL_FUNCTION_COUNTER();

	for (int32 ScalarIndex = 0; ScalarIndex < ScalarNames.Num(); ScalarIndex++)
	{
		TObjectPtr<UTexture2D>& Texture = InOutTextures.FindOrAdd(ScalarNames[ScalarIndex]);

		if (!Texture ||
			Texture->GetPixelFormat() != PF_R32_FLOAT ||
			Texture->GetSizeX() != ScalarSets.Num() ||
			Texture->GetSizeY() != 1)
		{
			Texture = UTexture2D::CreateTransient(ScalarSets.Num(), 1, PF_R32_FLOAT);
		}
		Texture->CompressionSettings = TC_HDR;
		Texture->SRGB = false;
		Texture->Filter = TF_Nearest;

		if (!ensure(Texture->GetPlatformData()) ||
			!ensure(Texture->GetPlatformData()->Mips.Num() > 0))
		{
			continue;
		}

		FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];

		float* Data = reinterpret_cast<float*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
		if (!ensure(Data))
		{
			continue;
		}

		for (int32 ScalarSetIndex = 0; ScalarSetIndex < ScalarSets.Num(); ScalarSetIndex++)
		{
			Data[ScalarSetIndex] = ScalarSets[ScalarSetIndex].Scalars[ScalarIndex];
		}

		Mip.BulkData.Unlock();

		Texture->UpdateResource();
	}
}

void FVoxelBlockUtilities::BuildTextureArrays(
	int32 TextureSize,
	const TArray<FName>& TextureNames,
	const TArray<FTextureSet>& TextureSets,
	TMap<FName, TObjectPtr<UTexture2DArray>>& InOutTextureArrays)
{
	VOXEL_FUNCTION_COUNTER();
	
	for (int32 TextureIndex = 0; TextureIndex < TextureNames.Num(); TextureIndex++)
	{
		TObjectPtr<UTexture2DArray>& TextureArray = InOutTextureArrays.FindOrAdd(TextureNames[TextureIndex]);
		if (!TextureArray)
		{
			TextureArray = NewObject<UTexture2DArray>();
		}

#if VOXEL_ENGINE_VERSION < 501
		delete TextureArray->PlatformData;
#else
		delete TextureArray->GetPlatformData();
#endif

		FTexturePlatformData* PlatformData = new FTexturePlatformData();
		PlatformData->SizeX = TextureSize;
		PlatformData->SizeY = TextureSize;
		PlatformData->SetNumSlices(TextureSets.Num());
		PlatformData->PixelFormat = PF_B8G8R8A8;

#if VOXEL_ENGINE_VERSION < 501
		TextureArray->PlatformData = PlatformData;
#else
		TextureArray->SetPlatformData(PlatformData);
#endif

		{
			int32 MipSize = TextureSize;

			while (MipSize)
			{
				FTexture2DMipMap* Mip = new FTexture2DMipMap();
				PlatformData->Mips.Add(Mip);
				Mip->SizeX = MipSize;
				Mip->SizeY = MipSize;
				Mip->SizeZ = TextureSets.Num();
				Mip->BulkData.Lock(LOCK_READ_WRITE);
				Mip->BulkData.Realloc(MipSize * MipSize * TextureSets.Num() * sizeof(FColor));
				Mip->BulkData.Unlock();

				MipSize /= 2;
			}
		}

		UObject* TextureUsedForFilter = nullptr;
		for (int32 TextureSetIndex = 0; TextureSetIndex < TextureSets.Num(); TextureSetIndex++)
		{
			TArray<FColor> Buffer;
			Buffer.SetNum(TextureSize * TextureSize);

			// Copy texture to Buffer
			if (UTexture2D* Texture = TextureSets[TextureSetIndex].Textures[TextureIndex])
			{
				FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
				if (!ensure(Mip.BulkData.GetBulkDataSize() > 0) ||
					!ensure(Mip.SizeX == Texture->GetSizeX()) ||
					!ensure(Mip.SizeY == Texture->GetSizeY()))
				{
					continue;
				}

				const void* Data = Mip.BulkData.Lock(LOCK_READ_ONLY);
				if (ensure(Data))
				{
					FMemory::Memcpy(Buffer.GetData(), Data, TextureSize * TextureSize * sizeof(FColor));
				}
				Mip.BulkData.Unlock();

				if (!TextureUsedForFilter)
				{
					TextureArray->Filter = Texture->Filter;
					TextureUsedForFilter = Texture;
				}
				else if (TextureArray->Filter != Texture->Filter)
				{
					VOXEL_MESSAGE(Error,
						"{0} and {1} have different filter settings, but are used by block assets in the same texture array!\n"
						"You most likely want to set them both to be Nearest",
						Texture,
						TextureUsedForFilter);
				}
			}
			else
			{
				for (FColor& Color : Buffer)
				{
					Color = FColor::Black;
				}
			}

			// Copy main mip
			{
				FTexture2DMipMap& Mip = PlatformData->Mips[0];
				void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
				FMemory::Memcpy(
					static_cast<FColor*>(Data) + TextureSize * TextureSize * TextureSetIndex,
					Buffer.GetData(),
					TextureSize * TextureSize * sizeof(FColor));
				Mip.BulkData.Unlock();
			}

			// Generate mips & copy them
			check(FMath::IsPowerOfTwo(TextureSize));
			check(FMath::IsPowerOfTwo(TextureSize));
			for (int32 MipIndex = 1; MipIndex < TextureArray->GetNumMips(); MipIndex++)
			{
				const int32 MipSize = TextureSize / (1 << MipIndex);
				check(MipSize > 0);
				for (int32 Y = 0; Y < MipSize; Y++)
				{
					for (int32 X = 0; X < MipSize; X++)
					{
						FLinearColor Color{ ForceInit };
						Color += FLinearColor(Buffer[(2 * X + 0) + (2 * Y + 0) * MipSize * 2]);
						Color += FLinearColor(Buffer[(2 * X + 1) + (2 * Y + 0) * MipSize * 2]);
						Color += FLinearColor(Buffer[(2 * X + 0) + (2 * Y + 1) * MipSize * 2]);
						Color += FLinearColor(Buffer[(2 * X + 1) + (2 * Y + 1) * MipSize * 2]);
						Buffer[X + Y * MipSize] = (Color / 4).ToFColor(true);
					}
				}

				FTexture2DMipMap& Mip = PlatformData->Mips[MipIndex];
				void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
				FMemory::Memcpy(
					static_cast<FColor*>(Data) + MipSize * MipSize * TextureSetIndex,
					Buffer.GetData(),
					MipSize * MipSize * sizeof(FColor));
				Mip.BulkData.Unlock();
			}
		}
		
		TextureArray->UpdateResource();
	}
}