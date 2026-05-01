// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#include "DreamLipSyncEditorModule.h"

#include "AssetToolsModule.h"
#include "DreamLipSyncClipAssetTypeActions.h"
#include "ISequencerModule.h"
#include "IAssetTools.h"
#include "DreamLipSyncSoundWaveActions.h"
#include "MovieScene/DreamLipSyncTrackEditor.h"
#include "ToolMenus.h"

void FDreamLipSyncEditorModule::StartupModule()
{
	ISequencerModule& SequencerModule = FModuleManager::Get().LoadModuleChecked<ISequencerModule>("Sequencer");
	TrackEditorHandle = SequencerModule.RegisterTrackEditor(FOnCreateTrackEditor::CreateStatic(&FDreamLipSyncTrackEditor::CreateTrackEditor));

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	TSharedPtr<FDreamLipSyncClipAssetTypeActions> LipSyncClipActions = MakeShareable(new FDreamLipSyncClipAssetTypeActions());
	AssetTools.RegisterAssetTypeActions(LipSyncClipActions.ToSharedRef());
	CreatedAssetTypeActions.Add(LipSyncClipActions);

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FDreamLipSyncEditorModule::RegisterMenus));
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

	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
}

void FDreamLipSyncEditorModule::RegisterMenus()
{
	DreamLipSyncSoundWaveActions::RegisterMenus(this);
}

IMPLEMENT_MODULE(FDreamLipSyncEditorModule, DreamLipSyncEditor)
