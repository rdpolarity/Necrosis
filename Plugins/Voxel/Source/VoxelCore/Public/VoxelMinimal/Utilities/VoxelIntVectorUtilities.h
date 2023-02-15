// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "VoxelMinimal/Containers/VoxelStaticArray.h"
#include "VoxelMinimal/Utilities/VoxelBaseUtilities.h"

namespace FVoxelUtilities
{
	FORCEINLINE bool CountIs32Bits(const FIntVector& Size)
	{
		return FMath::Abs(int64(Size.X) * int64(Size.Y) * int64(Size.Z)) < MAX_int32;
	}

	// Defaults to the "lowest" axis if equal (will return X if X and Y are equal)
	template<typename TVector>
	FORCEINLINE int32 GetArgMin(const TVector& V)
	{
		if (V.X <= V.Y && V.X <= V.Z)
		{
			return 0;
		}
		else if (V.Y <= V.Z)
		{
			return 1;
		}
		else
		{
			return 2;
		}
	}
	// Defaults to the "lowest" axis if equal (will return X if X and Y are equal)
	template<typename TVector>
	FORCEINLINE int32 GetArgMax(const TVector& V)
	{
		if (V.X >= V.Y && V.X >= V.Z)
		{
			return 0;
		}
		else if (V.Y >= V.Z)
		{
			return 1;
		}
		else
		{
			return 2;
		}
	}

	FORCEINLINE FIntVector PositiveMod(const FIntVector& V, int32 Divisor)
	{
		return FIntVector(
			PositiveMod(V.X, Divisor),
			PositiveMod(V.Y, Divisor),
			PositiveMod(V.Z, Divisor));
	}

	FORCEINLINE FIntVector DivideFloor(const FIntVector& V, int32 Divisor)
	{
		return FIntVector(
			DivideFloor(V.X, Divisor),
			DivideFloor(V.Y, Divisor),
			DivideFloor(V.Z, Divisor));
	}
	FORCEINLINE FIntVector DivideFloor_Positive(const FIntVector& V, int32 Divisor)
	{
		return FIntVector(
			DivideFloor_Positive(V.X, Divisor),
			DivideFloor_Positive(V.Y, Divisor),
			DivideFloor_Positive(V.Z, Divisor));
	}
	FORCEINLINE FIntVector DivideFloor_FastLog2(const FIntVector& V, int32 DivisorLog2)
	{
		return FIntVector(
			DivideFloor_FastLog2(V.X, DivisorLog2),
			DivideFloor_FastLog2(V.Y, DivisorLog2),
			DivideFloor_FastLog2(V.Z, DivisorLog2));
	}
	FORCEINLINE FIntVector DivideCeil(const FIntVector& V, int32 Divisor)
	{
		return FIntVector(
			DivideCeil(V.X, Divisor),
			DivideCeil(V.Y, Divisor),
			DivideCeil(V.Z, Divisor));
	}
	FORCEINLINE FIntVector DivideRound(const FIntVector& V, int32 Divisor)
	{
		return FIntVector(
			DivideRound(V.X, Divisor),
			DivideRound(V.Y, Divisor),
			DivideRound(V.Z, Divisor));
	}
	
	FORCEINLINE FIntPoint DivideFloor(const FIntPoint& V, int32 Divisor)
	{
		return FIntPoint(
			DivideFloor(V.X, Divisor),
			DivideFloor(V.Y, Divisor));
	}
	FORCEINLINE FIntPoint DivideCeil(const FIntPoint& V, int32 Divisor)
	{
		return FIntPoint(
			DivideCeil(V.X, Divisor),
			DivideCeil(V.Y, Divisor));
	}
	FORCEINLINE FIntPoint DivideRound(const FIntPoint& V, int32 Divisor)
	{
		return FIntPoint(
			DivideRound(V.X, Divisor),
			DivideRound(V.Y, Divisor));
	}
	
	FORCEINLINE uint64 SquaredSize(const FIntPoint& V)
	{
		return FMath::Square<int64>(V.X) + FMath::Square<int64>(V.Y);
	}
	FORCEINLINE double Size(const FIntPoint& V)
	{
		return FMath::Sqrt(double(SquaredSize(V)));
	}
	
