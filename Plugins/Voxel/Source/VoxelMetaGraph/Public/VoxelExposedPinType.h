// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelExposedPinType.generated.h"

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelExposedPinType : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	virtual FVoxelPinType GetComputedType() const VOXEL_PURE_VIRTUAL({});
	virtual FVoxelPinType GetExposedType() const VOXEL_PURE_VIRTUAL({});

	FVoxelPinValue Compute(const FVoxelPinValue& Value, FVoxelRuntime& InRuntime) const;

protected:
	virtual void ComputeImpl(FVoxelPinValue& OutValue, const FVoxelPinValue& Value) const VOXEL_PURE_VIRTUAL();

	template<typename T>
	T& GetSubsystem() const
	{
		check(Runtime);
		return Runtime->GetSubsystem<T>();
	}

private:
	mutable FVoxelRuntime* Runtime = nullptr;
};

#define DEFINE_VOXEL_EXPOSED_PIN_TYPE(ComputedType, ExposedType) \
	GENERATED_VIRTUAL_STRUCT_BODY() \
	void Dummy() { checkStatic(TIsSame<VOXEL_THIS_TYPE, ComputedType ## PinType>::Value); } \
	\
	virtual FVoxelPinType GetComputedType() const override \
	{ \
		return FVoxelPinType::Make<ComputedType>(); \
	} \
	virtual FVoxelPinType GetExposedType() const override \
	{ \
		return FVoxelPinType::Make<ExposedType>(); \
	} \
	virtual void ComputeImpl(FVoxelPinValue& OutValue, const FVoxelPinValue& Value) const override \
	{ \
		const TSharedPtr<ComputedType> Ptr = Compute(Value.Get<ExposedType>()); \
		if (!Ptr) \
		{ \
			return; \
		} \
		OutValue = FVoxelSharedPinValue::Make(Ptr.ToSharedRef()).MakeValue(); \
	} \
	TSharedPtr<ComputedType> Compute(const ExposedType& Value) const