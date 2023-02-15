// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "VoxelMinimal/VoxelIntBox.h"
#include "VoxelMinimal/Containers/VoxelStaticArray.h"
#include "VoxelMinimal/Utilities/VoxelBaseUtilities.h"
#include "VoxelMinimal/Utilities/VoxelVectorUtilities.h"
#include "VoxelMinimal/Utilities/VoxelIntVectorUtilities.h"
#include "Algo/IsSorted.h"
#include "VoxelMathUtilities.generated.h"

UENUM(BlueprintType)
enum class EVoxelFalloff : uint8
{
	Linear,
	Smooth,
	Spherical,
	Tip
};

namespace FVoxelUtilities
{
	// Falloff: between 0 and 1
	FORCEINLINE float RoundCylinder(const FVector3f& LocalPosition, float Radius, float Height, float Falloff)
	{
		const float InternalRadius = Radius * (1.f - Falloff);
		const float ExternalRadius = Radius * Falloff;

		const float DistanceToCenterXY = FVector2f(LocalPosition.X, LocalPosition.Y).Size();
		const float DistanceToCenterZ = FMath::Abs(LocalPosition.Z);

		const float SidesSDF = DistanceToCenterXY - InternalRadius;
		const float TopSDF = DistanceToCenterZ - Height / 2 + ExternalRadius;

		return
			FMath::Min<float>(FMath::Max<float>(SidesSDF, TopSDF), 0.0f) +
			FVector2f(FMath::Max<float>(SidesSDF, 0.f), FMath::Max<float>(TopSDF, 0.f)).Size() +
			-ExternalRadius;
	}

	FORCEINLINE float BoxSDF(const FVector3f& LocalPosition, const FVector3f& Extent)
	{
		const FVector3f Q = Abs(LocalPosition) - Extent;
		return ComponentMax(Q, FVector3f(0.f)).Size() + FMath::Min(Q.GetMax(), 0.f);
	}

	FORCEINLINE float CapsuleSDF(FVector3f LocalPosition, const float HalfHeight, const float Radius)
	{
		const float Offset = HalfHeight - Radius;
		LocalPosition.Z -= FMath::Clamp(LocalPosition.Z, -Offset, Offset);
		return LocalPosition.Size() - Radius;
	}

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	template<typename TIn, typename TOut>
	FORCEINLINE void XWayBlend_AlphasToStrengths_Impl(int32 NumChannels, const TIn& Alphas, TOut& Strengths)
	{
		ensureVoxelSlow(NumChannels > 1);
		
		// Unpack the strengths from the lerp values
		for (int32 Index = 0; Index < NumChannels; Index++)
		{
			Strengths[Index] = Index == 0 ? 1.f : Alphas[Index - 1];
			for (int32 AlphaIndex = Index; AlphaIndex < NumChannels - 1; AlphaIndex++)
			{
				Strengths[Index] *= 1.f - Alphas[AlphaIndex];
			}
		}

#if VOXEL_DEBUG && 0 // Happens pretty often
		float Sum = 0.f;
		for (int32 Index = 0; Index < NumChannels; Index++)
		{
			Sum += Strengths[Index];
		}
		ensure(FMath::IsNearlyEqual(Sum, 1.f, KINDA_SMALL_NUMBER));
#endif
	}

