// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "VoxelMinimal/VoxelBox.h"
#include "VoxelMinimal/Utilities/VoxelVectorUtilities.h"
#include "VoxelMinimal/Utilities/VoxelIntVectorUtilities.h"
#include "Async/ParallelFor.h"

/**
 * A Box with int32 coordinates
 */
struct VOXELCORE_API FVoxelIntBox
{
	// Inclusive
	FIntVector Min = FIntVector(ForceInit);

	// Exclusive
	FIntVector Max = FIntVector(ForceInit);

	static const FVoxelIntBox Infinite;

	FVoxelIntBox() = default;

	FORCEINLINE FVoxelIntBox(const FIntVector& Min, const FIntVector& Max)
		: Min(Min)
		, Max(Max)
	{
		ensureVoxelSlow(Min.X <= Max.X);
		ensureVoxelSlow(Min.Y <= Max.Y);
		ensureVoxelSlow(Min.Z <= Max.Z);
	}
	FORCEINLINE explicit FVoxelIntBox(int32 Min, const FIntVector& Max)
		: FVoxelIntBox(FIntVector(Min), Max)
	{
	}
	FORCEINLINE explicit FVoxelIntBox(const FIntVector& Min, int32 Max)
		: FVoxelIntBox(Min, FIntVector(Max))
	{
	}
	FORCEINLINE explicit FVoxelIntBox(int32 Min, int32 Max)
		: FVoxelIntBox(FIntVector(Min), FIntVector(Max))
	{
	}
	FORCEINLINE explicit FVoxelIntBox(const FVector3f& Min, const FVector3f& Max)
		: FVoxelIntBox(FVoxelUtilities::FloorToInt(Min), FVoxelUtilities::CeilToInt(Max) + 1)
	{
	}
	FORCEINLINE explicit FVoxelIntBox(const FVector3d& Min, const FVector3d& Max)
		: FVoxelIntBox(FVoxelUtilities::FloorToInt(Min), FVoxelUtilities::CeilToInt(Max) + 1)
	{
	}

	FORCEINLINE explicit FVoxelIntBox(const FVector3f& Position)
		: FVoxelIntBox(Position, Position)
	{
	}
	FORCEINLINE explicit FVoxelIntBox(const FVector3d& Position)
		: FVoxelIntBox(Position, Position)
	{
	}

	FORCEINLINE explicit FVoxelIntBox(const FIntVector& Position)
		: FVoxelIntBox(Position, Position + 1)
	{
	}

	FORCEINLINE explicit FVoxelIntBox(int32 X, int32 Y, int32 Z)
		: FVoxelIntBox(FIntVector(X, Y, Z), FIntVector(X + 1, Y + 1, Z + 1))
	{
	}
	FORCEINLINE explicit FVoxelIntBox(float X, float Y, float Z)
		: FVoxelIntBox(FVector3f(X, Y, Z), FVector3f(X + 1, Y + 1, Z + 1))
	{
	}
	FORCEINLINE explicit FVoxelIntBox(double X, double Y, double Z)
		: FVoxelIntBox(FVector3d(X, Y, Z), FVector3d(X + 1, Y + 1, Z + 1))
	{
	}

	template<typename T>
	explicit FVoxelIntBox(const TArray<T>& Data)
	{
		if (!ensure(Data.Num() > 0))
		{
			Min = Max = FIntVector::ZeroValue;
			return;
		}

		*this = FVoxelIntBox(Data[0]);
		for (int32 Index = 1; Index < Data.Num(); Index++)
		{
			*this = *this + Data[Index];
		}
	}

	static FVoxelIntBox FromPositions(TVoxelArrayView<const FIntVector> Positions);

	FORCEINLINE static FVoxelIntBox SafeConstruct(const FIntVector& A, const FIntVector& B)
	{
		FVoxelIntBox Box;
		Box.Min = FVoxelUtilities::ComponentMin(A, B);
		Box.Max = FVoxelUtilities::ComponentMax3(A, B, Box.Min + FIntVector(1, 1, 1));
		return Box;
	}
	FORCEINLINE static FVoxelIntBox SafeConstruct(const FVector& A, const FVector& B)
	{
		FVoxelIntBox Box;
		Box.Min = FVoxelUtilities::FloorToInt(FVoxelUtilities::ComponentMin(A, B));
		Box.Max = FVoxelUtilities::CeilToInt(FVoxelUtilities::ComponentMax3(A, B, FVector(Box.Min + FIntVector(1, 1, 1))));
		return Box;
	}

	template<typename T>
	FORCEINLINE static FVoxelIntBox FromFloatBox_NoPadding(T Box)
	{
		return
		{
			FVoxelUtilities::FloorToInt(Box.Min),
			FVoxelUtilities::CeilToInt(Box.Max),
		};
	}
	template<typename T>
	FORCEINLINE static FVoxelIntBox FromFloatBox_WithPadding(T Box)
	{
		return
		{
			FVoxelUtilities::FloorToInt(Box.Min),
			FVoxelUtilities::CeilToInt(Box.Max) + 1,
		};
	}

