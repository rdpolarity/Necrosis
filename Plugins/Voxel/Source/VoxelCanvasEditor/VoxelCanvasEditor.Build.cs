// Copyright Voxel Plugin, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VoxelCanvasEditor : VoxelModuleRules
{
    public VoxelCanvasEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "VoxelCanvas",
                "InteractiveToolsFramework",
                "EditorInteractiveToolsFramework"
            });
    }
}