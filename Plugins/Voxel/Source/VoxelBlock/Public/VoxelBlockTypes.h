// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBlockTypes.generated.h"

UENUM(BlueprintType)
enum class EVoxelBlockFace : uint8
{
	// -X
	Back,
	// +X
	Front,
	// -Y
	Left,
	// +Y
	Right,
	// -Z
	Bottom,
	// +Z
	Top,

#if CPP
	XMin = Back,
	XMax = Front,
	YMin = Left,
	YMax = Right,
	ZMin = Bottom,
	ZMax = Top,
#endif
};
ENUM_RANGE_BY_COUNT(EVoxelBlockFace, 6);

struct FVoxelBlockFaceIds
{
	int32 Top = 0;
	int32 Bottom = 0;
	int32 Front = 0;
	int32 Back = 0;
	int32 Left = 0;
	int32 Right = 0;
	
	int32& GetId(EVoxelBlockFace Face)
	{
		switch (Face)
		{
		default: ensure(false);
		case EVoxelBlockFace::Back: return Back;
		case EVoxelBlockFace::Front: return Front;
		case EVoxelBlockFace::Left: return Left;
		case EVoxelBlockFace::Right: return Right;
		case EVoxelBlockFace::Bottom: return Bottom;
		case EVoxelBlockFace::Top: return Top;
		}
	}
	const int32& GetId(EVoxelBlockFace Face) const
	{
		return VOXEL_CONST_CAST(this)->GetId(Face);
	}
};

struct FVoxelBlockId
{
public:
	FVoxelBlockId() = default;
	FORCEINLINE explicit FVoxelBlockId(int32 Id)
		: Id(Id)
	{
		checkVoxelSlow(Id >= 0);
	}

	FORCEINLINE int32 GetIndex() const
	{
		return Id;
	}

private:
	int32 Id = 0;

public:
	FORCEINLINE bool operator==(const FVoxelBlockId& Other) const
	{
		return Id == Other.Id;
	}
	FORCEINLINE bool operator!=(const FVoxelBlockId& Other) const
	{
		return Id != Other.Id;
	}
	
public:
	FORCEINLINE friend uint32 GetTypeHash(const FVoxelBlockId& BlockId)
	{
		return BlockId.Id;
	}
};

// The 24 rotations that can be applied with 90 degrees angles
// See https://gamedev.stackexchange.com/a/19264
enum class EVoxelBlockRotation : uint8
{
	PosX_PosY_PosZ,
	PosX_NegY_NegZ,
	PosX_PosZ_NegY,
	PosX_NegZ_PosY,

	NegX_PosY_NegZ,
	NegX_NegY_PosZ,
	NegX_PosZ_PosY,
	NegX_NegZ_NegY,

	PosY_PosX_NegZ,
	PosY_NegX_PosZ,
	PosY_PosZ_PosX,
	PosY_NegZ_NegX,

	NegY_PosX_PosZ,
	NegY_NegX_NegZ,
	NegY_PosZ_NegX,
	NegY_NegZ_PosX,

	PosZ_PosX_PosY,
	PosZ_NegX_NegY,
	PosZ_PosY_NegX,
	PosZ_NegY_PosX,

	NegZ_PosX_NegY,
	NegZ_NegX_PosY,
	NegZ_PosY_PosX,
	NegZ_NegY_NegX,

	Invalid
};

struct VOXELBLOCK_API FVoxelBlockRotation
{
	static EVoxelBlockRotation FromRotator(FRotator Rotation);
	static FVoxelIntBox RotateBounds(const FVoxelIntBox& Bounds, EVoxelBlockRotation Rotation);
	