	FORCEINLINE FIntVector Size() const
	{
		ensure(SizeIs32Bit());
		return Max - Min;
	}
	FORCEINLINE FVector GetCenter() const
	{
		return FVector(Min + Max) / 2.f;
	}
	FORCEINLINE FIntVector GetIntCenter() const
	{
		return (Min + Max) / 2;
	}
	FORCEINLINE double Count_LargeBox() const
	{
		return
			(double(Max.X) - double(Min.X)) *
			(double(Max.Y) - double(Min.Y)) *
			(double(Max.Z) - double(Min.Z));
	}
	FORCEINLINE uint64 Count_SmallBox() const
	{
		checkVoxelSlow(uint64(Max.X - Min.X) < (1llu << 21));
		checkVoxelSlow(uint64(Max.Y - Min.Y) < (1llu << 21));
		checkVoxelSlow(uint64(Max.Z - Min.Z) < (1llu << 21));
		return
			uint64(Max.X - Min.X) *
			uint64(Max.Y - Min.Y) *
			uint64(Max.Z - Min.Z);
	}

	FORCEINLINE bool SizeIs32Bit() const
	{
		return
			int64(Max.X) - int64(Min.X) < MAX_int32 &&
			int64(Max.Y) - int64(Min.Y) < MAX_int32 &&
			int64(Max.Z) - int64(Min.Z) < MAX_int32;
	}

	FORCEINLINE bool IsInfinite() const
	{
		// Not exactly accurate, but should be safe
		const int32 InfiniteMin = MIN_int32 / 2;
		const int32 InfiniteMax = MAX_int32 / 2;
		return
			Min.X < InfiniteMin ||
			Min.Y < InfiniteMin ||
			Min.Z < InfiniteMin ||
			Max.X > InfiniteMax ||
			Max.Y > InfiniteMax ||
			Max.Z > InfiniteMax;
	}
 
	/**
	 * Get the corners that are inside the box (max - 1)
	 */
	TVoxelStaticArray<FIntVector, 8> GetCorners(int32 MaxBorderSize) const
	{
		return {
			FIntVector(Min.X, Min.Y, Min.Z),
			FIntVector(Max.X - MaxBorderSize, Min.Y, Min.Z),
			FIntVector(Min.X, Max.Y - MaxBorderSize, Min.Z),
			FIntVector(Max.X - MaxBorderSize, Max.Y - MaxBorderSize, Min.Z),
			FIntVector(Min.X, Min.Y, Max.Z - MaxBorderSize),
			FIntVector(Max.X - MaxBorderSize, Min.Y, Max.Z - MaxBorderSize),
			FIntVector(Min.X, Max.Y - MaxBorderSize, Max.Z - MaxBorderSize),
			FIntVector(Max.X - MaxBorderSize, Max.Y - MaxBorderSize, Max.Z - MaxBorderSize)
		};
	}
	FString ToString() const
	{
		return FString::Printf(TEXT("(%d/%d, %d/%d, %d/%d)"), Min.X, Max.X, Min.Y, Max.Y, Min.Z, Max.Z);
	}
	FVoxelBox ToVoxelBox() const
	{
		return FVoxelBox(Min, Max);
	}
	FVoxelBox ToVoxelBox_NoPadding() const
	{
		return FVoxelBox(Min, Max - 1);
	}
	FBox ToFBox() const
	{
		return FBox(FVector(Min), FVector(Max));
	}
	FBox3f ToFBox3f() const
	{
		return FBox3f(FVector3f(Min), FVector3f(Max));
	}

	FORCEINLINE bool IsValid() const
	{
		return Min.X < Max.X && Min.Y < Max.Y && Min.Z < Max.Z;
	}
 
	template<typename T>
	FORCEINLINE bool ContainsTemplate(T X, T Y, T Z) const
	{
		return ((X >= Min.X) && (X < Max.X) && (Y >= Min.Y) && (Y < Max.Y) && (Z >= Min.Z) && (Z < Max.Z));
	}
	template<typename T>
	FORCEINLINE typename TEnableIf<TOr<TIsSame<T, FVector3f>, TIsSame<T, FVector3d>, TIsSame<T, FIntVector>>::Value, bool>::Type ContainsTemplate(const T& V) const
	{
		return ContainsTemplate(V.X, V.Y, V.Z);
	}
	template<typename T>
	FORCEINLINE typename TEnableIf<TOr<TIsSame<T, FBox>, TIsSame<T, FVoxelIntBox>>::Value, bool>::Type ContainsTemplate(const T& Other) const
	{
		return Min.X <= Other.Min.X && Min.Y <= Other.Min.Y && Min.Z <= Other.Min.Z &&
			   Max.X >= Other.Max.X && Max.Y >= Other.Max.Y && Max.Z >= Other.Max.Z;
	}

