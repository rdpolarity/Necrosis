// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Tools/UEdMode.h"
#include "InteractiveGizmo.h"
#include "VoxelLandmassActor.h"
#include "BaseGizmos/GizmoActor.h"
#include "BaseGizmos/AxisPositionGizmo.h"
#include "BaseGizmos/GizmoBaseComponent.h"
#include "VoxelLandmassEdMode.generated.h"

UCLASS()
class UVoxelLandmassEdModeSettings : public UObject
{
	GENERATED_BODY()
};

UCLASS(Abstract)
class UGizmoVoxelLandmassBaseParameterSource : public UGizmoBaseFloatParameterSource
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<AVoxelLandmassActor> Actor;

	virtual void BeginModify() override
	{
		if (Actor)
		{
			Actor->Modify();
		}
	}

	virtual void EndModify() override
	{
	}

public:
	template<typename T>
	static T* Construct(AVoxelLandmassActor* Actor)
	{
		T* NewSource = NewObject<T>();
		NewSource->Actor = Actor;
		return NewSource;
	}
};

UCLASS()
class UGizmoVoxelLandmassSmoothnessParameterSource : public UGizmoVoxelLandmassBaseParameterSource
{
	GENERATED_BODY()

public:
	virtual float GetParameter() const override
	{
		return 0; // TODO Actor->Smoothness * 1000;
	}

	virtual void SetParameter(float NewValue) override
	{
		// TODO
		//Actor->Smoothness = FMath::Clamp(NewValue / 1000.f, 0.f, 1.f);
		Actor->UpdateBrush();
	}
};

UCLASS()
class UGizmoVoxelLandmassStrengthParameterSource : public UGizmoVoxelLandmassBaseParameterSource
{
	GENERATED_BODY()

public:
	virtual float GetParameter() const override
	{
		return 0; // TODO Actor->Strength * 1000;
	}

	virtual void SetParameter(float NewValue) override
	{
		// TODO
		//Actor->Strength = FMath::Clamp(NewValue / 1000.f, 0.f, 1.f);
		Actor->UpdateBrush();
	}
};

UCLASS(Transient)
class AVoxelLandmassGizmoActor : public AGizmoActor
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UGizmoCircleComponent> SmoothnessComponent;
	
	UPROPERTY()
	TObjectPtr<UGizmoCircleComponent> StrengthComponent;

	AVoxelLandmassGizmoActor();
		
	static AVoxelLandmassGizmoActor* ConstructDefaultIntervalGizmo(UWorld* World);

};

UCLASS()
class UVoxelLandmassGizmo : public UInteractiveGizmo
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UWorld> World;

	//~ Begin UInteractiveGizmo interface
	virtual void Setup() override;
	virtual void Shutdown() override;
	virtual void Tick(float DeltaTime) override;
	//~ End UInteractiveGizmo interface

	void SetActor(AVoxelLandmassActor* Actor);

private:
	UPROPERTY()
	TObjectPtr<AVoxelLandmassGizmoActor> GizmoActor;

	UPROPERTY()
	TObjectPtr<AVoxelLandmassActor> LandmassActor;
	
	UPROPERTY()
	TObjectPtr<UAxisPositionGizmo> SmoothnessGizmo;
	
	UPROPERTY()
	TObjectPtr<UAxisPositionGizmo> StrengthGizmo;
};

UCLASS()
class UVoxelLandmassGizmoBuilder : public UInteractiveGizmoBuilder
{
	GENERATED_BODY()

	virtual UInteractiveGizmo* BuildGizmo(const FToolBuilderState& SceneState) const override
	{
		UVoxelLandmassGizmo* NewGizmo = NewObject<UVoxelLandmassGizmo>(SceneState.GizmoManager);
		NewGizmo->World = SceneState.World;
		return NewGizmo;
	}
};

UCLASS()
class UVoxelLandmassEdMode : public UEdMode, public ILegacyEdModeViewportInterface
{
	GENERATED_BODY()

public:
	UVoxelLandmassEdMode();

	//~ Begin UEdMode Interface
	virtual bool IsCompatibleWith(FEditorModeID OtherModeID) const override
	{
		return true;
	}
	virtual bool UsesToolkits() const override
	{
		return false;
	}

	virtual void Enter() override;
	virtual void Exit() override;

	virtual bool Select(AActor* InActor, bool bInSelected) override;
	virtual bool HandleClick(FEditorViewportClient* ViewportClient, HHitProxy* HitProxy, const FViewportClick& Click) override;
	//~ End UEdMode Interface

private:
	UPROPERTY()
	TArray<TObjectPtr<UInteractiveGizmo>> InteractiveGizmos;

	void DestroyGizmos();
};