	FORCEINLINE uint64 SquaredSize(const FIntVector& V)
	{
		return FMath::Square<int64>(V.X) + FMath::Square<int64>(V.Y) + FMath::Square<int64>(V.Z);
	}
	FORCEINLINE double Size(const FIntVector& V)
	{
		return FMath::Sqrt(double(SquaredSize(V)));
	}
	
	FORCEINLINE bool IsMultipleOfPowerOfTwo(const FIntVector& Vector, int32 PowerOfTwo)
	{
		return
			IsMultipleOfPowerOfTwo(Vector.X, PowerOfTwo) && 
			IsMultipleOfPowerOfTwo(Vector.Y, PowerOfTwo) && 
			IsMultipleOfPowerOfTwo(Vector.Z, PowerOfTwo);
	}

	FORCEINLINE TVoxelStaticArray<FIntVector, 8> GetNeighbors(const FVector& P)
	{
		const int32 MinX = FMath::FloorToInt(P.X);
		const int32 MinY = FMath::FloorToInt(P.Y);
		const int32 MinZ = FMath::FloorToInt(P.Z);

		const int32 MaxX = FMath::CeilToInt(P.X);
		const int32 MaxY = FMath::CeilToInt(P.Y);
		const int32 MaxZ = FMath::CeilToInt(P.Z);

		return {
		FIntVector(MinX, MinY, MinZ),
		FIntVector(MaxX, MinY, MinZ),
		FIntVector(MinX, MaxY, MinZ),
		FIntVector(MaxX, MaxY, MinZ),
		FIntVector(MinX, MinY, MaxZ),
		FIntVector(MaxX, MinY, MaxZ),
		FIntVector(MinX, MaxY, MaxZ),
		FIntVector(MaxX, MaxY, MaxZ)
		};
	}
	FORCEINLINE TVoxelStaticArray<FIntPoint, 4> GetNeighbors(float X, float Y)
	{
		const int32 MinX = FMath::FloorToInt(X);
		const int32 MinY = FMath::FloorToInt(Y);

		const int32 MaxX = FMath::CeilToInt(X);
		const int32 MaxY = FMath::CeilToInt(Y);

		return {
		FIntPoint(MinX, MinY),
		FIntPoint(MaxX, MinY),
		FIntPoint(MinX, MaxY),
		FIntPoint(MaxX, MaxY)
		};
	}
	
	inline TArray<FIntVector, TFixedAllocator<6>> GetImmediateNeighbors(const FIntVector& V)
	{
		return {
			FIntVector(V.X - 1, V.Y, V.Z),
			FIntVector(V.X + 1, V.Y, V.Z),
			FIntVector(V.X, V.Y - 1, V.Z),
			FIntVector(V.X, V.Y + 1, V.Z),
			FIntVector(V.X, V.Y, V.Z - 1),
			FIntVector(V.X, V.Y, V.Z + 1)
		};
	}
	inline void AddImmediateNeighborsToArray(const FIntVector& V, TArray<FIntVector>& Array)
	{
		const int32& X = V.X;
		const int32& Y = V.Y;
		const int32& Z = V.Z;

		const uint32 Pos = Array.AddUninitialized(6);
		FIntVector* Ptr = Array.GetData() + Pos;

		new (Ptr++) FIntVector(X - 1, Y, Z);
		new (Ptr++) FIntVector(X + 1, Y, Z);

		new (Ptr++) FIntVector(X, Y - 1, Z);
		new (Ptr++) FIntVector(X, Y + 1, Z);

		new (Ptr++) FIntVector(X, Y, Z - 1);
		new (Ptr++) FIntVector(X, Y, Z + 1);

		checkVoxelSlow(Ptr == Array.GetData() + Array.Num());
	}
	
