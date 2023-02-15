// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "VoxelMinimal/VoxelBox.h"
#include "VoxelMinimal/Containers/VoxelArrayView.h"
#include "VoxelMinimal/Utilities/VoxelBaseUtilities.h"
#include "VoxelMinimal/Utilities/VoxelVectorUtilities.h"

struct VOXELCORE_API FVoxelBox2D
{
	FVector2d Min = FVector2d(ForceInit);
	FVector2d Max = FVector2d(ForceInit);

	static const FVoxelBox2D Infinite;

	FVoxelBox2D() = default;

	FORCEINLINE FVoxelBox2D(const FVector2d& Min, const FVector2d& Max)
		: Min(Min)
		, Max(Max)
	{
		ensureVoxelSlow(Min.X <= Max.X);
		ensureVoxelSlow(Min.Y <= Max.Y);
	}
	FORCEINLINE explicit FVoxelBox2D(double Min, const FVector2d& Max)
		: FVoxelBox2D(FVector2d(Min), Max)
	{
	}
	FORCEINLINE explicit FVoxelBox2D(const FVector2d& Min, double Max)
		: FVoxelBox2D(Min, FVector2d(Max))
	{
	}
	FORCEINLINE explicit FVoxelBox2D(const FVector2f& Min, const FVector2f& Max)
		: FVoxelBox2D(FVector2d(Min), FVector2d(Max))
	{
	}
	FORCEINLINE explicit FVoxelBox2D(const FIntPoint& Min, const FIntPoint& Max)
		: FVoxelBox2D(FVector2d(Min), FVector2d(Max))
	{
	}
	FORCEINLINE explicit FVoxelBox2D(double Min, double Max)
		: FVoxelBox2D(FVector2d(Min), FVector2d(Max))
	{
	}

	FORCEINLINE explicit FVoxelBox2D(const FVector2f& Position)
		: FVoxelBox2D(Position, Position)
	{
	}
	FORCEINLINE explicit FVoxelBox2D(const FVector2d& Position)
		: FVoxelBox2D(Position, Position)
	{
	}
	FORCEINLINE explicit FVoxelBox2D(const FIntPoint& Position)
		: FVoxelBox2D(Position, Position)
	{
	}

	FORCEINLINE explicit FVoxelBox2D(const FBox2f& Box)
		: FVoxelBox2D(Box.Min, Box.Max)
	{
		ensureVoxelSlow(Box.bIsValid);
	}
	FORCEINLINE explicit FVoxelBox2D(const FBox2d& Box)
		: FVoxelBox2D(Box.Min, Box.Max)
	{
		ensureVoxelSlow(Box.bIsValid);
	}

	template<typename T>
	explicit FVoxelBox2D(const TArray<T>& Data)
	{
		if (!ensure(Data.Num() > 0))
		{
			Min = Max = FVector2d::ZeroVector;
			return;
		}

		*this = FVoxelBox2D(Data[0]);
		for (int32 Index = 1; Index < Data.Num(); Index++)
		{
			*this = *this + Data[Index];
		}
	}

	static FVoxelBox2D FromPositions(TConstVoxelArrayView<FIntPoint> Positions);
	static FVoxelBox2D FromPositions(TConstVoxelArrayView<FVector2f> Positions);
	static FVoxelBox2D FromPositions(TConstVoxelArrayView<FVector2d> Positions);

	static FVoxelBox2D FromPositions(
		TConstVoxelArrayView<float> PositionX,
		TConstVoxelArrayView<float> PositionY);

