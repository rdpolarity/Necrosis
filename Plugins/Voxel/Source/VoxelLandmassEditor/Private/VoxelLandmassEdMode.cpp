// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelLandmassEdMode.h"
#include "VoxelBrushSubsystem.h"
#include "VoxelMesh/VoxelMeshComponent.h"
#include "VoxelRuntime/VoxelRuntimeUtilities.h"

#include "EditorModes.h"
#include "EditorModeManager.h"
#include "LevelEditor.h"
#include "LevelEditorViewport.h"
#include "LevelViewportClickHandlers.h"
#include "BaseGizmos/GizmoArrowComponent.h"
#include "BaseGizmos/GizmoCircleComponent.h"

VOXEL_RUN_ON_STARTUP_EDITOR(ActivateLandmassEdMode)
{
	FLevelEditorModule& LevelEditor = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditor.OnMapChanged().AddLambda([](UWorld* World, EMapChangeType ChangeType)
	{
		if (ChangeType == EMapChangeType::SaveMap)
		{
			return;
		}

		FVoxelSystemUtilities::DelayedCall([]
		{
			GLevelEditorModeTools().AddDefaultMode(GetDefault<UVoxelLandmassEdMode>()->GetID());
			GLevelEditorModeTools().ActivateDefaultMode();
		});
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

AVoxelLandmassGizmoActor::AVoxelLandmassGizmoActor()
{
	// Root component is a hidden sphere
	USphereComponent* SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("GizmoCenter"));
	RootComponent = SphereComponent;
	SphereComponent->InitSphereRadius(1.0f);
	SphereComponent->SetVisibility(false);
	SphereComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}

AVoxelLandmassGizmoActor* AVoxelLandmassGizmoActor::ConstructDefaultIntervalGizmo(UWorld* World)
{
	const FActorSpawnParameters SpawnInfo;
	AVoxelLandmassGizmoActor* NewActor = World->SpawnActor<AVoxelLandmassGizmoActor>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);

	NewActor->SmoothnessComponent = AddDefaultCircleComponent(World, NewActor, nullptr, FColor::Red, FVector(0, 0, 1), 120.0f);
	NewActor->StrengthComponent = AddDefaultCircleComponent(World, NewActor, nullptr, FColor::Green, FVector(0, 0, 1), 180.0f);

	return NewActor;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelLandmassGizmo::Setup()
{
	Super::Setup();

	GizmoActor = AVoxelLandmassGizmoActor::ConstructDefaultIntervalGizmo(World);
}

void UVoxelLandmassGizmo::Shutdown()
{
	SetActor(nullptr);

	if (GizmoActor)
	{
		GizmoActor->Destroy();
		GizmoActor = nullptr;
	}

	Super::Shutdown();
}

void UVoxelLandmassGizmo::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GizmoActor && LandmassActor)
	{
		GizmoActor->SetActorLocation(LandmassActor->GetActorLocation());
	}
}