	template<typename TIn, typename TOut>
	FORCEINLINE void XWayBlend_StrengthsToAlphas_Impl(int32 NumChannels, TIn Strengths, TOut& Alphas, uint32 ChannelsToKeepIntact = 0)
	{
		ensureVoxelSlow(NumChannels > 1);
		
		if (!ChannelsToKeepIntact)
		{
			// Normalize: we want the sum to be 1
			float Sum = 0.f;
			for (int32 Index = 0; Index < NumChannels; Index++)
			{
				Sum += Strengths[Index];
			}
			if (Sum != 0.f) // This can very rarely happen if we subtracted all the data when editing
			{
				for (int32 Index = 0; Index < NumChannels; Index++)
				{
					Strengths[Index] /= Sum;
				}
			}
		}
		else
		{
			// Normalize so that Strengths[Index] doesn't change if (1 << Index) & ChannelsToKeepIntact
			{
				// Sum of all the other components
				float SumToKeepIntact = 0.f;
				float SumToChange = 0.f;
				for (int32 Index = 0; Index < NumChannels; Index++)
				{
					if ((1u << Index) & ChannelsToKeepIntact)
					{
						SumToKeepIntact += Strengths[Index];
					}
					else
					{
						SumToChange += Strengths[Index];
					}
				}

				// If the sum to keep intact is above 1, normalize these channels too
				// (but on their own)
				if (SumToKeepIntact > 1.f)
				{
					for (int32 Index = 0; Index < NumChannels; Index++)
					{
						if ((1u << Index) & ChannelsToKeepIntact)
						{
							Strengths[Index] /= SumToKeepIntact;
						}
					}
					SumToKeepIntact = 1.f;
				}
				
				// We need to split this into the other channels.
				const float SumToSplit = 1.f - SumToKeepIntact;
				if (SumToChange == 0.f)
				{
					// If the sum is 0, increase all the other channels the same way
					const float Value = SumToSplit / (NumChannels - FMath::CountBits(ChannelsToKeepIntact));
					
					for (int32 Index = 0; Index < NumChannels; Index++)
					{
						if (!((1u << Index) & ChannelsToKeepIntact))
						{
							Strengths[Index] = Value;
						}
					}
				}
				else
				{
					// Else scale them
					const float Value = SumToSplit / SumToChange;
					
					for (int32 Index = 0; Index < NumChannels; Index++)
					{
						if (!((1u << Index) & ChannelsToKeepIntact))
						{
							Strengths[Index] *= Value;
						}
					}
				}
			}
		}

#if VOXEL_DEBUG && 0 // Raised pretty often
		float Sum = 0.f;
		for (int32 Index = 0; Index < NumChannels; Index++)
		{
			Sum += Strengths[Index];
		}
		ensure(FMath::IsNearlyEqual(Sum, 1.f, 0.001f));
#endif

		const auto SafeDivide = [](float X, float Y)
		{
			// Here we resolve Y * A = X. If Y = 0, X should be 0 and A can be anything (here return 0)
			ensureVoxelSlowNoSideEffects(Y != 0.f || FMath::IsNearlyZero(X));
			return Y == 0.f ? 0.f : X / Y;
		};
		
		// Pack them back in: do the maths in reverse order
		const int32 NumAlphas = NumChannels - 1;
		for (int32 AlphaIndex = NumAlphas - 1; AlphaIndex >= 0; AlphaIndex--)
		{
			float Divisor = 1.f;
			for (int32 DivisorIndex = AlphaIndex + 1; DivisorIndex < NumAlphas; DivisorIndex++)
			{
				Divisor *= 1.f - Alphas[DivisorIndex];
			}

			Alphas[AlphaIndex] = SafeDivide(Strengths[AlphaIndex + 1], Divisor);
			
		}
	}

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	template<int32 NumChannels>
	FORCEINLINE TVoxelStaticArray<float, NumChannels> XWayBlend_AlphasToStrengths_Static(const TVoxelStaticArray<float, NumChannels - 1>& Alphas)
	{
		TVoxelStaticArray<float, NumChannels> Strengths{ NoInit };
		XWayBlend_AlphasToStrengths_Impl(NumChannels, Alphas, Strengths);
		return Strengths;
	}

	template<int32 NumChannels>
	FORCEINLINE TVoxelStaticArray<float, NumChannels - 1> XWayBlend_StrengthsToAlphas_Static(const TVoxelStaticArray<float, NumChannels>& Strengths, uint32 ChannelsToKeepIntact = 0)
	{
		TVoxelStaticArray<float, NumChannels - 1> Alphas{ NoInit };
		XWayBlend_StrengthsToAlphas_Impl(NumChannels, Strengths, Alphas, ChannelsToKeepIntact);
		return Alphas;
	}
	// Avoid mistakes with ChannelsToKeepIntact not being a mask
	template<int32 NumChannels, typename T>
	void XWayBlend_StrengthsToAlphas_Static(const TVoxelStaticArray<float, NumChannels>&, T) = delete;

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	template<typename TOutAllocator = FDefaultAllocator, typename TInArray>
	FORCEINLINE TArray<float, TOutAllocator> XWayBlend_AlphasToStrengths_Dynamic(const TInArray& Alphas)
	{
		TArray<float, TOutAllocator> Strengths;
		Strengths.SetNumUninitialized(Alphas.Num() + 1);
		XWayBlend_AlphasToStrengths_Impl(Alphas.Num() + 1, Alphas, Strengths);
		return Strengths;
	}

