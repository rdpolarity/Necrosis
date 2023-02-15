// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "Nodes/VoxelMeshMaterialNodes.h"

TSharedRef<FVoxelMaterialRef> FVoxelMeshMaterial::GetMaterial_GameThread() const
{
	ensure(!Getter || !Material);

	const TSharedPtr<FVoxelMaterialRef> MaterialRef = Getter ? Getter() : Material;
	if (!MaterialRef)
	{
		return FVoxelMaterialRef::Default();
	}
	return MaterialRef.ToSharedRef();
}

///////////////////////////////////////////////////////////////////////////////

TSharedRef<FVoxelMaterialRef> FVoxelMeshMaterialInstance::GetMaterial_GameThread() const
{
	UMaterialInterface* LocalParent = nullptr;
	FVoxelMaterialParameters LocalParameters = Parameters;

	if (Parent)
	{
		const TSharedRef<FVoxelMaterialRef> ParentRef = Parent->GetMaterial_GameThread();

		LocalParent = ParentRef->GetMaterial();

		// Make sure to keep the parent resources alive
		LocalParameters.Resources.Add(ParentRef);
	}

	return FVoxelMaterialRef::MakeInstance(LocalParent, LocalParameters);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_MakeMaterialInstance, Instance)
{
	const TValue<FVoxelMeshMaterial> Parent = Get(ParentPin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Parent)
	{
		const TSharedRef<FVoxelMeshMaterialInstance> NewInstance = MakeShared<FVoxelMeshMaterialInstance>();
		NewInstance->Parent = Parent;
		return NewInstance;
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_SetMaterialInstanceScalar, OutInstance)
{
	const TValue<FVoxelMeshMaterialInstance> Instance = Get(InstancePin, Query);
	const TValue<FName> Name = Get(NamePin, Query);
	const TValue<float> Value = Get(ValuePin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Instance, Name, Value)
	{
		const TSharedRef<FVoxelMeshMaterialInstance> NewInstance = Instance->MakeSharedCopy();
		NewInstance->Parameters.ScalarParameters.Add(Name, Value);
		return NewInstance;
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename T>
auto TypeHack() -> decltype(auto)
{
	return T();
}

template<typename T>
struct TChoose
{
	using Type = TChooseClass<false, float, bool>::Result;
};

DEFINE_VOXEL_NODE(FVoxelNode_SetMaterialInstanceVector, OutInstance)
{
	const TValue<FVoxelMeshMaterialInstance> Instance = Get(InstancePin, Query);
	const TValue<FName> Name = Get(NamePin, Query);
	const TValue<FLinearColor> Value = Get(ValuePin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Instance, Name, Value)
	{
		const TSharedRef<FVoxelMeshMaterialInstance> NewInstance = Instance->MakeSharedCopy();
		NewInstance->Parameters.VectorParameters.Add(Name, Value);
		return NewInstance;
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE(FVoxelNode_SetMaterialInstanceTexture, OutInstance)
{
	const TValue<FVoxelMeshMaterialInstance> Instance = Get(InstancePin, Query);
	const TValue<FName> Name = Get(NamePin, Query);
	const TValue<FVoxelMeshTexture> Value = Get(ValuePin, Query);

	return VOXEL_ON_COMPLETE(AnyThread, Instance, Name, Value)
	{
		const TSharedRef<FVoxelMeshMaterialInstance> NewInstance = Instance->MakeSharedCopy();
		NewInstance->Parameters.TextureParameters.Add(Name, Value.Texture);
		return NewInstance;
	};
}