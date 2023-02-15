// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMaterialParameters.generated.h"

USTRUCT()
struct VOXELMATERIAL_API FVoxelMaterialParameter : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	virtual void Fixup() {}
	virtual bool Initialize() { return true; }
};

USTRUCT()
struct VOXELMATERIAL_API FVoxelMaterialParameterData : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	FName GetName() const
	{
		return PrivateName;
	}

	void Initialize(FName Name, const FVoxelMaterialParameter& DefaultParameter);
	void AddParameter(const FVoxelMaterialParameter& Parameter);
	void FlushChanges();

	virtual void Create(const TArray<const FVoxelMaterialParameter*>& Parameters) VOXEL_PURE_VIRTUAL();
	virtual void SetupMaterial(UMaterialInstanceDynamic& Material) const VOXEL_PURE_VIRTUAL();

private:
	FName PrivateName;
	bool bDirty = false;

	UPROPERTY(Transient)
	TArray<FVoxelInstancedStruct> PrivateParameters;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct VOXELMATERIAL_API FVoxelMaterialScalarParameter : public FVoxelMaterialParameter
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	float Value = 0.f;
};

USTRUCT()
struct VOXELMATERIAL_API FVoxelMaterialScalarParameterData : public FVoxelMaterialParameterData
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	virtual void Create(const TArray<const FVoxelMaterialParameter*>& Parameters) override;
	virtual void SetupMaterial(UMaterialInstanceDynamic& Material) const override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> Texture = nullptr;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UENUM(BlueprintType)
enum class EVoxelMaterialLayerTextureCompression : uint8
{
	DXT1
};

FORCEINLINE EPixelFormat GetPixelFormat(EVoxelMaterialLayerTextureCompression Compression)
{
	switch (Compression)
	{
	default: ensure(false);
	case EVoxelMaterialLayerTextureCompression::DXT1: return PF_DXT1;
	}
}

USTRUCT(BlueprintType)
struct VOXELMATERIAL_API FVoxelMaterialTextureParameter : public FVoxelMaterialParameter
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()
		
public:
	UPROPERTY(EditDefaultsOnly, Category = "Voxel")
	int32 TextureSize = 1024;

	UPROPERTY(EditDefaultsOnly, Category = "Voxel")
	int32 LastMipTextureSize = 16;
	
	UPROPERTY(EditDefaultsOnly, Category = "Voxel")
	EVoxelMaterialLayerTextureCompression Compression = EVoxelMaterialLayerTextureCompression::DXT1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	TObjectPtr<UTexture2D> Texture;

	int32 GetNumMips() const
	{
		ensureVoxelSlow(TextureSize % LastMipTextureSize == 0);
		return 1 + FVoxelUtilities::ExactLog2(TextureSize / LastMipTextureSize);
	}

public:
	virtual void Fixup() override;
	virtual bool Initialize() override;

public:
	using FCopyData = TFunction<void(TVoxelArrayView<uint8> OutData, int32 MipIndex)>;
	FCopyData CopyData;
};

USTRUCT()
struct VOXELMATERIAL_API FVoxelMaterialTextureParameterData : public FVoxelMaterialParameterData
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()
		
public:
	UPROPERTY()
	FVoxelMaterialTextureParameter Settings;

	virtual void Create(const TArray<const FVoxelMaterialParameter*>& Parameters) override;
	virtual void SetupMaterial(UMaterialInstanceDynamic& Material) const override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTexture2DArray> TextureArray = nullptr;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct VOXELMATERIAL_API FVoxelMaterialStreamedTextureParameter : public FVoxelMaterialParameter
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Voxel")
	int32 HighResTextureSize = 1024;

	UPROPERTY(EditDefaultsOnly, Category = "Voxel")
	int32 LowResTextureSize = 512;
	
	UPROPERTY(EditDefaultsOnly, Category = "Voxel")
	int32 NumHighRes = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	TObjectPtr<UTexture2D> Texture;
};