	FORCEINLINE FVector2d Size() const
	{
		return Max - Min;
	}
	FORCEINLINE FVector2d GetCenter() const
	{
		return (Min + Max) / 2;
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("(%f/%f, %f/%f)"), Min.X, Max.X, Min.Y, Max.Y);
	}
	FBox2D ToFBox() const
	{
		return FBox2D(FVector2D(Min), FVector2D(Max));
	}
	FBox2f ToFBox2f() const
	{
		return FBox2f(FVector2f(Min), FVector2f(Max));
	}
	FVoxelBox ToBox3D(double MinZ, double MaxZ) const
	{
		return FVoxelBox(
			FVector3d(Min.X, Min.Y, MinZ),
			FVector3d(Max.X, Max.Y, MaxZ));
	}

	FORCEINLINE bool IsValid() const
	{
		return Min.X < Max.X && Min.Y < Max.Y;
	}

	FORCEINLINE bool Contains(double X, double Y) const
	{
		return
			(Min.X <= X) && (X <= Max.X) &&
			(Min.Y <= Y) && (Y <= Max.Y);
	}
	FORCEINLINE bool IsInfinite() const
	{
		return Contains(Infinite.Extend(-1000));
	}

	FORCEINLINE bool Contains(const FIntPoint& Vector) const
	{
		return Contains(Vector.X, Vector.Y);
	}
	FORCEINLINE bool Contains(const FVector2f& Vector) const
	{
		return Contains(Vector.X, Vector.Y);
	}
	FORCEINLINE bool Contains(const FVector2d& Vector) const
	{
		return Contains(Vector.X, Vector.Y);
	}

	FORCEINLINE bool Contains(const FVoxelBox2D& Other) const
	{
		return
			(Min.X <= Other.Min.X) && (Other.Max.X <= Max.X) &&
			(Min.Y <= Other.Min.Y) && (Other.Max.Y <= Max.Y);
	}

	FORCEINLINE bool Intersect(const FVoxelBox2D& Other) const
	{
		if (Min.X >= Other.Max.X || Other.Min.X >= Max.X)
		{
			return false;
		}

		if (Min.Y >= Other.Max.Y || Other.Min.Y >= Max.Y)
		{
			return false;
		}

		return true;
	}

	// Return the intersection of the two boxes
	FVoxelBox2D Overlap(const FVoxelBox2D& Other) const
	{
		if (!Intersect(Other))
		{
			return {};
		}

		FVector2d NewMin;
		FVector2d NewMax;

		NewMin.X = FMath::Max(Min.X, Other.Min.X);
		NewMax.X = FMath::Min(Max.X, Other.Max.X);

		NewMin.Y = FMath::Max(Min.Y, Other.Min.Y);
		NewMax.Y = FMath::Min(Max.Y, Other.Max.Y);

		return FVoxelBox2D(NewMin, NewMax);
	}
	FVoxelBox2D Union(const FVoxelBox2D& Other) const
	{
		FVector2d NewMin;
		FVector2d NewMax;

		NewMin.X = FMath::Min(Min.X, Other.Min.X);
		NewMax.X = FMath::Max(Max.X, Other.Max.X);

		NewMin.Y = FMath::Min(Min.Y, Other.Min.Y);
		NewMax.Y = FMath::Max(Max.Y, Other.Max.Y);

		return FVoxelBox2D(NewMin, NewMax);
	}

	FORCEINLINE double ComputeSquaredDistanceFromBoxToPoint(const FVector2d& Point) const
	{
		// Accumulates the distance as we iterate axis
		double DistSquared = 0;

		// Check each axis for min/max and add the distance accordingly
		if (Point.X < Min.X)
		{
			DistSquared += FMath::Square<double>(Min.X - Point.X);
		}
		else if (Point.X > Max.X)
		{
			DistSquared += FMath::Square<double>(Point.X - Max.X);
		}

		if (Point.Y < Min.Y)
		{
			DistSquared += FMath::Square<double>(Min.Y - Point.Y);
		}
		else if (Point.Y > Max.Y)
		{
			DistSquared += FMath::Square<double>(Point.Y - Max.Y);
		}

		return DistSquared;
	}
	FORCEINLINE double DistanceFromBoxToPoint(const FVector2d& Point) const
	{
		return FMath::Sqrt(ComputeSquaredDistanceFromBoxToPoint(Point));
	}

	FORCEINLINE FVoxelBox2D Scale(double S) const
	{
		return { Min * S, Max * S };
	}
	FORCEINLINE FVoxelBox2D Scale(const FVector2D& S) const
	{
		return { Min * S, Max * S };
	}
	FORCEINLINE FVoxelBox2D Extend(double Amount) const
	{
		return { Min - Amount, Max + Amount };
	}
	FORCEINLINE FVoxelBox2D Translate(const FVector2d& Position) const
	{
		return FVoxelBox2D(Min + Position, Max + Position);
	}
	FORCEINLINE FVoxelBox2D ShiftBy(const FVector2d& Offset) const
	{
		return Translate(Offset);
	}

	FORCEINLINE FVoxelBox2D& operator*=(double Scale)
	{
		Min *= Scale;
		Max *= Scale;
		return *this;
	}
	FORCEINLINE FVoxelBox2D& operator/=(double Scale)
	{
		Min /= Scale;
		Max /= Scale;
		return *this;
	}

	FORCEINLINE bool operator==(const FVoxelBox2D& Other) const
	{
		return Min == Other.Min && Max == Other.Max;
	}
	FORCEINLINE bool operator!=(const FVoxelBox2D& Other) const
	{
		return Min != Other.Min || Max != Other.Max;
	}

	template<typename MatrixType>
	FVoxelBox2D TransformByImpl(const MatrixType& Transform) const
	{
		FVector2d Vertices[4] =
		{
			FVector2d(Min.X, Min.Y),
			FVector2d(Max.X, Min.Y),
			FVector2d(Min.X, Max.Y),
			FVector2d(Max.X, Max.Y),
		};

		for (int32 Index = 0; Index < 4; Index++)
		{
			Vertices[Index] = FVector2d(Transform.TransformPoint(FVector2D(Vertices[Index])));
		}

		FVoxelBox2D NewBox;
		NewBox.Min = Vertices[0];
		NewBox.Max = Vertices[0];

		for (int32 Index = 1; Index < 4; Index++)
		{
			NewBox.Min = FVoxelUtilities::ComponentMin(NewBox.Min, Vertices[Index]);
			NewBox.Max = FVoxelUtilities::ComponentMax(NewBox.Max, Vertices[Index]);
		}

		return NewBox;
	}
	
	FVoxelBox2D TransformBy(const FQuat2D& Transform) const
	{
		return TransformByImpl(Transform);
	}
	FVoxelBox2D TransformBy(const FMatrix2x2& Transform) const
	{
		return TransformByImpl(Transform);
	}
	FVoxelBox2D TransformBy(const FTransform2D& Transform) const
	{
		return TransformByImpl(Transform);
	}

	FORCEINLINE FVoxelBox2D& operator+=(const FVoxelBox2D& Other)
	{
		Min = FVoxelUtilities::ComponentMin(Min, Other.Min);
		Max = FVoxelUtilities::ComponentMax(Max, Other.Max);
		return *this;
	}
	FORCEINLINE FVoxelBox2D& operator+=(const FVector2d& Point)
	{
		Min = FVoxelUtilities::ComponentMin(Min, Point);
		Max = FVoxelUtilities::ComponentMax(Max, Point);
		return *this;
	}
	FORCEINLINE FVoxelBox2D& operator+=(const FVector2f& Point)
	{
		return operator+=(FVector2d(Point));
	}
};