	FORCEINLINE uint64 FastIntVectorHash_DoubleWord(const FIntVector& Key)
	{
		const uint32 X = Key.X;
		const uint32 Y = Key.Y;
		const uint32 Z = Key.Z;

		uint64 Hash = uint64(X) | (uint64(Y) << 32);
		Hash ^= uint64(Z) << 16;

		return MurmurHash64(Hash);
	}
	FORCEINLINE uint32 FastIntVectorHash(const FIntVector& Key)
	{
		return FastIntVectorHash_DoubleWord(Key);
	}
	FORCEINLINE uint32 FastIntVectorHash_Low(const FIntVector& Key)
	{
		return FastIntVectorHash_DoubleWord(Key);
	}
	FORCEINLINE uint32 FastIntVectorHash_High(const FIntVector& Key)
	{
		return FastIntVectorHash_DoubleWord(Key) >> 32;
	}

	FORCEINLINE uint32 FastIntPointHash(const FIntPoint& Key)
	{
		const uint64 Hash = uint64(Key.X) | (uint64(Key.Y) << 32);
		return MurmurHash64(Hash);
	}

}

#if VOXEL_ENGINE_VERSION < 501
template<>
struct TBaseStructure<FIntPoint>
{
	static UScriptStruct* Get()
	{
		static UScriptStruct* Struct = FindObject<UScriptStruct>(ANY_PACKAGE, TEXT("IntPoint"));
		return Struct;
	}
};
template<>
struct TBaseStructure<FIntVector>
{
	static UScriptStruct* Get()
	{
		static UScriptStruct* Struct = FindObject<UScriptStruct>(ANY_PACKAGE, TEXT("IntVector"));
		return Struct;
	}
};
#endif

template <typename ValueType, bool bInAllowDuplicateKeys = false>
struct TVoxelIntVectorMapKeyFuncs : public TDefaultMapKeyFuncs<FIntVector, ValueType, bInAllowDuplicateKeys>
{
	FORCEINLINE static uint32 GetKeyHash(FIntVector Key)
	{
		return FVoxelUtilities::FastIntVectorHash(Key);
	}
};
struct FVoxelIntVectorSetKeyFuncs : DefaultKeyFuncs<FIntVector>
{
	FORCEINLINE static uint32 GetKeyHash(FIntVector Key)
	{
		return FVoxelUtilities::FastIntVectorHash(Key);
	}
};

template <typename ValueType, bool bInAllowDuplicateKeys = false>
struct TVoxelIntPointMapKeyFuncs : public TDefaultMapKeyFuncs<FIntPoint, ValueType, bInAllowDuplicateKeys>
{
	FORCEINLINE static uint32 GetKeyHash(FIntPoint Key)
	{
		return FVoxelUtilities::FastIntPointHash(Key);
	}
};

template<typename ValueType, typename SetAllocator = FDefaultSetAllocator>
using TVoxelIntVectorMap = TMap<FIntVector, ValueType, SetAllocator, TVoxelIntVectorMapKeyFuncs<ValueType>>;

template<typename Allocator = FDefaultSetAllocator>
using TVoxelIntVectorSet = TSet<FIntVector, FVoxelIntVectorSetKeyFuncs, Allocator>;

using FVoxelIntVectorSet = TVoxelIntVectorSet<>;

template<typename ValueType, typename SetAllocator = FDefaultSetAllocator>
using TVoxelIntPointMap = TMap<FIntPoint, ValueType, SetAllocator, TVoxelIntPointMapKeyFuncs<ValueType>>;

template<>
struct TLess<FIntVector>
{
	FORCEINLINE bool operator()(const FIntVector& A, const FIntVector& B) const
	{
		if (A.X != B.X) return A.X < B.X;
		if (A.Y != B.Y) return A.Y < B.Y;
		return A.Z < B.Z;
	}
};

FORCEINLINE FIntVector operator-(const FIntVector& V)
{
	return FIntVector(-V.X, -V.Y, -V.Z);
}

FORCEINLINE FIntVector operator-(const FIntVector& V, int32 I)
{
	return FIntVector(V.X - I, V.Y - I, V.Z - I);
}
FORCEINLINE FIntVector operator-(const FIntVector& V, uint32 I)
{
	return FIntVector(V.X - I, V.Y - I, V.Z - I);
}
FORCEINLINE FIntVector operator-(int32 I, const FIntVector& V)
{
	return FIntVector(I - V.X, I - V.Y, I - V.Z);
}
FORCEINLINE FIntVector operator-(uint32 I, const FIntVector& V)
{
	return FIntVector(I - V.X, I - V.Y, I - V.Z);
}