	FORCEINLINE bool Contains(int32 X, int32 Y, int32 Z) const
	{
		return ContainsTemplate(X, Y, Z);
	}
	FORCEINLINE bool Contains(const FIntVector& V) const
	{
		return ContainsTemplate(V);
	}
	FORCEINLINE bool Contains(const FVoxelIntBox& Other) const
	{
		return ContainsTemplate(Other);
	}

	// Not an overload as the float behavior can be a bit tricky. Use ContainsTemplate if the input type is unknown
	FORCEINLINE bool ContainsFloat(float X, float Y, float Z) const
	{
		return ContainsTemplate(X, Y, Z);
	}
	FORCEINLINE bool ContainsFloat(const FVector3f& V) const
	{
		return ContainsTemplate(V);
	}
	FORCEINLINE bool ContainsFloat(const FVector3d& V) const
	{
		return ContainsTemplate(V);
	}
	FORCEINLINE bool ContainsFloat(const FBox& Other) const
	{
		return ContainsTemplate(Other);
	}
 
	template<typename T>
	bool Contains(T X, T Y, T Z) const = delete;
 
	template<typename T>
	FORCEINLINE T Clamp(T P, int32 Step = 1) const
	{
		Clamp(P.X, P.Y, P.Z, Step);
		return P;
	}
	FORCEINLINE void Clamp(int32& X, int32& Y, int32& Z, int32 Step = 1) const
	{
		X = FMath::Clamp(X, Min.X, Max.X - Step);
		Y = FMath::Clamp(Y, Min.Y, Max.Y - Step);
		Z = FMath::Clamp(Z, Min.Z, Max.Z - Step);
		ensureVoxelSlowNoSideEffects(Contains(X, Y, Z));
	}
	template<typename T>
	FORCEINLINE void Clamp(T& X, T& Y, T& Z) const
	{
		// Note: use - 1 even if that's not the closest value for which Contains would return true
		// because it's really hard to figure out that value (largest float f such that f < i)
		X = FMath::Clamp<T>(X, Min.X, Max.X - 1);
		Y = FMath::Clamp<T>(Y, Min.Y, Max.Y - 1);
		Z = FMath::Clamp<T>(Z, Min.Z, Max.Z - 1);
		ensureVoxelSlowNoSideEffects(ContainsTemplate(X, Y, Z));
	}
	FORCEINLINE FVoxelIntBox Clamp(const FVoxelIntBox& Other) const
	{
		// It's not valid to call Clamp if we're not intersecting Other
		ensureVoxelSlowNoSideEffects(Intersect(Other));

		FVoxelIntBox Result;
		
		Result.Min.X = FMath::Clamp(Other.Min.X, Min.X, Max.X - 1);
		Result.Min.Y = FMath::Clamp(Other.Min.Y, Min.Y, Max.Y - 1);
		Result.Min.Z = FMath::Clamp(Other.Min.Z, Min.Z, Max.Z - 1);

		Result.Max.X = FMath::Clamp(Other.Max.X, Min.X + 1, Max.X);
		Result.Max.Y = FMath::Clamp(Other.Max.Y, Min.Y + 1, Max.Y);
		Result.Max.Z = FMath::Clamp(Other.Max.Z, Min.Z + 1, Max.Z);
		
		ensureVoxelSlowNoSideEffects(Other.Contains(Result));
		return Result;
	}

	template<typename TBox>
	FORCEINLINE bool Intersect(const TBox& Other) const
	{
		if ((Min.X >= Other.Max.X) || (Other.Min.X >= Max.X))
		{
			return false;
		}

		if ((Min.Y >= Other.Max.Y) || (Other.Min.Y >= Max.Y))
		{
			return false;
		}

		if ((Min.Z >= Other.Max.Z) || (Other.Min.Z >= Max.Z))
		{
			return false;
		}

		return true;
	}
	/**
	 * Useful for templates taking a box or coordinates
	 */
	template<typename TNumeric>
	FORCEINLINE bool Intersect(TNumeric X, TNumeric Y, TNumeric Z) const
	{
		return ContainsTemplate(X, Y, Z);
	}
	
	/**
	 * Useful for templates taking a box or coordinates
	 */
	template<typename TNumeric, typename TVector>
	FORCEINLINE bool IntersectSphere(TVector Center, TNumeric Radius) const
	{
		// See FMath::SphereAABBIntersection
		return ComputeSquaredDistanceFromBoxToPoint<TNumeric>(Center) <= FMath::Square(Radius);
	}