	FORCEINLINE static FRotator ToRotator(EVoxelBlockRotation Rotation)
	{
		switch (Rotation)
		{
		default: VOXEL_ASSUME(false);
		case EVoxelBlockRotation::PosX_PosY_PosZ:  return FRotator(-180, -180, -180);
		case EVoxelBlockRotation::PosX_NegY_NegZ:  return FRotator(-180, -180, 0);
		case EVoxelBlockRotation::PosX_PosZ_NegY:  return FRotator(-180, -180, -90);
		case EVoxelBlockRotation::PosX_NegZ_PosY:  return FRotator(-180, -180, 90);

		case EVoxelBlockRotation::NegX_PosY_NegZ:  return FRotator(-180, 0, 0);
		case EVoxelBlockRotation::NegX_NegY_PosZ:  return FRotator(-180, 0, -180);
		case EVoxelBlockRotation::NegX_PosZ_PosY:  return FRotator(-180, 0, 90);
		case EVoxelBlockRotation::NegX_NegZ_NegY:  return FRotator(-180, 0, -90);

		case EVoxelBlockRotation::PosY_PosX_NegZ:  return FRotator(-180, -90, 0);
		case EVoxelBlockRotation::PosY_NegX_PosZ:  return FRotator(-180, 90, -180);
		case EVoxelBlockRotation::PosY_PosZ_PosX:  return FRotator(90, -180, -90);
		case EVoxelBlockRotation::PosY_NegZ_NegX:  return FRotator(-90, -180, 90);

		case EVoxelBlockRotation::NegY_PosX_PosZ:  return FRotator(-180, -90, -180);
		case EVoxelBlockRotation::NegY_NegX_NegZ:  return FRotator(-180, 90, 0);
		case EVoxelBlockRotation::NegY_PosZ_NegX:  return FRotator(-90, -180, -90);
		case EVoxelBlockRotation::NegY_NegZ_PosX:  return FRotator(90, -180, 90);

		case EVoxelBlockRotation::PosZ_PosX_PosY:  return FRotator(-180, -90, 90);
		case EVoxelBlockRotation::PosZ_NegX_NegY:  return FRotator(-180, 90, -90);
		case EVoxelBlockRotation::PosZ_PosY_NegX:  return FRotator(-90, -180, -180);
		case EVoxelBlockRotation::PosZ_NegY_PosX:  return FRotator(90, -180, 0);

		case EVoxelBlockRotation::NegZ_PosX_NegY:  return FRotator(-180, -90, -90);
		case EVoxelBlockRotation::NegZ_NegX_PosY:  return FRotator(-180, 90, 90);
		case EVoxelBlockRotation::NegZ_PosY_PosX:  return FRotator(90, -180, -180);
		case EVoxelBlockRotation::NegZ_NegY_NegX:  return FRotator(-90, -180, 0);
		}
	}

	// This is compiled to a jump and ~3 instructions per case - much faster than a regular transform
	template<typename VectorType>
	FORCEINLINE static VectorType Rotate(const VectorType& Position, EVoxelBlockRotation Rotation)
	{
		const auto X = Position.X;
		const auto Y = Position.Y;
		const auto Z = Position.Z;
		switch (Rotation)
		{
		default: VOXEL_ASSUME(false);
		case EVoxelBlockRotation::PosX_PosY_PosZ: return VectorType(+X, +Y, +Z);
		case EVoxelBlockRotation::PosX_NegY_NegZ: return VectorType(+X, -Y, -Z);
		case EVoxelBlockRotation::PosX_PosZ_NegY: return VectorType(+X, +Z, -Y);
		case EVoxelBlockRotation::PosX_NegZ_PosY: return VectorType(+X, -Z, +Y);

		case EVoxelBlockRotation::NegX_PosY_NegZ: return VectorType(-X, +Y, -Z);
		case EVoxelBlockRotation::NegX_NegY_PosZ: return VectorType(-X, -Y, +Z);
		case EVoxelBlockRotation::NegX_PosZ_PosY: return VectorType(-X, +Z, +Y);
		case EVoxelBlockRotation::NegX_NegZ_NegY: return VectorType(-X, -Z, -Y);

		case EVoxelBlockRotation::PosY_PosX_NegZ: return VectorType(+Y, +X, -Z);
		case EVoxelBlockRotation::PosY_NegX_PosZ: return VectorType(+Y, -X, +Z);
		case EVoxelBlockRotation::PosY_PosZ_PosX: return VectorType(+Y, +Z, +X);
		case EVoxelBlockRotation::PosY_NegZ_NegX: return VectorType(+Y, -Z, -X);

		case EVoxelBlockRotation::NegY_PosX_PosZ: return VectorType(-Y, +X, +Z);
		case EVoxelBlockRotation::NegY_NegX_NegZ: return VectorType(-Y, -X, -Z);
		case EVoxelBlockRotation::NegY_PosZ_NegX: return VectorType(-Y, +Z, -X);
		case EVoxelBlockRotation::NegY_NegZ_PosX: return VectorType(-Y, -Z, +X);

		case EVoxelBlockRotation::PosZ_PosX_PosY: return VectorType(+Z, +X, +Y);
		case EVoxelBlockRotation::PosZ_NegX_NegY: return VectorType(+Z, -X, -Y);
		case EVoxelBlockRotation::PosZ_PosY_NegX: return VectorType(+Z, +Y, -X);
		case EVoxelBlockRotation::PosZ_NegY_PosX: return VectorType(+Z, -Y, +X);

		case EVoxelBlockRotation::NegZ_PosX_NegY: return VectorType(-Z, +X, -Y);
		case EVoxelBlockRotation::NegZ_NegX_PosY: return VectorType(-Z, -X, +Y);
		case EVoxelBlockRotation::NegZ_PosY_PosX: return VectorType(-Z, +Y, +X);
		case EVoxelBlockRotation::NegZ_NegY_NegX: return VectorType(-Z, -Y, -X);
		}
	}

