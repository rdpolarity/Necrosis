// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMinimal.h"
#include "Engine/World.h"

float FVoxelMaterialUtilities::GetMaterialTime(UWorld* World)
{
	if (!ensure(World)) 
	{
		return 0;
	}
	
	if (World->IsGameWorld())
	{
		return World->GetTimeSeconds();
	}
	else
	{
		return FApp::GetCurrentTime() - GStartTime;
	}
}

void FVoxelMaterialUtilities::SetMatrixParameter(
	UMaterialInstanceDynamic& MaterialInstance,
	const FString& ParameterName,
	const FMatrix& Matrix)
{
	VOXEL_FUNCTION_COUNTER();

	MaterialInstance.SetVectorParameterValue(*(ParameterName + TEXT("0")), FLinearColor(
		Matrix.M[0][0],
		Matrix.M[0][1],
		Matrix.M[0][2],
		Matrix.M[0][3]));

	MaterialInstance.SetVectorParameterValue(*(ParameterName + TEXT("1")), FLinearColor(
		Matrix.M[1][0],
		Matrix.M[1][1],
		Matrix.M[1][2],
		Matrix.M[1][3]));

	MaterialInstance.SetVectorParameterValue(*(ParameterName + TEXT("2")), FLinearColor(
		Matrix.M[2][0],
		Matrix.M[2][1],
		Matrix.M[2][2],
		Matrix.M[2][3]));

	MaterialInstance.SetVectorParameterValue(*(ParameterName + TEXT("3")), FLinearColor(
		Matrix.M[3][0],
		Matrix.M[3][1],
		Matrix.M[3][2],
		Matrix.M[3][3]));
}