	// Return the intersection of the two boxes
	FVoxelIntBox Overlap(const FVoxelIntBox& Other) const
	{
		if (!Intersect(Other))
		{
			return FVoxelIntBox();
		}

		// otherwise they overlap
		// so find overlapping box
		FIntVector MinVector, MaxVector;

		MinVector.X = FMath::Max(Min.X, Other.Min.X);
		MaxVector.X = FMath::Min(Max.X, Other.Max.X);

		MinVector.Y = FMath::Max(Min.Y, Other.Min.Y);
		MaxVector.Y = FMath::Min(Max.Y, Other.Max.Y);

		MinVector.Z = FMath::Max(Min.Z, Other.Min.Z);
		MaxVector.Z = FMath::Min(Max.Z, Other.Max.Z);

		return FVoxelIntBox(MinVector, MaxVector);
	}
	FVoxelIntBox Union(const FVoxelIntBox& Other) const
	{
		FIntVector MinVector, MaxVector;

		MinVector.X = FMath::Min(Min.X, Other.Min.X);
		MaxVector.X = FMath::Max(Max.X, Other.Max.X);

		MinVector.Y = FMath::Min(Min.Y, Other.Min.Y);
		MaxVector.Y = FMath::Max(Max.Y, Other.Max.Y);

		MinVector.Z = FMath::Min(Min.Z, Other.Min.Z);
		MaxVector.Z = FMath::Max(Max.Z, Other.Max.Z);

		return FVoxelIntBox(MinVector, MaxVector);
	}

	// union(return value, Other) = this
	TArray<FVoxelIntBox, TFixedAllocator<6>> Difference(const FVoxelIntBox& Other) const;

	template<typename TNumeric, typename TVector>
	FORCEINLINE TNumeric ComputeSquaredDistanceFromBoxToPoint(const TVector& Point) const
	{
		// Accumulates the distance as we iterate axis
		TNumeric DistSquared = 0;

		// Check each axis for min/max and add the distance accordingly
		if (Point.X < Min.X)
		{
			DistSquared += FMath::Square<TNumeric>(Min.X - Point.X);
		}
		else if (Point.X > Max.X)
		{
			DistSquared += FMath::Square<TNumeric>(Point.X - Max.X);
		}

		if (Point.Y < Min.Y)
		{
			DistSquared += FMath::Square<TNumeric>(Min.Y - Point.Y);
		}
		else if (Point.Y > Max.Y)
		{
			DistSquared += FMath::Square<TNumeric>(Point.Y - Max.Y);
		}

		if (Point.Z < Min.Z)
		{
			DistSquared += FMath::Square<TNumeric>(Min.Z - Point.Z);
		}
		else if (Point.Z > Max.Z)
		{
			DistSquared += FMath::Square<TNumeric>(Point.Z - Max.Z);
		}

		return DistSquared;
	}
	FORCEINLINE uint64 ComputeSquaredDistanceFromBoxToPoint(const FIntVector& Point) const
	{
		return ComputeSquaredDistanceFromBoxToPoint<uint64>(Point);
	}

	// We try to make the following true:
	// Box.ApproximateDistanceToBox(Box.ShiftBy(X)) == X
	float ApproximateDistanceToBox(const FVoxelIntBox& Other) const
	{
		return (FVoxelUtilities::Size(Min - Other.Min) + FVoxelUtilities::Size(Max - Other.Max)) / 2;
	}

	FORCEINLINE bool IsMultipleOf(int32 Step) const
	{
		return Min.X % Step == 0 && Min.Y % Step == 0 && Min.Z % Step == 0 &&
			   Max.X % Step == 0 && Max.Y % Step == 0 && Max.Z % Step == 0;
	}
	
	// OldBox included in NewBox, but NewBox not included in OldBox
	FORCEINLINE FVoxelIntBox MakeMultipleOfBigger(int32 Step) const
	{
		FVoxelIntBox NewBox;
		NewBox.Min = FVoxelUtilities::DivideFloor(Min, Step) * Step;
		NewBox.Max = FVoxelUtilities::DivideCeil(Max, Step) * Step;
		return NewBox;
	}
	// NewBox included in OldBox, but OldBox not included in NewBox
	FORCEINLINE FVoxelIntBox MakeMultipleOfSmaller(int32 Step) const
	{
		FVoxelIntBox NewBox;
		NewBox.Min = FVoxelUtilities::DivideCeil(Min, Step) * Step;		
		NewBox.Max = FVoxelUtilities::DivideFloor(Max, Step) * Step;
		return NewBox;
	}
	FORCEINLINE FVoxelIntBox MakeMultipleOfRoundUp(int32 Step) const
	{
		FVoxelIntBox NewBox;
		NewBox.Min = FVoxelUtilities::DivideCeil(Min, Step) * Step;		
		NewBox.Max = FVoxelUtilities::DivideCeil(Max, Step) * Step;
		return NewBox;
	}
	
	FORCEINLINE FVoxelIntBox DivideBigger(int32 Step) const
	{
		FVoxelIntBox NewBox;
		NewBox.Min = FVoxelUtilities::DivideFloor(Min, Step);
		NewBox.Max = FVoxelUtilities::DivideCeil(Max, Step);
		return NewBox;
	}

