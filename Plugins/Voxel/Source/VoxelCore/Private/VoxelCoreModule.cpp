// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelMinimal.h"

class FVoxelCoreModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		VOXEL_LLM_SCOPE();
		LOG_VOXEL(Log, "VOXEL_DEBUG=%d", VOXEL_DEBUG);

		const IPlugin& Plugin = FVoxelSystemUtilities::GetPlugin();

		// This is needed to correctly share content across Pro and Free
		FPackageName::UnRegisterMountPoint(TEXT("/") VOXEL_PLUGIN_NAME TEXT("/"), Plugin.GetContentDir());
		FPackageName::RegisterMountPoint("/VoxelPlugin/", Plugin.GetContentDir());

		const FString Shaders = FPaths::ConvertRelativePathToFull(Plugin.GetBaseDir() / "Shaders");
		AddShaderSourceDirectoryMapping(TEXT("/Engine/Plugins/Voxel"), Shaders);
	}
	virtual void PreUnloadCallback() override
	{
		GOnVoxelModuleUnloaded.Broadcast();
	}
};

IMPLEMENT_MODULE(FVoxelCoreModule, VoxelCore);

#if VOXEL_DEBUG
extern "C" void VoxelISPC_Assert(const int32 Line)
{
	ensureAlwaysMsgf(false, TEXT("%d"), Line);
}
#endif