	template<typename TOutAllocator = FDefaultAllocator, typename TInArray>
	FORCEINLINE TArray<float, TOutAllocator> XWayBlend_StrengthsToAlphas_Dynamic(const TInArray& Strengths, uint32 ChannelsToKeepIntact = 0)
	{
		TArray<float, TOutAllocator> Alphas;
		Alphas.SetNumUninitialized(Strengths.Num() - 1);
		XWayBlend_StrengthsToAlphas_Impl(Strengths.Num(), Strengths, Alphas, ChannelsToKeepIntact);
		return Alphas;
	}
	// Avoid mistakes with ChannelsToKeepIntact not being a mask
	template<typename TInArray, typename T>
	void XWayBlend_StrengthsToAlphas_Dynamic(const TInArray&, T) = delete;

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	
	template<int32 N, typename T, typename ArrayType, typename TLambda>
	FORCEINLINE TVoxelStaticArray<TTuple<int32, T>, N> FindTopXElements_Impl(const ArrayType& Array, TLambda LessThan = TLess<T>())
	{
		checkVoxelSlow(Array.Num() >= N);

		// Biggest elements on top
		TVoxelStaticArray<TTuple<int32, T>, N> Stack{ NoInit };

		// Fill stack with first N values
		for (int32 Index = 0; Index < N; Index++)
		{
			Stack[Index].template Get<0>() = Index;
			Stack[Index].template Get<1>() = Array[Index];
		}

		// Sort the stack
		Algo::Sort(Stack, [&](const auto& A, const auto& B) { return LessThan(B.template Get<1>(), A.template Get<1>()); });

		for (int32 Index = N; Index < Array.Num(); Index++)
		{
			const T& ArrayValue = Array[Index];
			if (!LessThan(Stack[N - 1].template Get<1>(), ArrayValue))
			{
				// Smaller than the entire stack
				continue;
			}

			// Find the element to replace
			int32 StackToReplace = N - 1;
			while (StackToReplace >= 1 && LessThan(Stack[StackToReplace - 1].template Get<1>(), ArrayValue))
			{
				StackToReplace--;
			}

			// Move existing elements down
			for (int32 StackIndex = N - 1; StackIndex > StackToReplace; StackIndex--)
			{
				Stack[StackIndex] = Stack[StackIndex - 1];
			}

			// Write new element
			Stack[StackToReplace].template Get<0>() = Index;
			Stack[StackToReplace].template Get<1>() = ArrayValue;
		}

		return Stack;
	}
	template<int32 N, typename T, typename Allocator, typename TLambda>
	FORCEINLINE TVoxelStaticArray<TTuple<int32, T>, N> FindTopXElements(const TArray<T, Allocator>& Array, TLambda LessThan = TLess<T>())
	{
		return FindTopXElements_Impl<N, T>(Array, LessThan);
	}
	template<int32 N, typename T, int32 Num, typename TLambda>
	FORCEINLINE TVoxelStaticArray<TTuple<int32, T>, N> FindTopXElements(const TVoxelStaticArray<T, Num>& Array, TLambda LessThan = TLess<T>())
	{
		return FindTopXElements_Impl<N, T>(Array, LessThan);
	}

	template<
		typename ArrayTypeA,
		typename ArrayTypeB,
		typename OutArrayTypeA,
		typename OutArrayTypeB,
		typename PredicateType>
		void DiffSortedArrays(
		const ArrayTypeA& ArrayA,
		const ArrayTypeB& ArrayB,
		OutArrayTypeA& OutOnlyInA,
		OutArrayTypeB& OutOnlyInB,
		PredicateType Less)
	{
		VOXEL_FUNCTION_COUNTER();

		checkVoxelSlow(Algo::IsSorted(ArrayA, Less));
		checkVoxelSlow(Algo::IsSorted(ArrayB, Less));

		int64 IndexA = 0;
		int64 IndexB = 0;
		while (IndexA < ArrayA.Num() && IndexB < ArrayB.Num())
		{
			const auto& A = ArrayA[IndexA];
			const auto& B = ArrayB[IndexB];
			
			if (Less(A, B))
			{
				OutOnlyInA.Add(A);
				IndexA++;
			}
			else if (Less(B, A))
			{
				OutOnlyInB.Add(B);
				IndexB++;
			}
			else
			{
				IndexA++;
				IndexB++;
			}
		}

		while (IndexA < ArrayA.Num())
		{
			OutOnlyInA.Add(ArrayA[IndexA]);
			IndexA++;
		}

		while (IndexB < ArrayB.Num())
		{
			OutOnlyInB.Add(ArrayB[IndexB]);
			IndexB++;
		}

		checkVoxelSlow(IndexA == ArrayA.Num());
		checkVoxelSlow(IndexB == ArrayB.Num());
	}
	
	template<typename SourceArrayType, typename DestArrayType, typename IndexArrayType>
	void BulkRemoveAt_TwoArrays(const SourceArrayType& SourceArray, DestArrayType& DestArray, const IndexArrayType& SortedIndicesToRemove)
	{
		checkVoxelSlow(Algo::IsSorted(SortedIndicesToRemove));
		checkVoxelSlow(DestArray.Num() >= SourceArray.Num() - SortedIndicesToRemove.Num());

		int32 IndicesToRemoveIndex = 0;
		int32 NewIndex = 0;
		for (int32 Index = 0; Index < SourceArray.Num(); Index++)
		{
			if (IndicesToRemoveIndex < SortedIndicesToRemove.Num() &&
				SortedIndicesToRemove[IndicesToRemoveIndex] == Index)
			{
				IndicesToRemoveIndex++;
				continue;
			}

			DestArray[NewIndex++] = SourceArray[Index];
		}
		ensure(NewIndex == SourceArray.Num() - SortedIndicesToRemove.Num());
		DestArray.SetNum(NewIndex);
	}

