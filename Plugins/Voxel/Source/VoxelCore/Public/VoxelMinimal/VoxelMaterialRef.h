// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"

struct FVoxelInstanceRef;
class UMaterialInterface;
class UMaterialInstanceDynamic;

class VOXELCORE_API FVoxelMaterialParameters
{
public:
	TMap<FName, float> ScalarParameters;
	TMap<FName, FVector4f> VectorParameters;
	TMap<FName, TWeakObjectPtr<UTexture>> TextureParameters;

	TArray<TSharedPtr<FVirtualDestructor>> Resources;

	FVoxelMaterialParameters() = default;

	void ApplyTo(UMaterialInstanceDynamic* Instance) const;
	
public:
	FORCEINLINE friend uint32 GetTypeHash(const FVoxelMaterialParameters& Parameters)
	{
		// Ignore Resources
		return FVoxelUtilities::MurmurHashMulti(
			Parameters.ScalarParameters.Num(),
			Parameters.VectorParameters.Num(),
			Parameters.TextureParameters.Num());
	}
	bool operator==(const FVoxelMaterialParameters& Other) const
	{
		// Ignore Resources
		return
			ScalarParameters.OrderIndependentCompareEqual(Other.ScalarParameters) &&
			VectorParameters.OrderIndependentCompareEqual(Other.VectorParameters) &&
			TextureParameters.OrderIndependentCompareEqual(Other.TextureParameters);
	}
};

class VOXELCORE_API FVoxelMaterialRef : public FVirtualDestructor
{
public:
	static TSharedRef<FVoxelMaterialRef> Default();
	static TSharedRef<FVoxelMaterialRef> Make(UMaterialInterface* Material);
	// Will handle Parent being an instance
	static TSharedRef<FVoxelMaterialRef> MakeInstance(UMaterialInterface* Parent);
	static TSharedRef<FVoxelMaterialRef> MakeInstance(UMaterialInterface* Parent, const FVoxelMaterialParameters& Parameters);

public:
	virtual ~FVoxelMaterialRef() override;
	UE_NONCOPYABLE(FVoxelMaterialRef);

	// Will be null if the asset is force deleted
	UMaterialInterface* GetMaterial() const
	{
		return Material;
	}
	UMaterialInstanceDynamic* GetMaterialInstance() const
	{
		ensure(IsInGameThread());
		ensure(IsInstance());
		return Cast<UMaterialInstanceDynamic>(GetMaterial());
	}

	// If true, this a material instance the plugin created & we can set parameters on it
	bool IsInstance() const
	{
		return InstanceRef.IsValid();
	}

	void AddResource(const TSharedPtr<FVirtualDestructor>& Resource)
	{
		Resources.Enqueue(Resource);
	}

private:
	FVoxelMaterialRef() = default;

	UMaterialInterface* Material = nullptr;
	TSharedPtr<FVoxelInstanceRef> InstanceRef;
	TQueue<TSharedPtr<FVirtualDestructor>, EQueueMode::Mpsc> Resources;

	friend class FVoxelMaterialRefManager;
};