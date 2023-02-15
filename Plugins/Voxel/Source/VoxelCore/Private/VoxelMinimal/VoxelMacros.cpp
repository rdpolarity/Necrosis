// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMinimal.h"

VOXELCORE_API FSimpleMulticastDelegate GOnVoxelModuleUnloaded;

FString Voxel_RemoveFunctionNameScope(const FString& FunctionName)
{
	VOXEL_FUNCTION_COUNTER();

	FString Right;
	if (FunctionName.Split(TEXT("::"), nullptr, &Right, ESearchCase::CaseSensitive, ESearchDir::FromEnd))
	{
		return Right;
	}
	else
	{
		return FunctionName;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct FVoxelRunOnStartupStatics
{
	struct FFunctions
	{
		bool bRun = false;
		TArray<TPair<int32, TFunction<void()>>> Functions;

		void Add(const int32 Priority, const TFunction<void()>& Lambda)
		{
			check(!bRun);
			Functions.Add({ Priority, Lambda });
		}
		void Execute()
		{
			VOXEL_LLM_SCOPE();
			VOXEL_FUNCTION_COUNTER();

			check(!bRun);
			bRun = true;

			Functions.Sort([](const TPair<int32, TFunction<void()>>& A, const TPair<int32, TFunction<void()>>& B)
			{
				return A.Key > B.Key;
			});

			for (const auto& It : Functions)
			{
				It.Value();
			}

			Functions.Empty();
		}
	};

	FFunctions GameFunctions;
	FFunctions EditorFunctions;
	FFunctions FirstTickFunctions;

	static FVoxelRunOnStartupStatics& Get()
	{
		static FVoxelRunOnStartupStatics Statics;
		return Statics;
	}
};

FVoxelRunOnStartupPhaseHelper::FVoxelRunOnStartupPhaseHelper(const EVoxelRunOnStartupPhase Phase, const int32 Priority, TFunction<void()> Lambda)
{
	if (Phase == EVoxelRunOnStartupPhase::Game)
	{
		FVoxelRunOnStartupStatics::Get().GameFunctions.Add(Priority, MoveTemp(Lambda));
	}
	else if (Phase == EVoxelRunOnStartupPhase::Editor)
	{
		FVoxelRunOnStartupStatics::Get().EditorFunctions.Add(Priority, MoveTemp(Lambda));
	}
	else
	{
		check(Phase == EVoxelRunOnStartupPhase::FirstTick);
		FVoxelRunOnStartupStatics::Get().FirstTickFunctions.Add(Priority, MoveTemp(Lambda));
	}
}

FDelayedAutoRegisterHelper GVoxelRunOnStartup_Game(EDelayedRegisterRunPhase::ObjectSystemReady, []
{
	IPluginManager::Get().OnLoadingPhaseComplete().AddLambda([](ELoadingPhase::Type LoadingPhase, bool bSuccess)
	{
		if (LoadingPhase != ELoadingPhase::PostDefault)
		{
			return;
		}

		FVoxelRunOnStartupStatics::Get().GameFunctions.Execute();
	});
});

FDelayedAutoRegisterHelper GVoxelRunOnStartup_Editor(EDelayedRegisterRunPhase::EndOfEngineInit, []
{
	FVoxelRunOnStartupStatics::Get().EditorFunctions.Execute();
});

FDelayedAutoRegisterHelper GVoxelRunOnStartup_FirstTick(EDelayedRegisterRunPhase::EndOfEngineInit, []
{
	FVoxelRunOnStartupStatics::Get().FirstTickFunctions.Execute();
});