// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelPositionNodes.generated.h"

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelGradientStepQueryData : public FVoxelQueryData
{
	GENERATED_BODY()
	GENERATED_VOXEL_QUERY_DATA_BODY()

public:
	float Step = 0.f;

private:
	uint64 GetHash() const
	{
		return FVoxelUtilities::MurmurHash(Step);
	}
	bool Identical(const FVoxelGradientStepQueryData& Other) const
	{
		return Step == Other.Step;
	}
};

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelPositionQueryData : public FVoxelQueryData
{
	GENERATED_BODY()
	GENERATED_VOXEL_QUERY_DATA_BODY()

public:
	virtual bool Is2D() const { return false; }
	virtual int32 GetGradientStride(EVoxelAxis Axis) const { return -1; }
	virtual FVoxelVectorBuffer GetPositions() const VOXEL_PURE_VIRTUAL({});
	virtual TSharedPtr<FVoxelPositionQueryData> TryCull(const FVoxelBox& Bounds) const { return nullptr; }

private:
	uint64 GetHash() const
	{
		return 0;
	}
	bool Identical(const FVoxelPositionQueryData& Other) const
	{
		return true;
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelGradientPositionQueryData : public FVoxelPositionQueryData
{
	GENERATED_BODY()
	GENERATED_VOXEL_QUERY_DATA_BODY()

public:
	virtual bool Is2D() const override
	{
		return bIs2D;
	}
	virtual int32 GetGradientStride(EVoxelAxis Axis) const override
	{
		switch (Axis)
		{
		default: ensure(false);
		case EVoxelAxis::X: return StrideX;
		case EVoxelAxis::Y: return StrideY;
		case EVoxelAxis::Z: return StrideZ;
		}
	}
	virtual FVoxelVectorBuffer GetPositions() const override
	{
		return PrivatePositions;
	}

	void Initialize(
		bool bInIs2D,
		EVoxelAxis Axis,
		const FVoxelVectorBuffer& Positions,
		const FVoxelGradientPositionQueryData* ExistingQueryData);

private:
	bool bIs2D = false;
	int32 StrideX = -1;
	int32 StrideY = -1;
	int32 StrideZ = -1;
	FVoxelVectorBuffer PrivatePositions;

	uint64 GetHash() const
	{
		return
			FVoxelUtilities::MurmurHash(StrideX, 0) ^
			FVoxelUtilities::MurmurHash(StrideY, 1) ^
			FVoxelUtilities::MurmurHash(StrideZ, 2) ^
			PrivatePositions.GetHash();
	}
	bool Identical(const FVoxelGradientPositionQueryData& Other) const
	{
		return
			bIs2D == Other.bIs2D &&
			StrideX == Other.StrideX &&
			StrideY == Other.StrideY &&
			StrideZ == Other.StrideZ &&
			PrivatePositions.Identical(PrivatePositions);
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelDensePositionQueryData : public FVoxelPositionQueryData
{
	GENERATED_BODY()
	GENERATED_VOXEL_QUERY_DATA_BODY()

public:
	virtual int32 GetGradientStride(EVoxelAxis Axis) const override
	{
		switch (Axis)
		{
		default: ensure(false);
		case EVoxelAxis::X: return 1;
		case EVoxelAxis::Y: return 2;
		case EVoxelAxis::Z: return 4;
		}
	}
	virtual FVoxelVectorBuffer GetPositions() const override
	{
		return CachedPositions;
	}
	virtual TSharedPtr<FVoxelPositionQueryData> TryCull(const FVoxelBox& Bounds) const override;

	const FVector3f& GetStart() const
	{
		return PrivateStart;
	}
	float GetStep() const
	{
		return PrivateStep;
	}
	const FIntVector& GetSize() const
	{
		return PrivateSize;
	}

	FVoxelBox GetBounds() const
	{
		return FVoxelBox(
			FVector(PrivateStart),
			FVector(PrivateStart) + PrivateStep * FVector(PrivateSize));
	}

	void Initialize(
		const FVector3f& Start,
		float Step,
		const FIntVector& Size);
	
private:
	FVector3f PrivateStart = FVector3f::ZeroVector;
	float PrivateStep = 0.f;
	FIntVector PrivateSize = FIntVector::ZeroValue;

	FVoxelVectorBuffer CachedPositions;

	uint64 GetHash() const
	{
		return
			FVoxelUtilities::MurmurHash(PrivateStart) ^
			FVoxelUtilities::MurmurHash(PrivateStep) ^
			FVoxelUtilities::MurmurHash(PrivateSize);
	}
	bool Identical(const FVoxelDensePositionQueryData& Other) const
	{
		return
			PrivateStart == Other.PrivateStart &&
			PrivateStep == Other.PrivateStep &&
			PrivateSize == Other.PrivateSize;
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelDense2DPositionQueryData : public FVoxelPositionQueryData
{
	GENERATED_BODY()
	GENERATED_VOXEL_QUERY_DATA_BODY()

public:
	virtual bool Is2D() const override
	{
		return true;
	}
	virtual int32 GetGradientStride(EVoxelAxis Axis) const override
	{
		switch (Axis)
		{
		default: ensure(false);
		case EVoxelAxis::X: return 1;
		case EVoxelAxis::Y: return 2;
		case EVoxelAxis::Z: return -1;
		}
	}
	virtual FVoxelVectorBuffer GetPositions() const override
	{
		return CachedPositions;
	}

	FVoxelBox2D GetBounds() const
	{
		return FVoxelBox2D(
			FVector2D(PrivateStart),
			FVector2D(PrivateStart) + PrivateStep * FVector2D(PrivateSize));
	}

	void Initialize(
		const FVector2f& Start,
		float Step,
		const FIntPoint& Size);
	
private:
	FVector2f PrivateStart = FVector2f::ZeroVector;
	float PrivateStep = 0.f;
	FIntPoint PrivateSize = FIntPoint::ZeroValue;

	FVoxelVectorBuffer CachedPositions;

	uint64 GetHash() const
	{
		return
			FVoxelUtilities::MurmurHash(PrivateStart) ^
			FVoxelUtilities::MurmurHash(PrivateStep) ^
			FVoxelUtilities::MurmurHash(PrivateSize);
	}
	bool Identical(const FVoxelDense2DPositionQueryData& Other) const
	{
		return
			PrivateStart == Other.PrivateStart &&
			PrivateStep == Other.PrivateStep &&
			PrivateSize == Other.PrivateSize;
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct VOXELMETAGRAPH_API FVoxelSparsePositionQueryData : public FVoxelPositionQueryData
{
	GENERATED_BODY()
	GENERATED_VOXEL_QUERY_DATA_BODY()
		
public:
	virtual FVoxelVectorBuffer GetPositions() const override
	{
		return PrivatePositions;
	}

	void Initialize(const FVoxelVectorBuffer& Positions);
	
private:
	FVoxelVectorBuffer PrivatePositions;
	
	uint64 GetHash() const
	{
		return PrivatePositions.GetHash();
	}
	bool Identical(const FVoxelSparsePositionQueryData& Other) const
	{
		return PrivatePositions.Identical(Other.PrivatePositions);
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(Category = "Misc")
struct VOXELMETAGRAPH_API FVoxelNode_GetPosition3D : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_OUTPUT_PIN(FVoxelVectorBuffer, Position);

	virtual bool IsPureNode() const override
	{
		return true;
	}
};

USTRUCT(Category = "Misc")
struct VOXELMETAGRAPH_API FVoxelNode_GetPosition2D : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_OUTPUT_PIN(FVoxelVector2DBuffer, Position);

	virtual bool IsPureNode() const override
	{
		return true;
	}
};

USTRUCT(Category = "Misc")
struct VOXELMETAGRAPH_API FVoxelNode_QueryWithGradientStep : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_GENERIC_INPUT_PIN(Data);
	VOXEL_INPUT_PIN(float, Step, nullptr);
	VOXEL_GENERIC_OUTPUT_PIN(OutData);

	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;
};

USTRUCT(Category = "Misc")
struct VOXELMETAGRAPH_API FVoxelNode_QueryWithPosition : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_GENERIC_INPUT_PIN(Data);
	VOXEL_INPUT_PIN(FVoxelVectorBuffer, Position, nullptr);
	VOXEL_GENERIC_OUTPUT_PIN(OutData);

	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;
};

USTRUCT(Category = "Misc")
struct VOXELMETAGRAPH_API FVoxelNode_Query2D : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_GENERIC_INPUT_PIN(Data);
	VOXEL_GENERIC_OUTPUT_PIN(OutData);

	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;
};