	template<typename ArrayType, typename IndexArrayType>
	void BulkRemoveAt(ArrayType& Array, const IndexArrayType& SortedIndicesToRemove)
	{
		FVoxelUtilities::BulkRemoveAt_TwoArrays(Array, Array, SortedIndicesToRemove);
	}

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	template<typename T = int64>
	FORCEINLINE T Get2DIndex(int32 SizeX, int32 SizeY, int32 X, int32 Y, const FIntPoint& Offset = FIntPoint(0, 0))
	{
		X -= Offset.X;
		Y -= Offset.Y;
		checkVoxelSlow(0 <= X && X < SizeX);
		checkVoxelSlow(0 <= Y && Y < SizeY);
		checkVoxelSlow(int64(SizeX) * int64(SizeY) <= TNumericLimits<T>::Max());
		return T(X) + T(Y) * SizeX;
	}
	template<typename T = int64>
	FORCEINLINE T Get2DIndex(int32 SizeX, int32 SizeY, int32 X, int32 Y, int32 Offset)
	{
		return Get2DIndex<T>(SizeX, SizeY, X, Y, FIntPoint(Offset));
	}
	template<typename T = int64>
	FORCEINLINE T Get2DIndex(int32 Size, int32 X, int32 Y, const FIntPoint& Offset = FIntPoint(0, 0))
	{
		return Get2DIndex<T>(Size, Size, X, Y, Offset);
	}
	template<typename T = int64>
	FORCEINLINE T Get2DIndex(const FIntPoint& Size, int32 X, int32 Y, const FIntPoint& Offset = FIntPoint(0, 0))
	{
		return Get2DIndex<T>(Size.X, Size.Y, X, Y, Offset);
	}
	template<typename T = int64>
	FORCEINLINE T Get2DIndex(int32 Size, const FIntPoint& Position, const FIntPoint& Offset = FIntPoint(0, 0))
	{
		return Get2DIndex<T>(Size, Position.X, Position.Y, Offset);
	}
	template<typename T = int64>
	FORCEINLINE T Get2DIndex(const FIntPoint& Size, const FIntPoint& Position, const FIntPoint& Offset = FIntPoint(0, 0))
	{
		return Get2DIndex<T>(Size, Position.X, Position.Y, Offset);
	}
	
	template<typename T>
	FORCEINLINE T& Get2D(T* RESTRICT Array, const FIntPoint& Size, int32 X, int32 Y, const FIntPoint& Offset = FIntPoint(0, 0))
	{
		return Array[Get2DIndex(Size, X, Y, Offset)];
	}
	template<typename T>
	FORCEINLINE T& Get2D(T* RESTRICT Array, const FIntPoint& Size, const FIntPoint& Position, const FIntPoint& Offset = FIntPoint(0, 0))
	{
		return Get2D(Array, Size, Position.X, Position.Y, Offset);
	}
	
	template<typename T>
	FORCEINLINE auto& Get2D(T& Array, const FIntPoint& Size, int32 X, int32 Y, const FIntPoint& Offset = FIntPoint(0, 0))
	{
		checkVoxelSlow(GetNum(Array) == Size.X * Size.Y);
		return Get2D(GetData(Array), Size, X, Y, Offset);
	}
	template<typename T>
	FORCEINLINE auto& Get2D(T& Array, const FIntPoint& Size, const FIntPoint& Position, const FIntPoint& Offset = FIntPoint(0, 0))
	{
		return Get2D(Array, Size, Position.X, Position.Y, Offset);
	}

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	template<typename T = int64>
	FORCEINLINE T Get3DIndex(const FIntVector& Size, int32 X, int32 Y, int32 Z, const FIntVector& Offset = FIntVector(0, 0, 0))
	{
		X -= Offset.X;
		Y -= Offset.Y;
		Z -= Offset.Z;
		checkVoxelSlow(0 <= X && X < Size.X);
		checkVoxelSlow(0 <= Y && Y < Size.Y);
		checkVoxelSlow(0 <= Z && Z < Size.Z);
		checkVoxelSlow(int64(Size.X) * int64(Size.Y) * int64(Size.Z) <= TNumericLimits<T>::Max());
		return T(X) + T(Y) * Size.X + T(Z) * Size.X * Size.Y;
	}
	template<typename T = int64>
	FORCEINLINE T Get3DIndex(const FIntVector& Size, int32 X, int32 Y, int32 Z, int32 Offset)
	{
		return Get3DIndex<T>(Size, X, Y, Z, FIntVector(Offset));
	}
	template<typename T = int64>
	FORCEINLINE T Get3DIndex(int32 Size, int32 X, int32 Y, int32 Z, int32 Offset = 0)
	{
		return Get3DIndex<T>(FIntVector(Size), X, Y, Z, FIntVector(Offset));
	}
	template<typename T = int64>
	FORCEINLINE T Get3DIndex(const FIntVector& Size, const FIntVector& Position, const FIntVector& Offset = FIntVector(0, 0, 0))
	{
		return Get3DIndex<T>(Size, Position.X, Position.Y, Position.Z, Offset);
	}
	template<typename T = int64>
	FORCEINLINE T Get3DIndex(int32 Size, const FIntVector& Position, const FIntVector& Offset = FIntVector(0, 0, 0))
	{
		return Get3DIndex<T>(FIntVector(Size), Position, Offset);
	}
	
