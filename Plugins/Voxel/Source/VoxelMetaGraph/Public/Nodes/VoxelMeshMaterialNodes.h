// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelExposedPinType.h"
#include "VoxelMeshMaterialNodes.generated.h"

USTRUCT(DisplayName = "Texture")
struct VOXELMETAGRAPH_API FVoxelMeshTexture
{
	GENERATED_BODY()

	TWeakObjectPtr<UTexture> Texture;
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelMeshTexturePinType : public FVoxelExposedPinType
{
	GENERATED_BODY()

	DEFINE_VOXEL_EXPOSED_PIN_TYPE(FVoxelMeshTexture, TSoftObjectPtr<UTexture>)
	{
		const TSharedRef<FVoxelMeshTexture> Texture = MakeShared<FVoxelMeshTexture>();
		Texture->Texture = Value.LoadSynchronous();
		return Texture;
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(DisplayName = "Material")
struct VOXELMETAGRAPH_API FVoxelMeshMaterial : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	TSharedPtr<FVoxelMaterialRef> Material;
	TFunction<TSharedPtr<FVoxelMaterialRef>()> Getter;

	virtual TSharedRef<FVoxelMaterialRef> GetMaterial_GameThread() const;
};

USTRUCT(DisplayName = "Material Instance")
struct VOXELMETAGRAPH_API FVoxelMeshMaterialInstance : public FVoxelMeshMaterial
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	TSharedPtr<const FVoxelMeshMaterial> Parent;
	FVoxelMaterialParameters Parameters;

	virtual TSharedRef<FVoxelMaterialRef> GetMaterial_GameThread() const override;
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelMeshMaterialPinType : public FVoxelExposedPinType
{
	GENERATED_BODY()

	DEFINE_VOXEL_EXPOSED_PIN_TYPE(FVoxelMeshMaterial, TSoftObjectPtr<UMaterialInterface>)
	{
		const TSharedRef<FVoxelMeshMaterial> Material = MakeShared<FVoxelMeshMaterial>();
		Material->Material = FVoxelMaterialRef::Make(Value.LoadSynchronous());
		return Material;
	}
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelMeshMaterialInstancePinType : public FVoxelExposedPinType
{
	GENERATED_BODY()

	DEFINE_VOXEL_EXPOSED_PIN_TYPE(FVoxelMeshMaterialInstance, TSoftObjectPtr<UMaterialInstanceDynamic>)
	{
		const TSharedRef<FVoxelMeshMaterial> Material = MakeShared<FVoxelMeshMaterial>();
		Material->Material = FVoxelMaterialRef::Make(Value.LoadSynchronous());

		const TSharedRef<FVoxelMeshMaterialInstance> MaterialInstance = MakeShared<FVoxelMeshMaterialInstance>();
		MaterialInstance->Parent = Material;
		return MaterialInstance;
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(Category = "Material")
struct VOXELMETAGRAPH_API FVoxelNode_MakeMaterialInstance : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelMeshMaterial, Parent, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelMeshMaterialInstance, Instance);
};

USTRUCT(Category = "Material")
struct VOXELMETAGRAPH_API FVoxelNode_SetMaterialInstanceScalar : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelMeshMaterialInstance, Instance, nullptr);
	VOXEL_INPUT_PIN(FName, Name, "Value");
	VOXEL_INPUT_PIN(float, Value, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelMeshMaterialInstance, OutInstance);
};

USTRUCT(Category = "Material")
struct VOXELMETAGRAPH_API FVoxelNode_SetMaterialInstanceVector : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelMeshMaterialInstance, Instance, nullptr);
	VOXEL_INPUT_PIN(FName, Name, "Value");
	VOXEL_INPUT_PIN(FLinearColor, Value, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelMeshMaterialInstance, OutInstance);
};

USTRUCT(Category = "Material")
struct VOXELMETAGRAPH_API FVoxelNode_SetMaterialInstanceTexture : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelMeshMaterialInstance, Instance, nullptr);
	VOXEL_INPUT_PIN(FName, Name, "Value");
	VOXEL_INPUT_PIN(FVoxelMeshTexture, Value, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelMeshMaterialInstance, OutInstance);
};