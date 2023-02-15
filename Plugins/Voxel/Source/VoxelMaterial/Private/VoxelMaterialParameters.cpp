// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMaterialParameters.h"

void FVoxelMaterialParameterData::Initialize(FName Name, const FVoxelMaterialParameter& DefaultParameter)
{
	ensure(PrivateName.IsNone());
	PrivateName = Name;

	if (const FStructProperty* StructProperty = CastField<FStructProperty>(GetStruct()->FindPropertyByName("Settings")))
	{
		check(StructProperty->Struct == DefaultParameter.GetStruct());
		void* Settings = StructProperty->ContainerPtrToValuePtr<void>(this);

		for (const FProperty& Property : GetStructProperties(DefaultParameter.GetStruct()))
		{
			if (!Property.HasAllPropertyFlags(CPF_Edit | CPF_DisableEditOnInstance))
			{
				continue;
			}

			Property.CopyCompleteValue_InContainer(Settings, &DefaultParameter);
		}
	}
}

void FVoxelMaterialParameterData::AddParameter(const FVoxelMaterialParameter& Parameter)
{
	FVoxelInstancedStruct ParameterInstance = FVoxelInstancedStruct::Make(Parameter);
	if (!ParameterInstance.Get<FVoxelMaterialParameter>().Initialize())
	{
		return;
	}

	PrivateParameters.Add(MoveTemp(ParameterInstance));
	bDirty = true;
}

void FVoxelMaterialParameterData::FlushChanges()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	if (!bDirty)
	{
		return;
	}
	bDirty = false;

	TArray<const FVoxelMaterialParameter*> Parameters;
	for (const FVoxelInstancedStruct& Parameter : PrivateParameters)
	{
		Parameters.Add(&Parameter.Get<FVoxelMaterialParameter>());
	}

	Create(Parameters);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMaterialScalarParameterData::Create(const TArray<const FVoxelMaterialParameter*>& Parameters)
{
	VOXEL_FUNCTION_COUNTER();

	TArray<float> Values;
	for (const FVoxelMaterialScalarParameter* Parameter : CastChecked<const FVoxelMaterialScalarParameter>(Parameters))
	{
		Values.Add(Parameter->Value);
	}

	if (Texture)
	{
		FVoxelTextureUtilities::DestroyTexture(Texture);
		Texture = nullptr;
	}

	Texture = FVoxelTextureUtilities::CreateTexture2D(
		"FVoxelMaterialScalarParameter_" + GetName(),
		Values.Num(),
		1,
		false,
		TF_Nearest,
		PF_R32_FLOAT,
		[&](TVoxelArrayView<uint8> Data)
		{
			FVoxelUtilities::Memcpy(ReinterpretCastVoxelArrayView<float>(Data), Values);
		});
}

