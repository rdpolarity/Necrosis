// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

BEGIN_VOXEL_NAMESPACE(PointCanvas)

constexpr int32 ChunkSize = 8;
constexpr int32 ChunkSizeLog2 = FVoxelUtilities::ExactLog2<ChunkSize>();
constexpr int32 ChunkCount = FMath::Cube(ChunkSize);
constexpr int32 SelfChunkIndex = 13;

struct FIndex
{
	static constexpr int32 NumPoints = 1024;

	union
	{
		uint16 Raw;
		struct
		{
			uint16 bIsValid : 1;
			uint16 ChunkIndex : FVoxelUtilities::CeilLog2<27>();
			uint16 PointIndex : FVoxelUtilities::CeilLog2<NumPoints>();
		};
	};

	FORCEINLINE FIndex()
	{
		Raw = 0;
	}

	FORCEINLINE static FIndex Make(const int32 ChunkIndex, const int32 PointIndex)
	{
		checkVoxelSlow(0 <= ChunkIndex && ChunkIndex < 27);
		checkVoxelSlow(0 <= PointIndex && PointIndex < NumPoints);

		FIndex Index;
		Index.bIsValid = true;
		Index.ChunkIndex = ChunkIndex;
		Index.PointIndex = PointIndex;
		return Index;
	}
};
checkStatic(sizeof(FIndex) == sizeof(uint16));

struct FPoint
{
	union
	{
		uint64 Raw;
		struct
		{
			uint16 X : ChunkSizeLog2;
			uint16 Y : ChunkSizeLog2;
			uint16 Z : ChunkSizeLog2;
			uint16 Direction : 2;
			uint16 Padding0 : 5;
			uint16 Alpha;
			uint8 NormalX;
			uint8 NormalY;
			uint8 NormalZ;
			uint8 Padding1;
		};
	};

	FORCEINLINE FPoint()
	{
		Raw = 0;
	}

	FORCEINLINE static FPoint Make(
		const FIntVector& Position,
		const int32 Direction,
		const float Alpha,
		const FVector3f& Normal)
	{
		checkVoxelSlow(0 <= Position.X && Position.X < ChunkSize);
		checkVoxelSlow(0 <= Position.Y && Position.Y < ChunkSize);
		checkVoxelSlow(0 <= Position.Z && Position.Z < ChunkSize);
		checkVoxelSlow(0 <= Direction && Direction < 3);

		FPoint Point;
		Point.X = Position.X;
		Point.Y = Position.Y;
		Point.Z = Position.Z;
		Point.Direction = Direction;
		Point.Alpha = FVoxelUtilities::FloatToUINT16(Alpha);
		Point.NormalX = FVoxelUtilities::FloatToUINT8((Normal.X + 1.f) / 2.f);
		Point.NormalY = FVoxelUtilities::FloatToUINT8((Normal.Y + 1.f) / 2.f);
		Point.NormalZ = FVoxelUtilities::FloatToUINT8((Normal.Z + 1.f) / 2.f);
		return Point;
	}

	FORCEINLINE FVector3f GetNormal() const
	{
		return FVector3f(
			2.f * FVoxelUtilities::UINT8ToFloat(NormalX) - 1.f,
			2.f * FVoxelUtilities::UINT8ToFloat(NormalY) - 1.f,
			2.f * FVoxelUtilities::UINT8ToFloat(NormalZ) - 1.f);
	}
	FORCEINLINE float GetSign(const FVector3f& ThisPosition, const FVector3f& OtherPosition) const
	{
		return FVector3f::DotProduct(GetNormal(), OtherPosition - ThisPosition) < 0 ? -1.f : 1.f;
	}
	FORCEINLINE FVector3f GetPosition() const
	{
		FVector3f Position(X, Y, Z);
		Position[Direction] += FVoxelUtilities::UINT16ToFloat(Alpha);
		return Position;
	}
};
checkStatic(sizeof(FPoint) == sizeof(uint64));

// TODO
// UberChunks, chunk size = 8 with closest chunks stored

DECLARE_VOXEL_MEMORY_STAT(VOXELCANVAS_API, STAT_ChunkMemory, "Voxel Smooth Canvas Chunk Memory");

namespace EChunkState
{
	enum Type : uint8
	{
		Uninitialized,
		JumpFlooded_Self,
		JumpFlooded_Faces,
		JumpFlooded_Edges,
		JumpFlooded_Corners,
	};
}

struct VOXELCANVAS_API FChunk
{
public:
	const FIntVector ChunkKey;
	TVoxelStaticArray<FChunk*, 27> Chunks{ ForceInit };
	TVoxelArray<FPoint> Points;

