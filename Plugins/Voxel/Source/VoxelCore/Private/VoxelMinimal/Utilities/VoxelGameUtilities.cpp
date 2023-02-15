// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMinimal.h"
#include "Kismet/GameplayStatics.h"

#if WITH_EDITOR
#include "Editor.h"
#include "EditorViewportClient.h"
#endif

bool FVoxelGameUtilities::GetCameraView(const UWorld* World, FVector& OutPosition, FRotator& OutRotation, float& OutFOV)
{
	VOXEL_FUNCTION_COUNTER();
	ensure(IsInGameThread());

	if (!World)
	{
		return false;
	}

	if (World->IsGameWorld())
	{
		const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(World, 0);
		if (!ensure(CameraManager))
		{
			return false;
		}

		OutPosition = CameraManager->GetCameraLocation();
		OutRotation = CameraManager->GetCameraRotation();
		OutFOV = CameraManager->GetFOVAngle();

		return true;
	}
	else
	{
#if WITH_EDITOR
		const FViewport* Viewport = GEditor->GetActiveViewport();
		if (!Viewport)
		{
			return false;
		}
		const FViewportClient* Client = Viewport->GetClient();
		if (!Client)
		{
			return false;
		}

		for (const FEditorViewportClient* EditorViewportClient : GEditor->GetAllViewportClients())
		{
			if (EditorViewportClient == Client)
			{
				OutPosition = EditorViewportClient->GetViewLocation();
				OutRotation = EditorViewportClient->GetViewRotation();
				OutFOV = EditorViewportClient->FOVAngle;

				return true;
			}
		}
#endif

		return false;
	}
}