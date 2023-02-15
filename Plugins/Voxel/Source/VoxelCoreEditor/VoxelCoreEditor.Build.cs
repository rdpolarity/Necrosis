// Copyright Voxel Plugin, Inc. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class VoxelCoreEditor : VoxelModuleRules
{
    public VoxelCoreEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        // For FDetailCategoryBuilderImpl
        PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Source/Editor/PropertyEditor/Private"));

        PublicDependencyModuleNames.AddRange(
            new string[] 
            {
                "AdvancedPreviewScene",
            });

        PrivateDependencyModuleNames.AddRange(
            new string[] 
            {
                "PlacementMode",
                "MessageLog",
                "WorkspaceMenuStructure",
                "DetailCustomizations",
                "BlueprintGraph",
                "Landscape",
                "ToolMenus",
                "SceneOutliner",
            });
    }
}
