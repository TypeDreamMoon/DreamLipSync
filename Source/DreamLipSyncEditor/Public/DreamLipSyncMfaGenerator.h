// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UDreamLipSyncClip;

struct FDreamLipSyncMfaGenerator
{
	static bool GenerateClipFromMfa(UDreamLipSyncClip* Clip, FString& OutMessage);
	static bool ImportMfaTextGridFile(UDreamLipSyncClip* Clip, const FString& TextGridFilePath, FString& OutMessage);
	static bool ImportMfaTextGridString(UDreamLipSyncClip* Clip, const FString& TextGridString, FString& OutMessage);

private:
	static bool ResolveInputAudioFile(const UDreamLipSyncClip* Clip, FString& OutAudioFilePath, FString& OutMessage);
	static bool ResolveMfaCommand(FString& OutCommand, FString& OutMessage);
};