	// Guarantee: union(OutChilds).Contains(this)
	template<typename T>
	bool Subdivide(int32 ChildrenSize, TArray<FVoxelIntBox, T>& OutChildren, bool bUseOverlap, int32 MaxChildren = -1) const
	{
		OutChildren.Reset();
		
		const FIntVector LowerBound = FVoxelUtilities::DivideFloor(Min, ChildrenSize) * ChildrenSize;
		const FIntVector UpperBound = FVoxelUtilities::DivideCeil(Max, ChildrenSize) * ChildrenSize;

		const FIntVector EstimatedSize = (UpperBound - LowerBound) / ChildrenSize;
		OutChildren.Reserve(EstimatedSize.X * EstimatedSize.Y * EstimatedSize.Z);
		
		for (int32 X = LowerBound.X; X < UpperBound.X; X += ChildrenSize)
		{
			for (int32 Y = LowerBound.Y; Y < UpperBound.Y; Y += ChildrenSize)
			{
				for (int32 Z = LowerBound.Z; Z < UpperBound.Z; Z += ChildrenSize)
				{
					FVoxelIntBox Child(FIntVector(X, Y, Z), FIntVector(X + ChildrenSize, Y + ChildrenSize, Z + ChildrenSize));
					if (bUseOverlap)
					{
						Child = Child.Overlap(*this);
					}
					OutChildren.Add(Child);
					
					if (MaxChildren != -1 && OutChildren.Num() > MaxChildren)
					{
						return false;
					}
				}
			}
		}
		return true;
	}

	FORCEINLINE FVoxelIntBox Scale(float S) const
	{
		return { FVoxelUtilities::FloorToInt(FVector(Min) * S), FVoxelUtilities::CeilToInt(FVector(Max) * S) };
	}
	FORCEINLINE FVoxelIntBox Scale(int32 S) const
	{
		return { Min * S, Max * S };
	}
	FORCEINLINE FVoxelIntBox Scale(const FVector& S) const
	{
		return SafeConstruct(FVector(Min) * S, FVector(Max) * S);
	}
	
	FORCEINLINE FVoxelIntBox Extend(const FIntVector& Amount) const
	{
		return { Min - Amount, Max + Amount };
	}
	FORCEINLINE FVoxelIntBox Extend(int32 Amount) const
	{
		return Extend(FIntVector(Amount));
	}
	FORCEINLINE FVoxelIntBox Translate(const FIntVector& Position) const
	{
		return FVoxelIntBox(Min + Position, Max + Position);
	}
	FORCEINLINE FVoxelIntBox ShiftBy(const FIntVector& Offset) const
	{
		return Translate(Offset);
	}
	
	FORCEINLINE FVoxelIntBox RemoveTranslation() const
	{
		return FVoxelIntBox(0, Max - Min);
	}
	// Will move the box so that GetCenter = 0,0,0. Will extend it if its size is odd
	FVoxelIntBox Center() const
	{
		FIntVector NewMin = Min;
		FIntVector NewMax = Max;
		if (FVector(FIntVector(GetCenter())) != GetCenter())
		{
			NewMax = NewMax + 1;
		}
		ensure(FVector(FIntVector(GetCenter())) == GetCenter());
		const FIntVector Offset = FIntVector(GetCenter());
		NewMin -= Offset;
		NewMax -= Offset;
		ensure(NewMin + NewMax == FIntVector(0));
		return FVoxelIntBox(NewMin, NewMax);
	}

	FORCEINLINE FVoxelIntBox& operator*=(int32 Scale)
	{
		Min *= Scale;
		Max *= Scale;
		return *this;
	}

	FORCEINLINE bool operator==(const FVoxelIntBox& Other) const
	{
		return Min == Other.Min && Max == Other.Max;
	}
	FORCEINLINE bool operator!=(const FVoxelIntBox& Other) const
	{
		return Min != Other.Min || Max != Other.Max;
	}

	FORCEINLINE uint32 GetHash() const
	{
		return FVoxelUtilities::FastIntVectorHash_Low(Min) ^ FVoxelUtilities::FastIntVectorHash_High(Max);
	}
	
	template<typename T, typename TCond>
	FORCEINLINE void Iterate(int32 Step, T Lambda, TCond Condition) const
	{
		for (int32 X = Min.X; X < Max.X; X += Step)
		{
			for (int32 Y = Min.Y; Y < Max.Y; Y += Step)
			{
				for (int32 Z = Min.Z; Z < Max.Z; Z += Step)
				{
					Lambda(FIntVector(X, Y, Z));
					if (!Condition()) return;
				}
			}
		}
	}
	
	template<typename T>
	FORCEINLINE void Iterate(int32 Step, T Lambda) const
	{
		Iterate(Step, Lambda, []() { return true; });
	}
	template<typename T>
	FORCEINLINE void Iterate(T Lambda) const
	{
		Iterate(1, Lambda);
	}