FORCEINLINE FIntVector operator+(const FIntVector& V, int32 I)
{
	return FIntVector(V.X + I, V.Y + I, V.Z + I);
}
FORCEINLINE FIntVector operator+(const FIntVector& V, uint32 I)
{
	return FIntVector(V.X + I, V.Y + I, V.Z + I);
}
FORCEINLINE FIntVector operator+(int32 I, const FIntVector& V)
{
	return FIntVector(I + V.X, I + V.Y, I + V.Z);
}
FORCEINLINE FIntVector operator+(uint32 I, const FIntVector& V)
{
	return FIntVector(I + V.X, I + V.Y, I + V.Z);
}

FORCEINLINE FIntVector operator*(int32 I, const FIntVector& V)
{
	return FIntVector(I * V.X, I * V.Y, I * V.Z);
}
FORCEINLINE FIntVector operator*(uint32 I, const FIntVector& V)
{
	return FIntVector(I * V.X, I * V.Y, I * V.Z);
}
FORCEINLINE FIntVector operator*(const FIntVector& V, uint32 I)
{
	return FIntVector(I * V.X, I * V.Y, I * V.Z);
}
FORCEINLINE FIntVector operator*(const FIntVector& A, const FIntVector& B)
{
	return FIntVector(A.X * B.X, A.Y * B.Y, A.Z * B.Z);
}

FORCEINLINE FIntVector operator%(const FIntVector& A, const FIntVector& B)
{
	return FIntVector(A.X % B.X, A.Y % B.Y, A.Z % B.Z);
}
FORCEINLINE FIntVector operator%(const FIntVector& V, int32 I)
{
	return V % FIntVector(I);
}
FORCEINLINE FIntVector operator%(const FIntVector& V, uint32 I)
{
	return V % FIntVector(I);
}

FORCEINLINE bool operator==(const FIntVector& V, int32 I)
{
	return V.X == I && V.Y == I && V.Z == I;
}

FORCEINLINE FIntPoint operator*(int32 I, const FIntPoint& V)
{
	return FIntPoint(I * V.X, I * V.Y);
}
FORCEINLINE FIntPoint operator*(uint32 I, const FIntPoint& V)
{
	return FIntPoint(I * V.X, I * V.Y);
}

FORCEINLINE FVector3f operator*(FVector3f F, const FIntVector& I)
{
	return FVector3f(F.X * I.X, F.Y * I.Y, F.Z * I.Z);
}
FORCEINLINE FVector3d operator*(FVector3d F, const FIntVector& I)
{
	return FVector3d(F.X * I.X, F.Y * I.Y, F.Z * I.Z);
}

template<typename T>
FIntVector operator-(const FIntVector& V, T A) = delete;
template<typename T>
FIntVector operator-(T A, const FIntVector& V) = delete;

template<typename T>
FIntVector operator+(const FIntVector& V, T A) = delete;
template<typename T>
FIntVector operator+(T A, const FIntVector& V) = delete;

template<typename T>
FIntVector operator*(const FIntVector& V, T A) = delete;
template<typename T>
FIntVector operator*(T A, const FIntVector& V) = delete;

template<typename T>
FIntVector operator%(const FIntVector& V, T A) = delete;
template<typename T>
FIntVector operator/(const FIntVector& V, T A) = delete;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FIntPoint operator*(const FIntPoint&, float) = delete;
FIntPoint operator*(float, const FIntPoint& V) = delete;

FIntPoint operator*(const FIntPoint&, double) = delete;
FIntPoint operator*(double, const FIntPoint& V) = delete;

FORCEINLINE FIntPoint operator%(const FIntPoint& A, const FIntPoint& B)
{
	return FIntPoint(A.X % B.X, A.Y % B.Y);
}
FORCEINLINE FIntPoint operator%(const FIntPoint& V, int32 I)
{
	return V % FIntPoint(I);
}
FORCEINLINE FIntPoint operator%(const FIntPoint& V, uint32 I)
{
	return V % FIntPoint(I);
}