	FORCEINLINE static EVoxelBlockRotation Invert(EVoxelBlockRotation Rotation)
	{
		switch (Rotation)
		{
		default: VOXEL_ASSUME(false);
		case EVoxelBlockRotation::PosX_PosY_PosZ: return EVoxelBlockRotation::PosX_PosY_PosZ;
		case EVoxelBlockRotation::PosX_NegY_NegZ: return EVoxelBlockRotation::PosX_NegY_NegZ;
		case EVoxelBlockRotation::PosX_PosZ_NegY: return EVoxelBlockRotation::PosX_NegZ_PosY;
		case EVoxelBlockRotation::PosX_NegZ_PosY: return EVoxelBlockRotation::PosX_PosZ_NegY;

		case EVoxelBlockRotation::NegX_PosY_NegZ: return EVoxelBlockRotation::NegX_PosY_NegZ;
		case EVoxelBlockRotation::NegX_NegY_PosZ: return EVoxelBlockRotation::NegX_NegY_PosZ;
		case EVoxelBlockRotation::NegX_PosZ_PosY: return EVoxelBlockRotation::NegX_PosZ_PosY;
		case EVoxelBlockRotation::NegX_NegZ_NegY: return EVoxelBlockRotation::NegX_NegZ_NegY;
		
		case EVoxelBlockRotation::PosY_PosX_NegZ: return EVoxelBlockRotation::PosY_PosX_NegZ;
		case EVoxelBlockRotation::PosY_NegX_PosZ: return EVoxelBlockRotation::NegY_PosX_PosZ;
		case EVoxelBlockRotation::PosY_PosZ_PosX: return EVoxelBlockRotation::PosZ_PosX_PosY;
		case EVoxelBlockRotation::PosY_NegZ_NegX: return EVoxelBlockRotation::NegZ_PosX_NegY;
		
		case EVoxelBlockRotation::NegY_PosX_PosZ: return EVoxelBlockRotation::PosY_NegX_PosZ;
		case EVoxelBlockRotation::NegY_NegX_NegZ: return EVoxelBlockRotation::NegY_NegX_NegZ;
		case EVoxelBlockRotation::NegY_PosZ_NegX: return EVoxelBlockRotation::NegZ_NegX_PosY;
		case EVoxelBlockRotation::NegY_NegZ_PosX: return EVoxelBlockRotation::PosZ_NegX_NegY;
		
		case EVoxelBlockRotation::PosZ_PosX_PosY: return EVoxelBlockRotation::PosY_PosZ_PosX;
		case EVoxelBlockRotation::PosZ_NegX_NegY: return EVoxelBlockRotation::NegY_NegZ_PosX;
		case EVoxelBlockRotation::PosZ_PosY_NegX: return EVoxelBlockRotation::NegZ_PosY_PosX;
		case EVoxelBlockRotation::PosZ_NegY_PosX: return EVoxelBlockRotation::PosZ_NegY_PosX;
		
		case EVoxelBlockRotation::NegZ_PosX_NegY: return EVoxelBlockRotation::PosY_NegZ_NegX;
		case EVoxelBlockRotation::NegZ_NegX_PosY: return EVoxelBlockRotation::NegY_PosZ_NegX;
		case EVoxelBlockRotation::NegZ_PosY_PosX: return EVoxelBlockRotation::PosZ_PosY_NegX;
		case EVoxelBlockRotation::NegZ_NegY_NegX: return EVoxelBlockRotation::NegZ_NegY_NegX;
		}
	}

public:
	FORCEINLINE static FIntVector CubicFaceToVector(EVoxelBlockFace Face)
	{
		switch (Face)
		{
		default: VOXEL_ASSUME(false);
		case EVoxelBlockFace::Back:   return FIntVector(-1, 0, 0);
		case EVoxelBlockFace::Front:  return FIntVector(+1, 0, 0);
		case EVoxelBlockFace::Left:   return FIntVector(0, -1, 0);
		case EVoxelBlockFace::Right:  return FIntVector(0, +1, 0);
		case EVoxelBlockFace::Bottom: return FIntVector(0, 0, -1);
		case EVoxelBlockFace::Top:    return FIntVector(0, 0, +1);
		}
	}
	FORCEINLINE static EVoxelBlockFace VectorToCubicFace(FIntVector Vector)
	{
		checkVoxelSlow(FMath::Abs(Vector.X) + FMath::Abs(Vector.Y) + FMath::Abs(Vector.Z) == 1);

#define INDEX(X, Y, Z) (X + 1) + ((Y + 1) << 2) + ((Z + 1) << 4)

		const int32 Index = INDEX(Vector.X, Vector.Y, Vector.Z);
		switch (Index)
		{
		default: VOXEL_ASSUME(false);
		case INDEX(-1, 0, 0): return EVoxelBlockFace::Back;
		case INDEX(+1, 0, 0): return EVoxelBlockFace::Front;
		case INDEX(0, -1, 0): return EVoxelBlockFace::Left;
		case INDEX(0, +1, 0): return EVoxelBlockFace::Right;
		case INDEX(0, 0, -1): return EVoxelBlockFace::Bottom;
		case INDEX(0, 0, +1): return EVoxelBlockFace::Top;
		}

#undef INDEX
	}
	
