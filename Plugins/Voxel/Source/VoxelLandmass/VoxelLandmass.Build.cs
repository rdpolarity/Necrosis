// Copyright Voxel Plugin, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VoxelLandmass : VoxelModuleRules
{
    public VoxelLandmass(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "VoxelMetaGraph",
                "Chaos",
                "Landscape",
                "MeshDescription",
            }
        );
    }
}