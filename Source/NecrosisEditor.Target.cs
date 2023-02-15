// Copyright 2022 Andras Ketzer, All rights reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class NecrosisEditorTarget : TargetRules
{
	public NecrosisEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;

		ExtraModuleNames.AddRange( new string[] { "Necrosis" } );
	}
}
