// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#include "DreamLipSyncEditorModule.h"

#include "AssetToolsModule.h"
#include "DreamLipSyncClipAssetTypeActions.h"
#include "ISequencerModule.h"
#include "IAssetTools.h"
#include "MovieScene/DreamLipSyncTrackEditor.h"

void FDreamLipSyncEditorModule::StartupModule()
{
	ISequencerModule& SequencerModule = FModuleManager::Get().LoadModuleChecked<ISequencerModule>("Sequencer");
	TrackEditorHandle = SequencerModule.RegisterTrackEditor(FOnCreateTrackEditor::CreateStatic(&FDreamLipSyncTrackEditor::CreateTrackEditor));

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	TSharedPtr<FDreamLipSyncClipAssetTypeActions> LipSyncClipActions = MakeShareable(new FDreamLipSyncClipAssetTypeActions());
	AssetTools.RegisterAssetTypeActions(LipSyncClipActions.ToSharedRef());
	CreatedAssetTypeActions.Add(LipSyncClipActions);
}

void FDreamLipSyncEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("Sequencer"))
	{
		ISequencerModule& SequencerModule = FModuleManager::GetModuleChecked<ISequencerModule>("Sequencer");
		SequencerModule.UnRegisterTrackEditor(TrackEditorHandle);
	}

	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (const TSharedPtr<FAssetTypeActions_Base>& Action : CreatedAssetTypeActions)
		{
			if (Action.IsValid())
			{
				AssetTools.UnregisterAssetTypeActions(Action.ToSharedRef());
			}
		}
	}
	CreatedAssetTypeActions.Empty();
}

IMPLEMENT_MODULE(FDreamLipSyncEditorModule, DreamLipSyncEditor)