	template<typename T = int64>
	FORCEINLINE FIntVector Break3DIndex_Log2(int32 SizeLog2, T Index)
	{
		const int32 X = (Index >> (0 * SizeLog2)) & ((1 << SizeLog2) - 1);
		const int32 Y = (Index >> (1 * SizeLog2)) & ((1 << SizeLog2) - 1);
		const int32 Z = (Index >> (2 * SizeLog2)) & ((1 << SizeLog2) - 1);
		checkVoxelSlow(Get3DIndex<T>(1 << SizeLog2, X, Y, Z) == Index);
		return { X, Y, Z };
	}
	
	template<typename T = int64>
	FORCEINLINE FIntVector Break3DIndex(const FIntVector& Size, T Index)
	{
		const T OriginalIndex = Index;

		const int32 Z = Index / (Size.X * Size.Y);
		checkVoxelSlow(0 <= Z && Z < Size.Z);
		Index -= Z * Size.X * Size.Y;

		const int32 Y = Index / Size.X;
		checkVoxelSlow(0 <= Y && Y < Size.Y);
		Index -= Y * Size.X;

		const int32 X = Index;
		checkVoxelSlow(0 <= X && X < Size.X);

		checkVoxelSlow(Get3DIndex<T>(Size, X, Y, Z) == OriginalIndex);

		return { X, Y, Z };
	}
	template<typename T = int64>
	FORCEINLINE FIntVector Break3DIndex(int32 Size, T Index)
	{
		return FVoxelUtilities::Break3DIndex<T>(FIntVector(Size), Index);
	}
	
	template<typename T>
	FORCEINLINE T& Get3D(T* RESTRICT Array, const FIntVector& Size, int32 X, int32 Y, int32 Z, const FIntVector& Offset = FIntVector(0, 0, 0))
	{
		return Array[Get3DIndex(Size, X, Y, Z, Offset)];
	}
	template<typename T>
	FORCEINLINE T& Get3D(T* RESTRICT Array, const FIntVector& Size, const FIntVector& Position, const FIntVector& Offset = FIntVector(0, 0, 0))
	{
		return Get3D(Array, Size, Position.X, Position.Y, Position.Z, Offset);
	}
	
	template<typename T>
	FORCEINLINE auto& Get3D(T& Array, const FIntVector& Size, int32 X, int32 Y, int32 Z, const FIntVector& Offset = FIntVector(0, 0, 0))
	{
		checkVoxelSlow(GetNum(Array) == Size.X * Size.Y * Size.Z);
		return Get3D(GetData(Array), Size, X, Y, Z, Offset);
	}
	template<typename T>
	FORCEINLINE auto& Get3D(T& Array, const FIntVector& Size, const FIntVector& Position, const FIntVector& Offset = FIntVector(0, 0, 0))
	{
		return Get3D(Array, Size, Position.X, Position.Y, Position.Z, Offset);
	}

	template<typename ArrayType>
	void ForceBulkSerializeArray(FArchive& Ar, ArrayType& Array)
	{
		if (Ar.IsLoading())
		{
			typename ArrayType::SizeType NewArrayNum = 0;
			Ar << NewArrayNum;
			Array.Empty(NewArrayNum);
			Array.AddUninitialized(NewArrayNum);
			Ar.Serialize(Array.GetData(), NewArrayNum * Array.GetTypeSize());
		}
		else if (Ar.IsSaving())
		{
			typename ArrayType::SizeType ArrayCount = Array.Num();
			Ar << ArrayCount;
			Ar.Serialize(Array.GetData(), ArrayCount * Array.GetTypeSize());
		}
	}

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	FORCEINLINE float LinearFalloff(float Distance, float Radius, float Falloff)
	{
		return Distance <= Radius
			? 1.0f
			: Radius + Falloff <= Distance
			? 0.f
			: 1.0f - (Distance - Radius) / Falloff;
	}
	FORCEINLINE float SmoothFalloff(float Distance, float Radius, float Falloff)
	{
		const float X = LinearFalloff(Distance, Radius, Falloff);
		return FMath::SmoothStep(0.f, 1.f, X);
	}
	FORCEINLINE float SphericalFalloff(float Distance, float Radius, float Falloff)
	{
		return Distance <= Radius
			? 1.0f
			: Radius + Falloff <= Distance
			? 0.f
			: FMath::Sqrt(1.0f - FMath::Square((Distance - Radius) / Falloff));
	}
	FORCEINLINE float TipFalloff(float Distance, float Radius, float Falloff)
	{
		return Distance <= Radius
			? 1.0f
			: Radius + Falloff <= Distance
			? 0.f
			: 1.0f - FMath::Sqrt(1.0f - FMath::Square((Falloff + Radius - Distance) / Falloff));
	}

