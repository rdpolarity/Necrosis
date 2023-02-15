// Copyright Voxel Plugin, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VoxelMetaGraph : VoxelModuleRules
{
    public VoxelMetaGraph(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "TraceLog",
                "SlateCore",
                "PhysicsCore",
            }
        );
    }
}