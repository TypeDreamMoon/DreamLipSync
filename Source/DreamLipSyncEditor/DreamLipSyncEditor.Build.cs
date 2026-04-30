// Copyright 2026 DreamDev, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DreamLipSyncEditor : ModuleRules
{
	public DreamLipSyncEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"DreamLipSync",
				"Engine",
				"AssetTools",
				"Json",
				"MovieScene",
				"MovieSceneTools",
				"MovieSceneTracks",
				"Sequencer",
				"UnrealEd"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"DesktopPlatform",
				"Slate",
				"SlateCore",
				"SequencerCore"
			}
		);
	}
}