	// Falloff: between 0 and 1
	FORCEINLINE float GetFalloff(EVoxelFalloff FalloffType, float Distance, float Radius, float Falloff)
	{
		Falloff = FMath::Clamp(Falloff, 0.f, 1.f);
		if (Falloff == 0.f)
		{
			return Distance <= Radius ? 1.f : 0.f;
		}

		const float RelativeRadius = Radius * (1.f - Falloff);
		const float RelativeFalloff = Radius * Falloff;
		switch (FalloffType)
		{
		default: VOXEL_ASSUME(false);
		case EVoxelFalloff::Linear:
		{
			return LinearFalloff(Distance, RelativeRadius, RelativeFalloff);
		}
		case EVoxelFalloff::Smooth:
		{
			return SmoothFalloff(Distance, RelativeRadius, RelativeFalloff);
		}
		case EVoxelFalloff::Spherical:
		{
			return SphericalFalloff(Distance, RelativeRadius, RelativeFalloff);
		}
		case EVoxelFalloff::Tip:
		{
			return TipFalloff(Distance, RelativeRadius, RelativeFalloff);
		}
		}
	}

	FORCEINLINE float PackIntIntoFloat(uint32 Int)
	{
		return *reinterpret_cast<float*>(&Int);
	}

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	template<typename T, typename Array>
	void GetMinAvgMax(const Array& Values, T& MinV, double& AvgV, T& MaxV)
	{
		MinV = TNumericLimits<T>::Max();
		MaxV = TNumericLimits<T>::Lowest();
		AvgV = 0.0;
		for (const T& V : Values)
		{
			MinV = V < MinV ? V : MinV;
			MaxV = V > MaxV ? V : MaxV;
			AvgV += V;
		}
		if (Values.Num())
		{
			AvgV /= Values.Num();
		}
		else
		{
			MinV = MaxV = 0;
		}
	}

	template<typename T>
	float GetAverage(const T& Values)
	{
		double AvgV = 0.0;
		for (auto& V : Values)
		{
			AvgV += V;
		}
		if (Values.Num())
		{
			AvgV /= Values.Num();
		}
		return AvgV;
	}

	template<typename T>
	float GetVariance(const T& Values, const double Avg)
	{
		double Variance = 0.0;
		for (auto& V : Values)
		{
			const double Deviation = V - Avg;
			Variance += Deviation * Deviation;
		}
		if (Values.Num())
		{
			Variance /= Values.Num();
		}
		return Variance;
	}

	template<typename T>
	float GetVariance(const T& Values)
	{
		return GetVariance(Values, GetAverage(Values));
	}
	template<typename T>
	float GetStandardDeviation(const T& Values)
	{
		const float Variance = GetVariance(Values);
		return FMath::Sqrt(Variance);
	}

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	FORCEINLINE float SmoothMin(float DistanceA, float DistanceB, float Smoothness)
	{
		const float H = FMath::Clamp(0.5f + 0.5f * (DistanceB - DistanceA) / Smoothness, 0.0f, 1.0f);
		return FMath::Lerp(DistanceB, DistanceA, H) - Smoothness * H * (1.0f - H);
	}

	// Subtract DistanceB from DistanceA
	// DistanceB should be negative/inverted
	FORCEINLINE float SmoothSubtraction(float DistanceA, float DistanceB, float Smoothness)
	{
		const float H = FMath::Clamp(0.5f + 0.5f * (DistanceB - DistanceA) / Smoothness, 0.0f, 1.0f);
		return FMath::Lerp(DistanceA, DistanceB, H) + Smoothness * H * (1.0f - H);
	}

	FORCEINLINE float SmoothMax(float DistanceA, float DistanceB, float Smoothness)
	{
		const float H = FMath::Clamp(0.5f - 0.5f * (DistanceB - DistanceA) / Smoothness, 0.0f, 1.0f);
		return FMath::Lerp(DistanceB, DistanceA, H) + Smoothness * H * (1.0f - H);
	}

	// See https://www.iquilezles.org/www/articles/smin/smin.htm
	// Unlike SmoothMin this is order-independent
	FORCEINLINE float ExponentialSmoothMin(float DistanceA, float DistanceB, float Smoothness)
	{
		ensureVoxelSlow(Smoothness > 0);
		const float H = FMath::Exp(-DistanceA / Smoothness) + FMath::Exp(-DistanceB / Smoothness);
		return -FMath::Loge(H) * Smoothness;
	}

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	template<typename ArrayType>
	FORCEINLINE auto Sample2D(const ArrayType& Array, const FIntPoint& Size, const FVector2f& Point) -> decltype(auto)
	{
		const int32 MinX = FMath::Clamp(FMath::FloorToInt(Point.X), 0, Size.X - 1);
		const int32 MinY = FMath::Clamp(FMath::FloorToInt(Point.Y), 0, Size.Y - 1);

		const int32 MaxX = FMath::Clamp(FMath::CeilToInt(Point.X), 0, Size.X - 1);
		const int32 MaxY = FMath::Clamp(FMath::CeilToInt(Point.Y), 0, Size.Y - 1);

		const float AlphaX = FMath::Clamp(Point.X - MinX, 0.f, 1.f);
		const float AlphaY = FMath::Clamp(Point.Y - MinY, 0.f, 1.f);

		const auto Value00 = Array[Get2DIndex(Size, MinX, MinY)];
		const auto Value10 = Array[Get2DIndex(Size, MaxX, MinY)];
		const auto Value01 = Array[Get2DIndex(Size, MinX, MaxY)];
		const auto Value11 = Array[Get2DIndex(Size, MaxX, MaxY)];

		return FVoxelUtilities::BilinearInterpolation(
			Value00,
			Value10,
			Value01,
			Value11,
			AlphaX,
			AlphaY);
	}