	using FIndices = TVoxelStaticArray<FIndex, ChunkCount>;

	explicit FChunk(const FIntVector& ChunkKey)
		: ChunkKey(ChunkKey)
	{
	}
	~FChunk()
	{
		checkVoxelSlow(!Chunks[SelfChunkIndex] || Chunks[SelfChunkIndex] == this);
	}
	UE_NONCOPYABLE(FChunk);

	FORCEINLINE bool IsReady() const
	{
		return
			State == EChunkState::JumpFlooded_Corners &&
			ensure(Indices_Corners);
	}
	FORCEINLINE const FIndices& GetIndices() const
	{
		checkVoxelSlow(IsReady());
		return *Indices_Corners.Get();
	}
	
public:
	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_ChunkMemory);

	int64 GetAllocatedSize() const;

public:
	template<typename LambdaType>
	FORCEINLINE void ForeachNeighbor(const EChunkState::Type InState, LambdaType&& Lambda)
	{
		switch (InState)
		{
		default: ensure(false);
		case EChunkState::JumpFlooded_Self: Lambda(SelfChunkIndex, *this); break;
		case EChunkState::JumpFlooded_Faces: this->ForeachNeighbor_Faces(Lambda); break;
		case EChunkState::JumpFlooded_Edges: this->ForeachNeighbor_Edges(Lambda); break;
		case EChunkState::JumpFlooded_Corners: this->ForeachNeighbor_Corners(Lambda); break;
		}
	}

#define Case(X, Y, Z) if (Chunks[X + Y * 3 + Z * 9]) { Lambda(X + Y * 3 + Z * 9, *Chunks[X + Y * 3 + Z * 9]); }

	template<typename LambdaType>
	FORCEINLINE void ForeachNeighbor_Faces(LambdaType&& Lambda) const
	{
		Case(0, 1, 1);
		Case(2, 1, 1);
		Case(1, 0, 1);
		Case(1, 2, 1);
		Case(1, 1, 0);
		Case(1, 1, 2);
	}

	template<typename LambdaType>
	FORCEINLINE void ForeachNeighbor_Edges(LambdaType&& Lambda) const
	{
		Case(0, 0, 1);
		Case(0, 2, 1);
		Case(0, 1, 0);
		Case(0, 1, 2);
		
		Case(2, 0, 1);
		Case(2, 2, 1);
		Case(2, 1, 0);
		Case(2, 1, 2);

		Case(1, 0, 0);
		Case(1, 2, 0);
		Case(1, 0, 2);
		Case(1, 2, 2);
	}

	template<typename LambdaType>
	FORCEINLINE void ForeachNeighbor_Corners(LambdaType&& Lambda) const
	{
		Case(0, 0, 0);
		Case(2, 0, 0);
		Case(0, 2, 0);
		Case(2, 2, 0);
		Case(0, 0, 2);
		Case(2, 0, 2);
		Case(0, 2, 2);
		Case(2, 2, 2);
	}

#undef Case

public:
	FORCEINLINE void Invalidate(const EChunkState::Type NewState)
	{
		if (NewState < State)
		{
			InvalidateImpl(NewState);
		}
	}
	FORCEINLINE void JumpFlood(const EChunkState::Type NewState)
	{
		if (State < NewState)
		{
			JumpFloodImpl(NewState);
		}
	}

	void Finalize();

private:
	EChunkState::Type State = EChunkState::Uninitialized;

	TSharedPtr<const FIndices> Indices_Self;
	TSharedPtr<const FIndices> Indices_Faces;
	TSharedPtr<const FIndices> Indices_Edges;
	TSharedPtr<const FIndices> Indices_Corners;

	struct FAllPoints
	{
		TVoxelStaticArray<TVoxelArray<FVector3f>, 27> ChunksPoints;

		FORCEINLINE const FVector3f& operator[](const FIndex& Index) const
		{
			return ChunksPoints[Index.ChunkIndex][Index.PointIndex];
		}
	};

	TSharedPtr<const FIndices>& GetIndices(const EChunkState::Type InState)
	{
		switch (InState)
		{
		default: ensure(false);
		case EChunkState::JumpFlooded_Self: return Indices_Self;
		case EChunkState::JumpFlooded_Faces: return Indices_Faces;
		case EChunkState::JumpFlooded_Edges: return Indices_Edges;
		case EChunkState::JumpFlooded_Corners: return Indices_Corners;
		}
	}
	
	void JumpFloodImpl(EChunkState::Type NewState);
	void InvalidateImpl(EChunkState::Type NewState);

	template<int32 Step>
	void JumpFloodPass(
		const FAllPoints& AllPoints,
		const TVoxelStaticArray<FIndex, ChunkCount>& InIndices,
		TVoxelStaticArray<FIndex, ChunkCount>& OutIndices);
};