	FORCEINLINE static EVoxelBlockFace Rotate(EVoxelBlockFace Face, EVoxelBlockRotation Rotation)
	{
		return VectorToCubicFace(Rotate(CubicFaceToVector(Face), Rotation));
	}
};

// TODO REMOVE
constexpr int32 BlockChunkSize = 32;

USTRUCT(DisplayName = "Block")
struct FVoxelBlockData
{
	GENERATED_BODY()

public:
	static constexpr const TCHAR* ChannelName = TEXT("Block");

	FORCEINLINE FVoxelBlockData()
	{
		Word = 0;
	}

	FORCEINLINE FVoxelBlockData(
		FVoxelBlockId Id,
		EVoxelBlockRotation Rotation,
		bool bIsAir,
		bool bIsMasked,
		bool bAlwaysRenderInnerFaces,
		bool bIsTwoSided)
		: Id(Id.GetIndex())
		, Rotation(uint32(Rotation))
		, bIsAir(bIsAir)
		, bIsMasked(bIsMasked)
		, bAlwaysRenderInnerFaces(bAlwaysRenderInnerFaces)
		, bIsTwoSided(bIsTwoSided)
	{
		checkVoxelSlow(GetId().GetIndex() == Id.GetIndex());
	}

public:
	FORCEINLINE FVoxelBlockId GetId() const
	{
		return FVoxelBlockId(Id);
	}
	FORCEINLINE EVoxelBlockRotation GetRotation() const
	{
		return EVoxelBlockRotation(Rotation);
	}
	FORCEINLINE bool IsAir() const
	{
		return bIsAir;
	}
	FORCEINLINE bool IsMasked() const
	{
		return bIsMasked;
	}
	FORCEINLINE bool AlwaysRenderInnerFaces() const
	{
		return bAlwaysRenderInnerFaces;
	}
	FORCEINLINE bool IsTwoSided() const
	{
		return bIsTwoSided;
	}

public:
	FORCEINLINE void SetRotation(EVoxelBlockRotation NewRotation)
	{
		Rotation = uint32(NewRotation);
	}

public:
	FORCEINLINE bool operator==(const FVoxelBlockData& Other) const
	{
		return Word == Other.Word;
	}
	FORCEINLINE bool operator!=(const FVoxelBlockData& Other) const
	{
		return Word != Other.Word;
	}

private:
	union
	{
		struct
		{
			uint32 Id : 23;
			uint32 Rotation : 5;
			uint32 bIsAir : 1;
			uint32 bIsMasked : 1;
			uint32 bAlwaysRenderInnerFaces : 1;
			uint32 bIsTwoSided : 1;
		};
		uint32 Word;
	};
};
checkStatic(sizeof(FVoxelBlockData) == 4);