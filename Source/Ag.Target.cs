// Copyright Woogle. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class AgTarget : TargetRules
{
	public AgTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("Ag");
	}
}