void UVoxelLandmassGizmo::SetActor(AVoxelLandmassActor* Actor)
{
	LandmassActor = Actor;

	if (SmoothnessGizmo)
	{
		GetGizmoManager()->DestroyGizmo(SmoothnessGizmo);
	}
	if (StrengthGizmo)
	{
		GetGizmoManager()->DestroyGizmo(StrengthGizmo);
	}

	if (!Actor)
	{
		return;
	}

	{
		UGizmoBaseComponent* Component = GizmoActor->SmoothnessComponent;

		SmoothnessGizmo = GetGizmoManager()->CreateGizmo<UAxisPositionGizmo>(UInteractiveGizmoManager::DefaultAxisPositionBuilderIdentifier);
		SmoothnessGizmo->bEnableSignedAxis = true;
		SmoothnessGizmo->AxisSource = UGizmoComponentAxisSource::Construct(Component, 0, true);
		SmoothnessGizmo->ParameterSource = UGizmoVoxelLandmassBaseParameterSource::Construct<UGizmoVoxelLandmassSmoothnessParameterSource>(Actor);

		UGizmoComponentHitTarget* HitTarget = UGizmoComponentHitTarget::Construct(Component);
		HitTarget->UpdateHoverFunction = [=](bool bHovering)
		{
			Component->UpdateHoverState(bHovering);
		};
		SmoothnessGizmo->HitTarget = HitTarget;
		SmoothnessGizmo->StateTarget = UGizmoObjectModifyStateTarget::Construct(Actor, VOXEL_LOCTEXT("Change smoothness"), GetGizmoManager());
	}

	{
		UGizmoBaseComponent* Component = GizmoActor->StrengthComponent;

		StrengthGizmo = GetGizmoManager()->CreateGizmo<UAxisPositionGizmo>(UInteractiveGizmoManager::DefaultAxisPositionBuilderIdentifier);
		StrengthGizmo->bEnableSignedAxis = true;
		StrengthGizmo->AxisSource = UGizmoComponentAxisSource::Construct(Component, 0, true);
		StrengthGizmo->ParameterSource = UGizmoVoxelLandmassBaseParameterSource::Construct<UGizmoVoxelLandmassStrengthParameterSource>(Actor);

		UGizmoComponentHitTarget* HitTarget = UGizmoComponentHitTarget::Construct(Component);
		HitTarget->UpdateHoverFunction = [=](bool bHovering)
		{
			Component->UpdateHoverState(bHovering);
		};
		StrengthGizmo->HitTarget = HitTarget;
		StrengthGizmo->StateTarget = UGizmoObjectModifyStateTarget::Construct(Actor, VOXEL_LOCTEXT("Change strength"), GetGizmoManager());
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UVoxelLandmassEdMode::UVoxelLandmassEdMode()
{
	Info = FEditorModeInfo(
		"VoxelLandmassEdMode",
		VOXEL_LOCTEXT("VoxelLandmassEdMode"),
		FSlateIcon(),
		false,
		MAX_int32
	);

	SettingsClass = UVoxelLandmassEdModeSettings::StaticClass();
}

void UVoxelLandmassEdMode::Enter()
{
	Super::Enter();
	
	GetToolManager()->GetPairedGizmoManager()->RegisterGizmoType("VoxelLandmassGizmo", NewObject<UVoxelLandmassGizmoBuilder>());
}

void UVoxelLandmassEdMode::Exit()
{
	DestroyGizmos();

	GetToolManager()->GetPairedGizmoManager()->DeregisterGizmoType("VoxelLandmassGizmo");

	Super::Exit();
}

bool UVoxelLandmassEdMode::Select(AActor* InActor, bool bInSelected)
{
	// Disable gizmos
	return false;

	DestroyGizmos();

	AVoxelLandmassActor* ActorBase = Cast<AVoxelLandmassActor>(InActor);
	if (!ActorBase)
	{
		return false;
	}
	
	UInteractiveGizmoManager* GizmoManager = GetToolManager()->GetPairedGizmoManager();
	if (!ensure(GizmoManager))
	{
		return false;
	}

	UVoxelLandmassGizmo* Gizmo = GizmoManager->CreateGizmo<UVoxelLandmassGizmo>("VoxelLandmassGizmo");
	Gizmo->SetActor(ActorBase);
	InteractiveGizmos.Add(Gizmo);

	USelection::SelectionChangedEvent.AddWeakLambda(Gizmo, [=](UObject* SelectionObject)
	{
		USelection* Selection = Cast<USelection>(SelectionObject);
		if (!Selection || !ActorBase->IsSelectedInEditor())
		{
			if (InteractiveGizmos.Remove(Gizmo))
			{
				GetToolManager()->GetPairedGizmoManager()->DestroyGizmo(Gizmo);
				Gizmo->MarkAsGarbage();
			}
		}
	});

	return false;
}

bool UVoxelLandmassEdMode::HandleClick(FEditorViewportClient* ViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	const auto SelectActor = [&](AActor* Actor)
	{
		LevelViewportClickHandlers::ClickActor(
			static_cast<FLevelEditorViewportClient*>(ViewportClient),
			Actor,
			Click,
			true);
	};

	if (Click.GetKey() != EKeys::LeftMouseButton ||
		(Click.GetEvent() != IE_Released && Click.GetEvent() != IE_DoubleClick) ||
		!HitProxy ||
		!HitProxy->IsA(HActor::StaticGetType()))
	{
		return false;
	}

	FVector Start;
	FVector End;
	if (!ensure(FVoxelEditorUtilities::GetRayInfo(ViewportClient, Start, End)))
	{
		return false;
	}

	TArray<FHitResult> HitResults;
	if (!GetWorld()->LineTraceMultiByChannel(HitResults, Start, End, ECC_Visibility))
	{
		return false;
	}

	AActor* Actor = static_cast<HActor*>(HitProxy)->Actor;
	TSharedPtr<FVoxelRuntime> Runtime = FVoxelRuntimeUtilities::GetRuntime(Actor);

	FHitResult VoxelWorldHit;
	if (Runtime)
	{
		for (const FHitResult& HitResult : HitResults)
		{
			if (HitResult.GetActor() == Actor)
			{
				VoxelWorldHit = HitResult;
				break;
			}
		}
	}
	else if (Cast<AVoxelBrushActor>(Actor))
	{
		for (const FHitResult& HitResult : HitResults)
		{
			Runtime = FVoxelRuntimeUtilities::GetRuntime(HitResult.GetActor());
			if (Runtime)
			{
				VoxelWorldHit = HitResult;
				break;
			}
		}
	}
	if (!VoxelWorldHit.IsValidBlockingHit())
	{
		return false;
	}

	const FVoxelBrushSubsystem& Subsystem = Runtime->GetSubsystem<FVoxelBrushSubsystem>();

	FVoxelBrushSubsystem::FFindResult FindResult;
	if (!Subsystem.FindClosestBrush(FindResult, VoxelWorldHit.Location))
	{
		return false;
	}

	AActor* BrushActor = FindResult.Brush->GetBrush().ActorToSelect.Get();
	float BrushDistance = FindResult.Distance;

	if (!BrushActor ||
		BrushDistance > 10000)
	{
		return false;
	}

	if (!ensure(ViewportClient->IsLevelEditorClient()))
	{
		return false;
	}

	SelectActor(BrushActor);

	return true;
}

void UVoxelLandmassEdMode::DestroyGizmos()
{
	UInteractiveGizmoManager* GizmoManager = GetToolManager()->GetPairedGizmoManager();
	if (!ensure(GizmoManager))
	{
		return;
	}

	for (UInteractiveGizmo* Gizmo : InteractiveGizmos)
	{
		GizmoManager->DestroyGizmo(Gizmo);
		Gizmo->MarkAsGarbage();
	}

	InteractiveGizmos.Empty();
}