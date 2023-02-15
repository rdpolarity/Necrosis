// Copyright Voxel Plugin, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VoxelMetaGraphEditor : VoxelModuleRules
{
    public VoxelMetaGraphEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "VoxelMetaGraph",
                "VoxelGraphEditor",
                "MainFrame",
                "ToolMenus",
                "MessageLog",
                "GraphEditor",
                "KismetWidgets",
                "EditorWidgets",
                "BlueprintGraph",
                "ApplicationCore",
                
                // For SItemSelector
                "NiagaraEditor",
            }
        );
    }
}