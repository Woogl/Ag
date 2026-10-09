// Copyright Epic Games, Inc. All Rights Reserved.

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
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"CommonUI",
			"MotionWarping",
			"TargetingSystem"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Ag",
			"Ag/Variant_Platforming",
			"Ag/Variant_Platforming/Animation",
			"Ag/Variant_Combat",
			"Ag/Variant_Combat/AI",
			"Ag/Variant_Combat/Animation",
			"Ag/Variant_Combat/Gameplay",
			"Ag/Variant_Combat/Interfaces",
			"Ag/Variant_Combat/UI",
			"Ag/Variant_SideScrolling",
			"Ag/Variant_SideScrolling/AI",
			"Ag/Variant_SideScrolling/Gameplay",
			"Ag/Variant_SideScrolling/Interfaces",
			"Ag/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
