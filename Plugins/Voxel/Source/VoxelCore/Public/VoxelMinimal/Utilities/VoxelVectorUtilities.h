// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "VoxelMinimal/Containers/VoxelStaticArray.h"

namespace FVoxelUtilities
{
	template<typename T>
	FORCEINLINE int64 FloorToInt64(T Value)
	{
		return FMath::FloorToInt64(Value);
	}
	template<typename T>
	FORCEINLINE int64 FloorToInt32(T Value)
	{
#if VOXEL_DEBUG
		const int64 Int = FMath::FloorToInt64(Value);
		ensureVoxelSlow(MIN_int32 <= Int && Int <= MAX_int32);
#endif
		return FMath::FloorToInt32(Value);
	}

	template<typename T>
	FORCEINLINE int64 CeilToInt64(T Value)
	{
		return FMath::CeilToInt64(Value);
	}
	template<typename T>
	FORCEINLINE int64 CeilToInt32(T Value)
	{
#if VOXEL_DEBUG
		const int64 Int = FMath::CeilToInt64(Value);
		ensureVoxelSlow(MIN_int32 <= Int && Int <= MAX_int32);
#endif
		return FMath::CeilToInt32(Value);
	}

	template<typename T>
	FORCEINLINE int64 RoundToInt64(T Value)
	{
		return FMath::RoundToInt64(Value);
	}
	template<typename T>
	FORCEINLINE int64 RoundToInt32(T Value)
	{
#if VOXEL_DEBUG
		const int64 Int = FMath::RoundToInt64(Value);
		ensureVoxelSlow(MIN_int32 <= Int && Int <= MAX_int32);
#endif
		return FMath::RoundToInt32(Value);
	}

	template<typename TVector, typename TResult = TVector>
	using TEnableIfAnyVector2 = typename TEnableIf<TOr<TIsSame<TVector, FIntPoint>, TIsSame<TVector, FVector2f>, TIsSame<TVector, FVector2d>>::Value, TResult>::Type;
	template<typename TVector, typename TResult = TVector>
	using TEnableIfAnyVector3 = typename TEnableIf<TOr<TIsSame<TVector, FIntVector>, TIsSame<TVector, FVector3f>, TIsSame<TVector, FVector3d>>::Value, TResult>::Type;

	template<typename TVector, typename TResult = TVector>
	using TEnableIfFloatVector2 = typename TEnableIf<TOr<TIsSame<TVector, FVector2f>, TIsSame<TVector, FVector2d>>::Value, TResult>::Type;
	template<typename TVector, typename TResult = TVector>
	using TEnableIfFloatVector3 = typename TEnableIf<TOr<TIsSame<TVector, FVector3f>, TIsSame<TVector, FVector3d>>::Value, TResult>::Type;
	
	template<typename TVector>
	FORCEINLINE TEnableIfFloatVector2<TVector, FIntPoint> RoundToInt(const TVector& Vector)
	{
		return FIntPoint(
			RoundToInt32(Vector.X),
			RoundToInt32(Vector.Y));
	}
	template<typename TVector>
	FORCEINLINE TEnableIfFloatVector3<TVector, FIntVector> RoundToInt(const TVector& Vector)
	{
		return FIntVector(
			RoundToInt32(Vector.X),
			RoundToInt32(Vector.Y),
			RoundToInt32(Vector.Z));
	}
	
	template<typename TVector>
	FORCEINLINE TEnableIfFloatVector2<TVector, FIntPoint> FloorToInt(const TVector& Vector)
	{
		return FIntPoint(
			FloorToInt32(Vector.X),
			FloorToInt32(Vector.Y));
	}
	template<typename TVector>
	FORCEINLINE TEnableIfFloatVector3<TVector, FIntVector> FloorToInt(const TVector& Vector)
	{
		return FIntVector(
			FloorToInt32(Vector.X),
			FloorToInt32(Vector.Y),
			FloorToInt32(Vector.Z));
	}
	
	template<typename TVector>
	FORCEINLINE TEnableIfFloatVector2<TVector, FIntPoint> CeilToInt(const TVector& Vector)
	{
		return FIntPoint(
			CeilToInt32(Vector.X),
			CeilToInt32(Vector.Y));
	}
	template<typename TVector>
	FORCEINLINE TEnableIfFloatVector3<TVector, FIntVector> CeilToInt(const TVector& Vector)
	{
		return FIntVector(
			CeilToInt32(Vector.X),
			CeilToInt32(Vector.Y),
			CeilToInt32(Vector.Z));
	}
	