	template<typename ArrayType>
	FORCEINLINE FVector2f Sample2DGradient(const ArrayType& Array, const FIntPoint& Size, const FVector2f& Point, float Step = 1)
	{
		const float ValueMinX = float(Sample2D(Array, Size, Point - FVector2f(Step, 0)));
		const float ValueMaxX = float(Sample2D(Array, Size, Point + FVector2f(Step, 0)));
		const float ValueMinY = float(Sample2D(Array, Size, Point - FVector2f(0, Step)));
		const float ValueMaxY = float(Sample2D(Array, Size, Point + FVector2f(0, Step)));

		return FVector2f(ValueMaxX - ValueMinX, ValueMaxY - ValueMinY).GetSafeNormal();
	}

	FORCEINLINE float RemapValue(float Value, float Min, float Max, float NewMin, float NewMax)
	{
		const float Alpha = (Value - Min) / (Max - Min);
		return NewMin + Alpha * (NewMax - NewMin);
	}

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	// H00
	FORCEINLINE float HermiteP0(float T)
	{
		return (1 + 2 * T) * FMath::Square(1 - T);
	}
	// H10
	FORCEINLINE float HermiteD0(float T)
	{
		return T * FMath::Square(1 - T);
	}

	// H01
	FORCEINLINE float HermiteP1(float T)
	{
		return FMath::Square(T) * (3 - 2 * T);
	}
	// H11
	FORCEINLINE float HermiteD1(float T)
	{
		return FMath::Square(T) * (T - 1);
	}

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	template<typename VectorType>
	FORCEINLINE bool SegmentAreIntersecting(
		const VectorType& StartA, 
		const VectorType& EndA, 
		const VectorType& StartB, 
		const VectorType& EndB)
	{
		const VectorType VectorA = EndA - StartA;
		const VectorType VectorB = EndB - StartB;

		const auto S =
			(VectorA.X * (StartA.Y - StartB.Y) - VectorA.Y * (StartA.X - StartB.X)) /
			(VectorA.X * VectorB.Y - VectorA.Y * VectorB.X);

		const auto T =
			(VectorB.X * (StartB.Y - StartA.Y) - VectorB.Y * (StartB.X - StartA.X)) /
			(VectorB.X * VectorA.Y - VectorB.Y * VectorA.X);

		return
			0 <= S && S <= 1 &&
			0 <= T && T <= 1;
	}

	FORCEINLINE bool RayTriangleIntersection(
		const FVector3f& RayOrigin,
		const FVector3f& RayDirection,
		const FVector3f& VertexA,
		const FVector3f& VertexB,
		const FVector3f& VertexC,
		const bool bAllowNegativeTime,
		float& OutTime,
		FVector3f* OutBarycentrics = nullptr)
	{
		const FVector3f Diff = RayOrigin - VertexA;
		const FVector3f Edge1 = VertexB - VertexA;
		const FVector3f Edge2 = VertexC - VertexA;
		const FVector3f Normal = FVector3f::CrossProduct(Edge1, Edge2);

		// With:
		// Q = Diff, D = RayDirection, E1 = Edge1, E2 = Edge2, N = Cross(E1, E2)
		//
		// Solve:
		// Q + t * D = b1 * E1 + b2 * E2
		//
		// Using:
		//   |Dot(D, N)| * b1 = sign(Dot(D, N)) * Dot(D, Cross(Q, E2))
		//   |Dot(D, N)| * b2 = sign(Dot(D, N)) * Dot(D, Cross(E1, Q))
		//   |Dot(D, N)| * t = -sign(Dot(D, N)) * Dot(Q, N)

		float Dot = RayDirection.Dot(Normal);
		float Sign;
		if (Dot > KINDA_SMALL_NUMBER)
		{
			Sign = 1;
		}
		else if (Dot < -KINDA_SMALL_NUMBER)
		{
			Sign = -1;
			Dot = -Dot;
		}
		else
		{
			// Ray and triangle are parallel
			return false;
		}

		const float DotTimesB1 = Sign * RayDirection.Dot(Diff.Cross(Edge2));
		if (DotTimesB1 < 0)
		{
			// b1 < 0, no intersection
			return false;
		}

		const float DotTimesB2 = Sign * RayDirection.Dot(Edge1.Cross(Diff));
		if (DotTimesB2 < 0)
		{
			// b2 < 0, no intersection
			return false;
		}

		if (DotTimesB1 + DotTimesB2 > Dot)
		{
			// b1 + b2 > 1, no intersection
			return false;
		}

		// Line intersects triangle, check if ray does.
		const float DotTimesT = -Sign * Diff.Dot(Normal);
		if (DotTimesT < 0 && !bAllowNegativeTime)
		{
			// t < 0, no intersection
			return false;
		}

		// Ray intersects triangle.
		OutTime = DotTimesT / Dot;

		if (OutBarycentrics)
		{
			OutBarycentrics->Y = DotTimesB1 / Dot;
			OutBarycentrics->Z = DotTimesB2 / Dot;
			OutBarycentrics->X = 1 - OutBarycentrics->Y - OutBarycentrics->Z;
		}

		return true;
	}