	// Note: it's often faster to iterate the full chunk & check for distance on each voxel than use this!
	template<typename T>
	void IterateSphere(FVector Center, float Radius, T Lambda) const
	{
		const int32 ItMinX = FMath::Max(Min.X, FMath::FloorToInt(Center.X - Radius));
		const int32 ItMaxX = FMath::Min(Max.X - 1, FMath::CeilToInt(Center.X + Radius));

		const float SquaredRadius = FMath::Square(Radius);

		for (int32 X = ItMinX; X <= ItMaxX; X++)
		{
			const float DeltaX = X - Center.X;
			const float DeltaY = FMath::Sqrt(FMath::Max(0.f, SquaredRadius - FMath::Square(DeltaX)));

			const int32 ItMinY = FMath::Max(Min.Y, FMath::FloorToInt(Center.Y - DeltaY));
			const int32 ItMaxY = FMath::Min(Max.Y - 1, FMath::CeilToInt(Center.Y + DeltaY));

			for (int32 Y = ItMinY; Y <= ItMaxY; Y++)
			{
				const float DeltaZ = FMath::Sqrt(FMath::Max(0.f, SquaredRadius - FMath::Square(DeltaX) - FMath::Square(Y - Center.Y)));

				const int32 MinZ = FMath::Max(Min.Z, FMath::FloorToInt(Center.Z - DeltaZ));
				const int32 MaxZ = FMath::Min(Max.Z - 1, FMath::CeilToInt(Center.Z + DeltaZ));

				for (int32 Z = MinZ; Z <= MaxZ; Z++)
				{
					const FIntVector Position(X, Y, Z);
					const float SquaredDistance = (FVector(Position) - Center).SizeSquared();
					if (SquaredDistance <= SquaredRadius)
					{
						Lambda(Position, SquaredDistance);
					}
				}
			}
		}
	}
	
	template<typename T>
	void ParallelIterateSphere(FVector Center, float Radius, T Lambda, bool bForceSingleThread = false) const
	{
		if (bForceSingleThread)
		{
			IterateSphere(Center, Radius, Lambda);
		}
		else
		{
			const int32 ItMinX = FMath::Max(Min.X, FMath::FloorToInt(Center.X - Radius));
			const int32 ItMaxX = FMath::Min(Max.X - 1, FMath::CeilToInt(Center.X + Radius));
			
			ParallelFor(ItMaxX - ItMinX + 1, [&](int32 X)
			{
				X += ItMinX;
				
				const float SquaredRadius = FMath::Square(Radius);

				const float DeltaX = X - Center.X;
				const float DeltaY = FMath::Sqrt(FMath::Max(0.f, SquaredRadius - FMath::Square(DeltaX)));

				const int32 ItMinY = FMath::Max(Min.Y, FMath::FloorToInt(Center.Y - DeltaY));
				const int32 ItMaxY = FMath::Min(Max.Y - 1, FMath::CeilToInt(Center.Y + DeltaY));

				for (int32 Y = ItMinY; Y <= ItMaxY; Y++)
				{
					const float DeltaZ = FMath::Sqrt(FMath::Max(0.f, SquaredRadius - FMath::Square(DeltaX) - FMath::Square(Y - Center.Y)));

					const int32 ItMinZ = FMath::Max(Min.Z, FMath::FloorToInt(Center.Z - DeltaZ));
					const int32 ItMaxZ = FMath::Min(Max.Z - 1, FMath::CeilToInt(Center.Z + DeltaZ));

					for (int32 Z = ItMinZ; Z <= ItMaxZ; Z++)
					{
						const FIntVector Position(X, Y, Z);
						const float SquaredDistance = (FVector(Position) - Center).SizeSquared();
						if (SquaredDistance <= SquaredRadius)
						{
							Lambda(Position, SquaredDistance);
						}
					}
				}
			});
		}
	}

	// MaxBorderSize: if we do a 180 rotation for example, min and max are inverted
	// If we don't work on values that are actually inside the box, the resulting box will be wrong
	FVoxelIntBox ApplyTransform(const FMatrix44f& Transform, int32 MaxBorderSize = 1) const
	{
		return ApplyTransformImpl([&](const FIntVector& Position)
		{
			return Transform.TransformPosition(FVector3f(Position));
		}, MaxBorderSize);
	}
	FVoxelIntBox ApplyTransform(const FTransform3f& Transform, int32 MaxBorderSize = 1) const
	{
		return ApplyTransformImpl([&](const FIntVector& Position)
		{
			return Transform.TransformPosition(FVector3f(Position));
		}, MaxBorderSize);
	}

