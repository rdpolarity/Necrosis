// Copyright 2022 Andras Ketzer, All rights reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class NecrosisTarget : TargetRules
{
	public NecrosisTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;

		ExtraModuleNames.AddRange( new string[] { "Necrosis" } );
	}
}