	FORCEINLINE FVector3f GetTriangleNormal(const FVector3f& A, const FVector3f& B, const FVector3f& C)
	{
		return FVector3f::CrossProduct(C - A, B - A).GetSafeNormal();
	}

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	VOXELCORE_API FQuat MakeQuaternionFromEuler(double Pitch, double Yaw, double Roll);
	VOXELCORE_API FQuat MakeQuaternionFromBasis(const FVector& X, const FVector& Y, const FVector& Z);
	VOXELCORE_API FQuat MakeQuaternionFromZ(const FVector& Z);

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	FORCEINLINE FVector2f UnitVectorToOctahedron(FVector3f Unit)
	{
		ensureVoxelSlow(Unit.IsNormalized());

		const float AbsSum = FMath::Abs(Unit.X) + FMath::Abs(Unit.Y) + FMath::Abs(Unit.Z);
		Unit.X /= AbsSum;
		Unit.Y /= AbsSum;

		FVector2f Result = FVector2f(Unit.X, Unit.Y);
		if (Unit.Z <= 0)
		{
			Result.X = (1 - FMath::Abs(Unit.Y)) * (Unit.X >= 0 ? 1 : -1);
			Result.Y = (1 - FMath::Abs(Unit.X)) * (Unit.Y >= 0 ? 1 : -1);
		}
		return Result * 0.5f + 0.5f;
	}
	FORCEINLINE FVector3f OctahedronToUnitVector(FVector2f Octahedron)
	{
		ensureVoxelSlow(0 <= Octahedron.X && Octahedron.X <= 1);
		ensureVoxelSlow(0 <= Octahedron.Y && Octahedron.Y <= 1);

		Octahedron = Octahedron * 2.f - 1.f;

		FVector3f Unit;
		Unit.X = Octahedron.X;
		Unit.Y = Octahedron.Y;
		Unit.Z = 1.f - FMath::Abs(Octahedron.X) - FMath::Abs(Octahedron.Y);

		const float T = FMath::Max(-Unit.Z, 0.f);

		Unit.X += Unit.X >= 0 ? -T : T;
		Unit.Y += Unit.Y >= 0 ? -T : T;

		ensureVoxelSlow(Unit.SizeSquared() >= KINDA_SMALL_NUMBER);

		return Unit.GetUnsafeNormal();
	}
}

#define __ISPC_STRUCT_FVoxelOctahedron__

namespace ispc
{
	struct FVoxelOctahedron
	{
		uint8 X;
		uint8 Y;
	};
}

struct FVoxelOctahedron : ispc::FVoxelOctahedron
{
	FVoxelOctahedron() = default;
	FORCEINLINE explicit FVoxelOctahedron(EForceInit)
	{
		X = 0;
		Y = 0;
	}
	FORCEINLINE explicit FVoxelOctahedron(const FVector2f& Octahedron)
	{
		X = FVoxelUtilities::FloatToUINT8(Octahedron.X);
		Y = FVoxelUtilities::FloatToUINT8(Octahedron.Y);

		ensureVoxelSlow(0 <= Octahedron.X && Octahedron.X <= 1);
		ensureVoxelSlow(0 <= Octahedron.Y && Octahedron.Y <= 1);
	}
	FORCEINLINE explicit FVoxelOctahedron(const FVector3f& UnitVector)
		: FVoxelOctahedron(FVoxelUtilities::UnitVectorToOctahedron(UnitVector))
	{
	}

	FORCEINLINE FVector2f GetOctahedron() const
	{
		return
		{
				FVoxelUtilities::UINT8ToFloat(X),
				FVoxelUtilities::UINT8ToFloat(Y)
		};
	}
	FORCEINLINE FVector3f GetUnitVector() const
	{
		return FVoxelUtilities::OctahedronToUnitVector(GetOctahedron());
	}

	FORCEINLINE friend FArchive& operator<<(FArchive& Ar, FVoxelOctahedron& Octahedron)
	{
		return Ar << Octahedron.X << Octahedron.Y;
	}
};