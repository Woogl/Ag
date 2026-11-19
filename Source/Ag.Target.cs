// Copyright Woogle. All Rights Reserved.

using UnrealBuildTool;

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
