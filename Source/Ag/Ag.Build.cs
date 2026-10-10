// Copyright Woogle. All Rights Reserved.

using UnrealBuildTool;

public class Ag : ModuleRules
{
	public Ag(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"UMG",
			"Slate",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"CommonUI",
			"CommonInput",
			"MotionWarping",
			"TargetingSystem",
			"DeveloperSettings",
			"SlateCore",
			"Niagara"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "Json", "JsonUtilities" });

		// The JSON data files the game reads at runtime (구현 계획서 '데이터'). They sit outside Content, so the cook doesn't know them.
		RuntimeDependencies.Add("$(ProjectDir)/Data/*.json", StagedFileType.UFS);

		PublicIncludePaths.AddRange(new string[] {
			"Ag"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
