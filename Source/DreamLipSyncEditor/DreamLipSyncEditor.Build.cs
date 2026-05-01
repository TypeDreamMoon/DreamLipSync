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
				"AssetRegistry",
				"AssetTools",
				"ContentBrowser",
				"InputCore",
				"Json",
				"MovieScene",
				"MovieSceneTools",
				"MovieSceneTracks",
				"Sequencer",
				"ToolMenus",
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

		bool bWithAce = Plugins.GetPlugin("NV_ACE_Reference") != null;
		if (bWithAce)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"ACECore",
					"ACERuntime"
				}
			);
			PrivateDefinitions.Add("WITH_DREAMLIPSYNC_ACE=1");
		}
		else
		{
			PrivateDefinitions.Add("WITH_DREAMLIPSYNC_ACE=0");
		}
	}
}
