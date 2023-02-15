// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMetaGraphPreviewActor.h"

AVoxelMetaGraphPreviewActor::AVoxelMetaGraphPreviewActor()
{
	BoxComponent = CreateDefaultSubobject<UBoxComponent>("Box");
	BoxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoxComponent->SetCanEverAffectNavigation(false);

	RootComponent = BoxComponent;

	BoxComponent->SetBoxExtent(FVector(1000, 1000, 1));

#if WITH_EDITOR
	PrimaryActorTick.bCanEverTick = true;
#endif
}

#if WITH_EDITOR
void AVoxelMetaGraphPreviewActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	BoxComponent->SetRelativeScale3D(FVector(BoxComponent->GetRelativeScale3D().Z));

	const FMatrix Matrix = INLINE_LAMBDA -> FMatrix
	{
		const FVector X = FVector::UnitX();
		const FVector Y = FVector::UnitY();
		const FVector Z = FVector::UnitZ();

		switch (Axis)
		{
		default: ensure(false);
		case EVoxelAxis::X: return FMatrix(Y, Z, X, FVector::ZeroVector);
		case EVoxelAxis::Y: return FMatrix(X, Z, -Y, FVector::ZeroVector);
		case EVoxelAxis::Z: return FMatrix(X, Y, Z, FVector::ZeroVector);
		}
	};

	BoxComponent->SetWorldRotation(Matrix.Rotator());
}

void AVoxelMetaGraphPreviewActor::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	OnChanged.Broadcast();
}

void AVoxelMetaGraphPreviewActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	OnChanged.Broadcast();
}

float AVoxelMetaGraphPreviewActor::GetAxisLocation() const
{
	switch (Axis)
	{
	default: return 0.f;
	case EVoxelAxis::X: return GetActorLocation().X;
	case EVoxelAxis::Y: return GetActorLocation().Y;
	case EVoxelAxis::Z: return GetActorLocation().Z;
	}
}

void AVoxelMetaGraphPreviewActor::SetAxisLocation(float NewAxisValue)
{
	FVector NewPosition = GetActorLocation();
	switch (Axis)
	{
	default: ensure(false);
	case EVoxelAxis::X: NewPosition.X = NewAxisValue; break;
	case EVoxelAxis::Y: NewPosition.Y = NewAxisValue; break;
	case EVoxelAxis::Z: NewPosition.Z = NewAxisValue; break;
	}

	SetActorLocation(NewPosition);
}

#endif