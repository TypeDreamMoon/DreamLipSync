// Copyright 2026 DreamDev, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DreamLipSync : ModuleRules
{
	public DreamLipSync(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"DeveloperSettings",
				"Engine",
				"MovieScene",
				"MovieSceneTracks"
			}
		);
	}
}
