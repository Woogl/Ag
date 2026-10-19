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
			"AnimGraphRuntime",
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
			"SlateCore",
			"Niagara",
			"StateTreeModule",
			"GameplayStateTreeModule"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "EngineSettings", "EngineCameras", "Json", "JsonUtilities", "LevelSequence", "MovieScene" });
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "AssetRegistry", "TargetPlatform" });
		}

		// JSON 데이터 파일은 Content 밖에 있어 쿡이 알지 못하므로 따로 넣습니다.
		RuntimeDependencies.Add("$(ProjectDir)/Data/*.json", StagedFileType.UFS);

		PublicIncludePaths.AddRange(new string[] {
			"Ag"
		});
	}
}
