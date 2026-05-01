// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#include "DreamLipSyncClipAssetTypeActions.h"

#include "DesktopPlatformModule.h"
#include "DreamLipSyncAceGenerator.h"
#include "DreamLipSyncClip.h"
#include "DreamLipSyncMfaGenerator.h"
#include "DreamLipSyncRhubarbGenerator.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IDesktopPlatform.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "DreamLipSyncClipAssetTypeActions"

namespace
{
void* GetDialogParentWindowHandle()
{
	TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindBestParentWindowForDialogs(nullptr);
	return ParentWindow.IsValid() && ParentWindow->GetNativeWindow().IsValid() ? ParentWindow->GetNativeWindow()->GetOSWindowHandle() : nullptr;
}

void ShowDreamLipSyncMessage(const FText& Title, const FString& Message)
{
	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message), Title);
}
}

FText FDreamLipSyncClipAssetTypeActions::GetName() const
{
	return LOCTEXT("DreamLipSyncClipName", "Dream Lip Sync Clip");
}

FColor FDreamLipSyncClipAssetTypeActions::GetTypeColor() const
{
	return FColor(80, 180, 220);
}

UClass* FDreamLipSyncClipAssetTypeActions::GetSupportedClass() const
{
	return UDreamLipSyncClip::StaticClass();
}

uint32 FDreamLipSyncClipAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Animation;
}

void FDreamLipSyncClipAssetTypeActions::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
	const TArray<TWeakObjectPtr<UDreamLipSyncClip>> Clips = GetTypedWeakObjectPtrs<UDreamLipSyncClip>(InObjects);

	if (FDreamLipSyncAceGenerator::IsAceAvailable())
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("GenerateFromAce", "Generate From NVIDIA ACE"),
			LOCTEXT("GenerateFromAceTooltip", "Run NVIDIA ACE Audio2Face-3D offline and write generated MorphFrames."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &FDreamLipSyncClipAssetTypeActions::GenerateFromAce, Clips))
		);

		MenuBuilder.AddMenuSeparator();
	}

	MenuBuilder.AddMenuEntry(
		LOCTEXT("GenerateFromRhubarb", "Generate From Rhubarb"),
		LOCTEXT("GenerateFromRhubarbTooltip", "Run Rhubarb Lip Sync on the clip audio and write generated MorphFrames."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &FDreamLipSyncClipAssetTypeActions::GenerateFromRhubarb, Clips))
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ImportRhubarbJson", "Import Rhubarb JSON"),
		LOCTEXT("ImportRhubarbJsonTooltip", "Import an existing Rhubarb JSON file into this clip as MorphFrames."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &FDreamLipSyncClipAssetTypeActions::ImportRhubarbJson, Clips))
	);

	MenuBuilder.AddMenuSeparator();

	MenuBuilder.AddMenuEntry(
		LOCTEXT("GenerateFromMfa", "Generate From MFA"),
		LOCTEXT("GenerateFromMfaTooltip", "Run Montreal Forced Aligner with the clip audio and DialogueText, then write generated MorphFrames."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &FDreamLipSyncClipAssetTypeActions::GenerateFromMfa, Clips))
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ImportMfaTextGrid", "Import MFA TextGrid"),
		LOCTEXT("ImportMfaTextGridTooltip", "Import an existing Montreal Forced Aligner TextGrid file into this clip as MorphFrames."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &FDreamLipSyncClipAssetTypeActions::ImportMfaTextGrid, Clips))
	);
}

void FDreamLipSyncClipAssetTypeActions::GenerateFromAce(TArray<TWeakObjectPtr<UDreamLipSyncClip>> Clips)
{
	int32 SuccessCount = 0;
	TArray<FString> Messages;

	for (const TWeakObjectPtr<UDreamLipSyncClip>& WeakClip : Clips)
	{
		UDreamLipSyncClip* Clip = WeakClip.Get();
		if (!Clip)
		{
			continue;
		}

		FString Message;
		if (FDreamLipSyncAceGenerator::GenerateClipFromAce(Clip, Message))
		{
			++SuccessCount;
		}

		Messages.Add(FString::Printf(TEXT("%s: %s"), *Clip->GetName(), *Message));
	}

	const FText Title = SuccessCount == Clips.Num()
		? LOCTEXT("GenerateFromAceSucceeded", "Dream Lip Sync")
		: LOCTEXT("GenerateFromAceFinishedWithIssues", "Dream Lip Sync - Issues");
	ShowDreamLipSyncMessage(Title, FString::Join(Messages, TEXT("\n")));
}