class VOXELCANVAS_API FLODData
{
public:
	mutable FVoxelSharedCriticalSection CriticalSection;

	FThreadSafeCounter RenderCounter;
	TVoxelArray<FVector3f> LastVertices;

	FChunk& FindOrAddChunk(const FIntVector& ChunkKey)
	{
		checkVoxelSlow(CriticalSection.IsLocked_Write_Debug());
		TSharedPtr<FChunk>& Chunk = Chunks.FindOrAdd(ChunkKey);
		if (!Chunk)
		{
			Chunk = MakeShared<FChunk>(ChunkKey);
			RecomputeNeighbors(ChunkKey);
		}
		return *Chunk;
	}
	FORCEINLINE FChunk* FindChunk(const FIntVector& ChunkKey)
	{
		checkVoxelSlow(CriticalSection.IsLocked_Read_Debug());
		const TSharedPtr<FChunk>* ChunkPtr = Chunks.Find(ChunkKey);
		if (!ChunkPtr)
		{
			return nullptr;
		}

		return ChunkPtr->Get();
	}
	FORCEINLINE const FChunk* FindChunk(const FIntVector& ChunkKey) const
	{
		return VOXEL_CONST_CAST(this)->FindChunk(ChunkKey);
	}

public:
	void Serialize(FArchive& Ar);

	FVoxelIntBox GetBounds() const;
	TVoxelArray<FVector4f> GetAllPoints() const;
	TVoxelArray<FVoxelIntBox> GetChunkBounds() const;

	void AddPoints(const TVoxelArray<FVector3f>& Vertices);
	void JumpFloodChunks();
	void RecomputeNeighbors(const FIntVector& ChunkKey);
	void EditPoints(const TFunctionRef<bool(FVoxelIntBox)> ShouldVisitChunk, const TFunctionRef<void(TVoxelArrayView<FVector3f>)> EditPoints);

private:
	TVoxelIntVectorMap<TSharedPtr<FChunk>> Chunks;

	void GenerateTriangles(const FIntVector& Start, TVoxelArray<FVector3f>& OutVertices) const;
};

struct VOXELCANVAS_API FUtilities
{
	FORCEINLINE static bool FindClosestPosition(
		float& OutDistance,
		const FLODData& Data,
		const FVector3f& Position,
		const FChunk*& Chunk,
		FIntVector& LastChunkKey)
	{
		VOXEL_USE_NAMESPACE(PointCanvas);

		const FIntVector PositionToQuery = FVoxelUtilities::RoundToInt(Position);
		const FIntVector ChunkKey = FVoxelUtilities::DivideFloor_FastLog2(PositionToQuery, ChunkSizeLog2);

		if (ChunkKey != LastChunkKey)
		{
			Chunk = Data.FindChunk(ChunkKey);
			LastChunkKey = ChunkKey;
		}

		if (!Chunk || !ensure(Chunk->IsReady()))
		{
			return false;
		}

		const FIntVector LocalPosition = PositionToQuery - ChunkKey * ChunkSize;
		const FIndex PointIndex = Chunk->GetIndices()[FVoxelUtilities::Get3DIndex<int32>(ChunkSize, LocalPosition)];

		if (!ensureVoxelSlow(PointIndex.bIsValid))
		{
			return false;
		}

		const FChunk* NeighborChunk = Chunk->Chunks[PointIndex.ChunkIndex];
		checkVoxelSlow(NeighborChunk);
		const FPoint Point = NeighborChunk->Points[PointIndex.PointIndex];

		const FVector3f PointPosition = FVector3f(NeighborChunk->ChunkKey * ChunkSize) + Point.GetPosition();
		const float Distance = FVector3f::Distance(Position, PointPosition);

		OutDistance = Distance * Point.GetSign(PointPosition, Position);
		return true;
	}
};

END_VOXEL_NAMESPACE(PointCanvas)

struct VOXELCANVAS_API FVoxelPointCanvasData
{
	VOXEL_USE_NAMESPACE_TYPES(PointCanvas, FLODData);

	TVoxelArray<TSharedPtr<FLODData>> LODs;
	
	FLODData& GetFirstLOD()
	{
		return *LODs[0];
	}
	const FLODData& GetFirstLOD() const
	{
		return *LODs[0];
	}

	void Serialize(FArchive& Ar);
};