void FVoxelMaterialScalarParameterData::SetupMaterial(UMaterialInstanceDynamic& Material) const
{
	VOXEL_FUNCTION_COUNTER();

	Material.SetTextureParameterValue(GetName(), Texture);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMaterialTextureParameter::Fixup()
{
	TextureSize = FMath::RoundUpToPowerOfTwo(FMath::Clamp(TextureSize, 16, 8192));
	LastMipTextureSize = FMath::RoundUpToPowerOfTwo(FMath::Clamp(LastMipTextureSize, 16, TextureSize));
}

bool FVoxelMaterialTextureParameter::Initialize()
{
	VOXEL_FUNCTION_COUNTER();

	if (!Texture)
	{
		VOXEL_MESSAGE(Error, "No texture");
		return false;
	}

	FVoxelTextureUtilities::FullyLoadTexture(Texture);
	
	if (Texture->GetSizeX() != Texture->GetSizeY())
	{
		VOXEL_MESSAGE(Error, "Texture {0} is not a square", Texture);
		return false;
	}

	const int32 Size = Texture->GetSizeX();
	if (!FMath::IsPowerOfTwo(Size))
	{
		VOXEL_MESSAGE(Error, "Texture {0} size is not a power of two", Texture);
		return false;
	}

	if (TextureSize > Size)
	{
		VOXEL_MESSAGE(Error, "Texture {0} has size {1} which is less than required texture size {2}", Texture, Size, TextureSize);
		return false;
	}

	if (!ensure(Size % TextureSize == 0))
	{
		VOXEL_MESSAGE(Error, "Invalid size");
		return false;
	}

	const int32 MipOffset = FVoxelUtilities::ExactLog2(Size / TextureSize);

	FTexturePlatformData* PlatformData = Texture->GetPlatformData();
	if (!ensure(PlatformData))
	{
		VOXEL_MESSAGE(Error, "Invalid platform data");
		return false;
	}

	if (PlatformData->PixelFormat != GetPixelFormat(Compression))
	{
		VOXEL_MESSAGE(Error, "Texture {0} should have compression set to {1}, current format is {2}",
			Texture,
			GPixelFormats[GetPixelFormat(Compression)].Name,
			GPixelFormats[PlatformData->PixelFormat].Name);
		return false;
	}

	if (!ensure(MipOffset + GetNumMips() <= PlatformData->Mips.Num()))
	{
		VOXEL_MESSAGE(Error, "Missing mips");
		return false;
	}

	CopyData = [=](TVoxelArrayView<uint8> OutData, int32 MipIndex)
	{
		const int32 NumMips = Texture->GetNumMips();

		// TODO Cleanup
		TArray<void*> MipData;
		MipData.SetNumZeroed(NumMips - MipOffset);
		Texture->GetMipData(MipOffset, MipData.GetData());
		ON_SCOPE_EXIT
		{
			for (void* Data : MipData)
			{
				FMemory::Free(Data);
			}
		};

		const void* Data = MipData[MipIndex];
		if (!ensure(Data))
		{
			FVoxelUtilities::Memzero(OutData);
			return;
		}

		FMemory::Memcpy(OutData.GetData(), Data, OutData.Num());
	};

	return true;
}

void FVoxelMaterialTextureParameterData::Create(const TArray<const FVoxelMaterialParameter*>& Parameters)
{
	VOXEL_FUNCTION_COUNTER();

	const int32 NumLayers = Parameters.Num();

	TArray<FVoxelMaterialTextureParameter::FCopyData> CopyDatas;
	for (const FVoxelMaterialTextureParameter* Parameter : CastChecked<const FVoxelMaterialTextureParameter>(Parameters))
	{
		ensure(Settings.TextureSize == Parameter->TextureSize);
		ensure(Settings.Compression == Parameter->Compression);
		CopyDatas.Add(Parameter->CopyData);
	}

	if (TextureArray)
	{
		FVoxelTextureUtilities::DestroyTexture(TextureArray);
		TextureArray = nullptr;
	}

	TextureArray = FVoxelTextureUtilities::CreateTextureArray(
		"FVoxelMaterialTextureParameter_" + GetName(),
		Settings.TextureSize, 
		Settings.TextureSize,
		NumLayers,
		false,
		TF_Bilinear,
		GetPixelFormat(Settings.Compression),
		Settings.GetNumMips(),
		[&](const TVoxelArrayView<uint8> Data, int32 MipIndex)
		{
			check(Data.Num() % NumLayers == 0);
			const int32 LayerSize = Data.Num() / NumLayers;

			for (int32 Index = 0; Index < NumLayers; Index++)
			{
				CopyDatas[Index](Data.Slice(Index * LayerSize, LayerSize), MipIndex);
			}
		});
}

void FVoxelMaterialTextureParameterData::SetupMaterial(UMaterialInstanceDynamic& Material) const
{
	VOXEL_FUNCTION_COUNTER();

	Material.SetTextureParameterValue(GetName(), TextureArray);
}