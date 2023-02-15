// Copyright Voxel Plugin, Inc. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class VoxelModuleRules : ModuleRules
{
	protected bool bDevWorkflow = false;
	protected bool bGeneratingResharperProjectFiles = false;

	public VoxelModuleRules(ReadOnlyTargetRules Target) : base(Target)
	{
		DefaultBuildSettings = BuildSettingsVersion.V2;

		bGeneratingResharperProjectFiles = Environment.CommandLine.Contains(" -Rider ");

		if (bGeneratingResharperProjectFiles)
        {
			PCHUsage = PCHUsageMode.NoPCHs;
            PublicDefinitions.Add("INTELLISENSE_PARSER=1");
        }

		if (Target.Configuration == UnrealTargetConfiguration.Debug ||
			Target.Configuration == UnrealTargetConfiguration.DebugGame)
		{
			bDevWorkflow = File.Exists(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "/Unreal Engine/VoxelDevWorkflow_Debug.txt");
		}
		else
		{
			bDevWorkflow = File.Exists(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "/Unreal Engine/VoxelDevWorkflow_Dev.txt");
		}

        if (PluginDirectory.Contains("HostProject"))
        {
            bDevWorkflow = false;
        }
		
		if (bDevWorkflow)
		{
			bUseUnity = File.Exists(PluginDirectory + "/../EnableUnity.txt");
		}
		else
		{
			// Unoptimized voxel code is _really_ slow, hurting iteration times
			// for projects with VP as a project plugin using DebugGame
			OptimizeCode = CodeOptimization.Always;
			bUseUnity = true;
		}

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"RHI",
				"Engine",
				"InputCore",
				"RenderCore",
				"DeveloperSettings",
			});

		if (GetType().Name != "VoxelCore")
		{
			PublicDependencyModuleNames.Add("VoxelCore");

            if (GetType().Name != "VoxelExternal")
            {
                PublicDependencyModuleNames.Add("VoxelExternal");

                if (GetType().Name != "VoxelRuntime")
                {
                    PublicDependencyModuleNames.Add("VoxelRuntime");
                }
            }
        }

		if (GetType().Name.EndsWith("Editor"))
		{
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Slate",
					"SlateCore",
					"EditorStyle",
					"PropertyEditor",
#if UE_5_0_OR_LATER
                    "EditorFramework",
#endif
                }
			);

			if (GetType().Name != "VoxelCoreEditor")
			{
				PublicDependencyModuleNames.Add("VoxelCoreEditor");
			}
		}

		if (Target.bBuildEditor)
		{
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"UnrealEd",
				}
			);
		}

		PrivateIncludePathModuleNames.Add("DerivedDataCache");

		if (!Target.bBuildRequiresCookedData)
		{
			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					"DerivedDataCache",
				});
		}

		if (bGeneratingResharperProjectFiles)
		{
			// ISPC support for R#
			PublicIncludePaths.Add(Path.Combine(
				PluginDirectory,
				"Intermediate",
				"Build",
				Target.Platform.ToString(),
				Target.bBuildEditor ? "UnrealEditor" : "UnrealGame",
				Target.Configuration.ToString(),
				GetType().Name));
		}
	}
}

public class VoxelCore : VoxelModuleRules
{
	public VoxelCore(ReadOnlyTargetRules Target) : base(Target)
	{
		bEnforceIWYU = false;

		if (!bUseUnity)
		{
			PrivatePCHHeaderFile = "Public/VoxelSharedPCH.h";

			Console.WriteLine("Using Voxel shared PCH");
			SharedPCHHeaderFile = "Public/VoxelSharedPCH.h";
		}

		bool bVoxelDebug = false;

		if (Target.Configuration == UnrealTargetConfiguration.Debug)
		{
			bVoxelDebug = true;
		}
		if (bDevWorkflow &&
			Target.Configuration == UnrealTargetConfiguration.DebugGame)
		{
			bVoxelDebug = true;
		}

		PublicDefinitions.Add("VOXEL_DEBUG=" + (bVoxelDebug ? "1" : "0"));
		PublicDefinitions.Add("VOXEL_PLUGIN_NAME=TEXT(\"Voxel\")");

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"TraceLog",
				"Renderer",
				"Projects",
				"PhysicsCore",
				"ApplicationCore",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"zlib",
				"UElibPNG",
				"Slate",
				"SlateCore",
				"Landscape",
			}
		);

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"DesktopPlatform",
				}
			);
		}

		if (Target.bWithLiveCoding)
		{
			PrivateIncludePathModuleNames.Add("LiveCoding");
		}

		SetupModulePhysicsSupport(Target);
	}
}