	FVoxelIntBox ApplyTransform(const FMatrix44d& Transform, int32 MaxBorderSize = 1) const
	{
		return ApplyTransformImpl([&](const FIntVector& Position)
		{
			return Transform.TransformPosition(FVector3d(Position));
		}, MaxBorderSize);
	}
	FVoxelIntBox ApplyTransform(const FTransform3d& Transform, int32 MaxBorderSize = 1) const
	{
		return ApplyTransformImpl([&](const FIntVector& Position)
		{
			return Transform.TransformPosition(FVector3d(Position));
		}, MaxBorderSize);
	}

	template<typename T>
	FVoxelIntBox ApplyTransformImpl(T GetNewPosition, int32 MaxBorderSize = 1) const
	{
		const auto Corners = GetCorners(MaxBorderSize);

		FIntVector NewMin(MAX_int32);
		FIntVector NewMax(MIN_int32);
		for (int32 Index = 0; Index < 8; Index++)
		{
			const FVector P = FVector(GetNewPosition(Corners[Index]));
			NewMin = FVoxelUtilities::ComponentMin(NewMin, FVoxelUtilities::FloorToInt(P));
			NewMax = FVoxelUtilities::ComponentMax(NewMax, FVoxelUtilities::CeilToInt(P));
		}
		return FVoxelIntBox(NewMin, NewMax + MaxBorderSize);
	}
	template<typename T>
	FBox ApplyTransformFloatImpl(T GetNewPosition, int32 MaxBorderSize = 1) const
	{
		const auto Corners = GetCorners(MaxBorderSize);
		
		FVector NewMin = GetNewPosition(Corners[0]);
		FVector NewMax = NewMin;
		for (int32 Index = 1; Index < 8; Index++)
		{
			const FVector P = GetNewPosition(Corners[Index]);
			NewMin = FVoxelUtilities::ComponentMin(NewMin, P);
			NewMax = FVoxelUtilities::ComponentMax(NewMax, P);
		}
		return FBox(NewMin, NewMax + MaxBorderSize);
	}

	FORCEINLINE FVoxelIntBox& operator+=(const FVoxelIntBox& Other)
	{
		Min = FVoxelUtilities::ComponentMin(Min, Other.Min);
		Max = FVoxelUtilities::ComponentMax(Max, Other.Max);
		return *this;
	}
	FORCEINLINE FVoxelIntBox& operator+=(const FIntVector& Point)
	{
		Min = FVoxelUtilities::ComponentMin(Min, Point);
		Max = FVoxelUtilities::ComponentMax(Max, Point + 1);
		return *this;
	}
	FORCEINLINE FVoxelIntBox& operator+=(const FVector3f& Point)
	{
		Min = FVoxelUtilities::ComponentMin(Min, FVoxelUtilities::FloorToInt(Point));
		Max = FVoxelUtilities::ComponentMax(Max, FVoxelUtilities::CeilToInt(Point) + 1);
		return *this;
	}
	FORCEINLINE FVoxelIntBox& operator+=(const FVector3d& Point)
	{
		Min = FVoxelUtilities::ComponentMin(Min, FVoxelUtilities::FloorToInt(Point));
		Max = FVoxelUtilities::ComponentMax(Max, FVoxelUtilities::CeilToInt(Point) + 1);
		return *this;
	}
};

FORCEINLINE uint32 GetTypeHash(const FVoxelIntBox& Box)
{
	return Box.GetHash();
}

FORCEINLINE FVoxelIntBox operator*(const FVoxelIntBox& Box, int32 Scale)
{
	FVoxelIntBox Copy = Box;
	return Copy *= Scale;
}

FORCEINLINE FVoxelIntBox operator*(int32 Scale, const FVoxelIntBox& Box)
{
	FVoxelIntBox Copy = Box;
	return Copy *= Scale;
}

FORCEINLINE FVoxelIntBox operator+(const FVoxelIntBox& Box, const FVoxelIntBox& Other)
{
	return FVoxelIntBox(Box) += Other;
}

FORCEINLINE FVoxelIntBox operator+(const FVoxelIntBox& Box, const FIntVector& Point)
{
	return FVoxelIntBox(Box) += Point;
}

FORCEINLINE FVoxelIntBox operator+(const FVoxelIntBox& Box, const FVector3f& Point)
{
	return Box + FVoxelIntBox(Point);
}

FORCEINLINE FVoxelIntBox operator+(const FVoxelIntBox& Box, const FVector3d& Point)
{
	return Box + FVoxelIntBox(Point);
}

FORCEINLINE FArchive& operator<<(FArchive& Ar, FVoxelIntBox& Box)
{
	Ar << Box.Min;
	Ar << Box.Max;
	return Ar;
}

// Voxel Int Box with a IsValid flag
struct FVoxelOptionalIntBox
{
	FVoxelOptionalIntBox() = default;
	FVoxelOptionalIntBox(const FVoxelIntBox& Box)
		: Box(Box)
		, bValid(true)
	{
	}

	FORCEINLINE const FVoxelIntBox& GetBox() const
	{
		check(IsValid());
		return Box;
	}

	FORCEINLINE bool IsValid() const
	{
		return bValid;
	}
	FORCEINLINE void Reset()
	{
		bValid = false;
	}

