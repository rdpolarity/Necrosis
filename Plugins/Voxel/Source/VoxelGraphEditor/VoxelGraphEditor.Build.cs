// Copyright Voxel Plugin, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VoxelGraphEditor : VoxelModuleRules
{
    public VoxelGraphEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateDependencyModuleNames.AddRange(
            new string[] 
            {
                "ToolMenus",
                "GraphEditor",
                "ApplicationCore",
                "BlueprintGraph",
            });
    }
}