FORCEINLINE uint32 GetTypeHash(const FVoxelBox2D& Box)
{
	return FVoxelUtilities::MurmurHash(Box);
}

FORCEINLINE FVoxelBox2D operator*(const FVoxelBox2D& Box, double Scale)
{
	return FVoxelBox2D(Box) *= Scale;
}
FORCEINLINE FVoxelBox2D operator*(double Scale, const FVoxelBox2D& Box)
{
	return FVoxelBox2D(Box) *= Scale;
}

FORCEINLINE FVoxelBox2D operator/(const FVoxelBox2D& Box, double Scale)
{
	return FVoxelBox2D(Box) /= Scale;
}

FORCEINLINE FVoxelBox2D operator+(const FVoxelBox2D& Box, const FVoxelBox2D& Other)
{
	return FVoxelBox2D(Box) += Other;
}
FORCEINLINE FVoxelBox2D operator+(const FVoxelBox2D& Box, const FVector2f& Point)
{
	return FVoxelBox2D(Box) += Point;
}
FORCEINLINE FVoxelBox2D operator+(const FVoxelBox2D& Box, const FVector2d& Point)
{
	return FVoxelBox2D(Box) += Point;
}

FORCEINLINE FArchive& operator<<(FArchive& Ar, FVoxelBox2D& Box)
{
	Ar << Box.Min;
	Ar << Box.Max;
	return Ar;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct FVoxelOptionalBox2D
{
	FVoxelOptionalBox2D() = default;
	FVoxelOptionalBox2D(const FVoxelBox2D& Box)
		: Box(Box)
		, bValid(true)
	{
	}

	FORCEINLINE const FVoxelBox2D& GetBox() const
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

	FORCEINLINE FVoxelOptionalBox2D& operator=(const FVoxelBox2D& Other)
	{
		Box = Other;
		bValid = true;
		return *this;
	}
	
	FORCEINLINE bool operator==(const FVoxelOptionalBox2D& Other) const
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
	FORCEINLINE bool operator!=(const FVoxelOptionalBox2D& Other) const
	{
		return !(*this == Other);
	}

	FORCEINLINE FVoxelOptionalBox2D& operator+=(const FVoxelBox2D& Other)
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
	FORCEINLINE FVoxelOptionalBox2D& operator+=(const FVoxelOptionalBox2D& Other)
	{
		if (Other.bValid)
		{
			*this += Other.GetBox();
		}
		return *this;
	}

	FORCEINLINE FVoxelOptionalBox2D& operator+=(const FVector2f& Point)
	{
		if (bValid)
		{
			Box += Point;
		}
		else
		{
			Box = FVoxelBox2D(Point);
			bValid = true;
		}
		return *this;
	}
	
	FORCEINLINE FVoxelOptionalBox2D& operator+=(const FVector2d& Point)
	{
		if (bValid)
		{
			Box += Point;
		}
		else
		{
			Box = FVoxelBox2D(Point);
			bValid = true;
		}
		return *this;
	}

	template<typename T>
	FORCEINLINE FVoxelOptionalBox2D& operator+=(const TArray<T>& Other)
	{
		for (auto& It : Other)
		{
			*this += It;
		}
		return *this;
	}
	
private:
	FVoxelBox2D Box;
	bool bValid = false;
};

template<typename T>
FORCEINLINE FVoxelOptionalBox2D operator+(const FVoxelOptionalBox2D& Box, const T& Other)
{
	FVoxelOptionalBox2D Copy = Box;
	return Copy += Other;
}