void FDreamLipSyncClipAssetTypeActions::GenerateFromRhubarb(TArray<TWeakObjectPtr<UDreamLipSyncClip>> Clips)
{
	int32 SuccessCount = 0;
	TArray<FString> Messages;

	for (const TWeakObjectPtr<UDreamLipSyncClip>& WeakClip : Clips)
	{
		UDreamLipSyncClip* Clip = WeakClip.Get();
		if (!Clip)
		{
			continue;
		}

		FString Message;
		if (FDreamLipSyncRhubarbGenerator::GenerateClipFromRhubarb(Clip, Message))
		{
			++SuccessCount;
		}

		Messages.Add(FString::Printf(TEXT("%s: %s"), *Clip->GetName(), *Message));
	}

	const FText Title = SuccessCount == Clips.Num()
		? LOCTEXT("GenerateFromRhubarbSucceeded", "Dream Lip Sync")
		: LOCTEXT("GenerateFromRhubarbFinishedWithIssues", "Dream Lip Sync - Issues");
	ShowDreamLipSyncMessage(Title, FString::Join(Messages, TEXT("\n")));
}

void FDreamLipSyncClipAssetTypeActions::ImportRhubarbJson(TArray<TWeakObjectPtr<UDreamLipSyncClip>> Clips)
{
	if (Clips.Num() != 1)
	{
		ShowDreamLipSyncMessage(LOCTEXT("ImportOneClipOnlyTitle", "Dream Lip Sync"), TEXT("Import Rhubarb JSON supports one clip at a time."));
		return;
	}

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return;
	}

	TArray<FString> OpenFilenames;
	const bool bPickedFile = DesktopPlatform->OpenFileDialog(
		GetDialogParentWindowHandle(),
		TEXT("Import Rhubarb JSON"),
		FPaths::ProjectContentDir(),
		TEXT(""),
		TEXT("Rhubarb JSON (*.json)|*.json"),
		0,
		OpenFilenames);

	if (!bPickedFile || OpenFilenames.IsEmpty())
	{
		return;
	}

	UDreamLipSyncClip* Clip = Clips[0].Get();
	if (!Clip)
	{
		return;
	}

	FString Message;
	const bool bImported = FDreamLipSyncRhubarbGenerator::ImportRhubarbJsonFile(Clip, OpenFilenames[0], Message);
	ShowDreamLipSyncMessage(
		bImported ? LOCTEXT("ImportRhubarbJsonSucceeded", "Dream Lip Sync") : LOCTEXT("ImportRhubarbJsonFailed", "Dream Lip Sync - Import Failed"),
		Message);
}

void FDreamLipSyncClipAssetTypeActions::GenerateFromMfa(TArray<TWeakObjectPtr<UDreamLipSyncClip>> Clips)
{
	int32 SuccessCount = 0;
	TArray<FString> Messages;

	for (const TWeakObjectPtr<UDreamLipSyncClip>& WeakClip : Clips)
	{
		UDreamLipSyncClip* Clip = WeakClip.Get();
		if (!Clip)
		{
			continue;
		}

		FString Message;
		if (FDreamLipSyncMfaGenerator::GenerateClipFromMfa(Clip, Message))
		{
			++SuccessCount;
		}

		Messages.Add(FString::Printf(TEXT("%s: %s"), *Clip->GetName(), *Message));
	}

	const FText Title = SuccessCount == Clips.Num()
		? LOCTEXT("GenerateFromMfaSucceeded", "Dream Lip Sync")
		: LOCTEXT("GenerateFromMfaFinishedWithIssues", "Dream Lip Sync - Issues");
	ShowDreamLipSyncMessage(Title, FString::Join(Messages, TEXT("\n")));
}

void FDreamLipSyncClipAssetTypeActions::ImportMfaTextGrid(TArray<TWeakObjectPtr<UDreamLipSyncClip>> Clips)
{
	if (Clips.Num() != 1)
	{
		ShowDreamLipSyncMessage(LOCTEXT("ImportMfaOneClipOnlyTitle", "Dream Lip Sync"), TEXT("Import MFA TextGrid supports one clip at a time."));
		return;
	}

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return;
	}

	TArray<FString> OpenFilenames;
	const bool bPickedFile = DesktopPlatform->OpenFileDialog(
		GetDialogParentWindowHandle(),
		TEXT("Import MFA TextGrid"),
		FPaths::ProjectContentDir(),
		TEXT(""),
		TEXT("TextGrid (*.TextGrid;*.textgrid)|*.TextGrid;*.textgrid"),
		0,
		OpenFilenames);

	if (!bPickedFile || OpenFilenames.IsEmpty())
	{
		return;
	}

	UDreamLipSyncClip* Clip = Clips[0].Get();
	if (!Clip)
	{
		return;
	}

	FString Message;
	const bool bImported = FDreamLipSyncMfaGenerator::ImportMfaTextGridFile(Clip, OpenFilenames[0], Message);
	ShowDreamLipSyncMessage(
		bImported ? LOCTEXT("ImportMfaTextGridSucceeded", "Dream Lip Sync") : LOCTEXT("ImportMfaTextGridFailed", "Dream Lip Sync - Import Failed"),
		Message);
}

#undef LOCTEXT_NAMESPACE
