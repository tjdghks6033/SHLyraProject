// Copyright SH. All Rights Reserved.

using UnrealBuildTool;

public class SHLyraProjectRuntime : ModuleRules
{
	public SHLyraProjectRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameFeatures",
			"ModularGameplay",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"EnhancedInput",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"LyraGame",
			"CommonGame",
			"CommonUI",
			"GameplayMessageRuntime",
			"AIModule",
			"NavigationSystem",
			"Niagara",
		});
	}
}
