// Copyright Voxel Plugin, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VoxelCanvas : VoxelModuleRules
{
    public VoxelCanvas(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "VoxelMetaGraph",
            }
        );
    }
}