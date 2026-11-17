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

		PrivateDependencyModuleNames.AddRange(new string[] { "ApplicationCore", "EngineSettings","Json", "JsonUtilities", "LevelSequence", "MovieScene", "NavigationSystem", "PhysicsCore" });

		// JSON 데이터 파일은 Content 밖에 있고 문구 표(CSV)는 에셋이 아니어서, 쿡이 알지 못하므로 따로 넣습니다.
		RuntimeDependencies.Add("$(ProjectDir)/DesignData/*.json", StagedFileType.UFS);
		RuntimeDependencies.Add("$(ProjectDir)/Content/StringTables/*.csv", StagedFileType.UFS);

		PublicIncludePaths.AddRange(new string[] {
			"Ag"
		});
	}
}