	FORCEINLINE FVoxelOptionalIntBox& operator=(const FVoxelIntBox& Other)
	{
		Box = Other;
		bValid = true;
		return *this;
	}
	
	FORCEINLINE bool operator==(const FVoxelOptionalIntBox& Other) const
	{
		if (bValid != Other.bValid)
		{
			return false;
		}
		if (!bValid && !Other.bValid)
		{
			return true;
		}
		return Box == Other.Box;
	}
	FORCEINLINE bool operator!=(const FVoxelOptionalIntBox& Other) const
	{
		return !(*this == Other);
	}

	FORCEINLINE FVoxelOptionalIntBox& operator+=(const FVoxelIntBox& Other)
	{
		if (bValid)
		{
			Box += Other;
		}
		else
		{
			Box = Other;
			bValid = true;
		}
		return *this;
	}
	FORCEINLINE FVoxelOptionalIntBox& operator+=(const FVoxelOptionalIntBox& Other)
	{
		if (Other.bValid)
		{
			*this += Other.GetBox();
		}
		return *this;
	}

	FORCEINLINE FVoxelOptionalIntBox& operator+=(const FIntVector& Point)
	{
		if (bValid)
		{
			Box += Point;
		}
		else
		{
			Box = FVoxelIntBox(Point);
			bValid = true;
		}
		return *this;
	}

	FORCEINLINE FVoxelOptionalIntBox& operator+=(const FVector3f& Point)
	{
		if (bValid)
		{
			Box += Point;
		}
		else
		{
			Box = FVoxelIntBox(Point);
			bValid = true;
		}
		return *this;
	}
	
	FORCEINLINE FVoxelOptionalIntBox& operator+=(const FVector3d& Point)
	{
		if (bValid)
		{
			Box += Point;
		}
		else
		{
			Box = FVoxelIntBox(Point);
			bValid = true;
		}
		return *this;
	}

	template<typename T>
	FORCEINLINE FVoxelOptionalIntBox& operator+=(const TArray<T>& Other)
	{
		for (auto& It : Other)
		{
			*this += It;
		}
		return *this;
	}
	
private:
	FVoxelIntBox Box;
	bool bValid = false;
};

template<typename T>
FORCEINLINE FVoxelOptionalIntBox operator+(const FVoxelOptionalIntBox& Box, const T& Other)
{
	FVoxelOptionalIntBox Copy = Box;
	return Copy += Other;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Manually defining the loop is much faster than passing a lambda to Bounds.Iterate

#define HELPER_VOXEL_INT_BOX_ITERATE_START_STEP(Bounds, Var, Step, Name) int32 Name = (Bounds).Min.Var; Name < (Bounds).Max.Var; Name += Step

#define VOXEL_INT_BOX_ITERATE_START_STEP(Bounds, Position, Step) \
		{ \
			const FVoxelIntBox PREPROCESSOR_JOIN(__Bounds, __LINE__) = Bounds; \
			for (HELPER_VOXEL_INT_BOX_ITERATE_START_STEP(PREPROCESSOR_JOIN(__Bounds, __LINE__), Z, Step, PREPROCESSOR_JOIN(__Z, __LINE__))) { \
			for (HELPER_VOXEL_INT_BOX_ITERATE_START_STEP(PREPROCESSOR_JOIN(__Bounds, __LINE__), Y, Step, PREPROCESSOR_JOIN(__Y, __LINE__))) { \
			for (HELPER_VOXEL_INT_BOX_ITERATE_START_STEP(PREPROCESSOR_JOIN(__Bounds, __LINE__), X, Step, PREPROCESSOR_JOIN(__X, __LINE__))) { \
				const FIntVector Position(PREPROCESSOR_JOIN(__X, __LINE__), PREPROCESSOR_JOIN(__Y, __LINE__), PREPROCESSOR_JOIN(__Z, __LINE__)); \
				INTELLISENSE_ONLY([]() {};) // Just to confuse resharper & remove the "can be moved to inner scope" warning

#define VOXEL_INT_BOX_ITERATE_START(Bounds, Position) VOXEL_INT_BOX_ITERATE_START_STEP(Bounds, Position, 1)

#define VOXEL_INT_BOX_ITERATE_END }}}}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define CHECK_VOXEL_INT_BOX_IMPL(Bounds, ReturnValue) \
	if (!(Bounds).IsValid()) \
	{ \
		VOXEL_FUNCTION_MESSAGE(Error, TEXT("Invalid Bounds! {0}"), (Bounds).ToString()); \
		return ReturnValue; \
	}

#define CHECK_VOXEL_INT_BOX(Bounds) CHECK_VOXEL_INT_BOX_IMPL(Bounds, {});
#define CHECK_VOXEL_INT_BOX_VOID(Bounds) CHECK_VOXEL_INT_BOX_IMPL(Bounds,);