	template<typename TVector>
	FORCEINLINE TEnableIfAnyVector2<TVector> Abs(const TVector& Vector)
	{
		return TVector(
			FMath::Abs(Vector.X),
			FMath::Abs(Vector.Y));
	}
	template<typename TVector>
	FORCEINLINE TEnableIfAnyVector3<TVector> Abs(const TVector& Vector)
	{
		return TVector(
			FMath::Abs(Vector.X),
			FMath::Abs(Vector.Y),
			FMath::Abs(Vector.Z));
	}
	
	template<typename TVector>
	FORCEINLINE TEnableIfAnyVector2<TVector> ComponentMax(const TVector& A, const TVector& B)
	{
		return TVector(
			FMath::Max(A.X, B.X),
			FMath::Max(A.Y, B.Y));
	}
	template<typename TVector>
	FORCEINLINE TEnableIfAnyVector3<TVector> ComponentMax(const TVector& A, const TVector& B)
	{
		return TVector(
			FMath::Max(A.X, B.X),
			FMath::Max(A.Y, B.Y),
			FMath::Max(A.Z, B.Z));
	}
	
	template<typename TVector>
	FORCEINLINE TEnableIfAnyVector2<TVector> ComponentMin(const TVector& A, const TVector& B)
	{
		return TVector(
			FMath::Min(A.X, B.X),
			FMath::Min(A.Y, B.Y));
	}
	template<typename TVector>
	FORCEINLINE TEnableIfAnyVector3<TVector> ComponentMin(const TVector& A, const TVector& B)
	{
		return TVector(
			FMath::Min(A.X, B.X),
			FMath::Min(A.Y, B.Y),
			FMath::Min(A.Z, B.Z));
	}

	template<typename TVector>
	FORCEINLINE TVector ComponentMin3(const TVector& A, const TVector& B, const TVector& C)
	{
		return ComponentMin(A, ComponentMin(B, C));
	}
	template<typename TVector>
	FORCEINLINE TVector ComponentMax3(const TVector& A, const TVector& B, const TVector& C)
	{
		return ComponentMax(A, ComponentMax(B, C));
	}

	template<typename TVector>
	FORCEINLINE TEnableIfAnyVector2<TVector> Clamp(const TVector& V, const TVector& Min, const TVector& Max)
	{
		return TVector(
			FMath::Clamp(V.X, Min.X, Max.X),
			FMath::Clamp(V.Y, Min.Y, Max.Y));
	}
	template<typename TVector>
	FORCEINLINE TEnableIfAnyVector3<TVector> Clamp(const TVector& V, const TVector& Min, const TVector& Max)
	{
		return TVector(
			FMath::Clamp(V.X, Min.X, Max.X),
			FMath::Clamp(V.Y, Min.Y, Max.Y),
			FMath::Clamp(V.Z, Min.Z, Max.Z));
	}

	template<typename TVector, typename ScalarTypeA, typename ScalarTypeB>
	FORCEINLINE TEnableIfAnyVector2<TVector> Clamp(const TVector& V, const ScalarTypeA& Min, const ScalarTypeB& Max)
	{
		return FVoxelUtilities::Clamp(V, TVector(Min), TVector(Max));
	}
	template<typename TVector, typename ScalarTypeA, typename ScalarTypeB>
	FORCEINLINE TEnableIfAnyVector3<TVector> Clamp(const TVector& V, const ScalarTypeA& Min, const ScalarTypeB& Max)
	{
		return FVoxelUtilities::Clamp(V, TVector(Min), TVector(Max));
	}

	template<typename TVector>
	FORCEINLINE TVoxelStaticArray<FIntVector, 8> GetNeighbors(const TVector& P)
	{
		const int32 MinX = FloorToInt32(P.X);
		const int32 MinY = FloorToInt32(P.Y);
		const int32 MinZ = FloorToInt32(P.Z);

		const int32 MaxX = CeilToInt32(P.X);
		const int32 MaxY = CeilToInt32(P.Y);
		const int32 MaxZ = CeilToInt32(P.Z);

		return
		{
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
}