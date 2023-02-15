// Copyright Voxel Plugin, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class VoxelRuntime : VoxelModuleRules
{
    public VoxelRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
	    SetupModulePhysicsSupport(Target);

	    PrivateIncludePaths.AddRange(
		    new string[] {
			    Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Private")
		    }
	    );
    }
}