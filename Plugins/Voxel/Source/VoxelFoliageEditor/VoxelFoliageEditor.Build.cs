// Copyright Voxel Plugin, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VoxelFoliageEditor : VoxelModuleRules
{
    public VoxelFoliageEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "VoxelFoliage",
                "EditorWidgets",
            });
    }
}