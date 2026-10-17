// Copyright Woogle. All Rights Reserved.

using UnrealBuildTool;

public class AgEditor : ModuleRules
{
	public AgEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"UMG",
			"UMGEditor",
			"MovieScene",
			"MovieSceneTracks",
			"ToolsetRegistry",
			"DirectoryWatcher",
			"EditorSubsystem",
			"AssetTools",
			"DataValidation",
			"GameplayTags",
			"StateTreeModule",
			"StateTreeEditorModule",
			"PropertyBindingUtils",
			"Ag"
		});
	}
}
