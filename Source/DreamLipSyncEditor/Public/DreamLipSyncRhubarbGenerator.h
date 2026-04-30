// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UDreamLipSyncClip;

class FDreamLipSyncRhubarbGenerator
{
public:
	static bool GenerateClipFromRhubarb(UDreamLipSyncClip* Clip, FString& OutMessage);
	static bool ImportRhubarbJsonFile(UDreamLipSyncClip* Clip, const FString& JsonFilePath, FString& OutMessage);
	static bool ImportRhubarbJsonString(UDreamLipSyncClip* Clip, const FString& JsonString, FString& OutMessage);

private:
	static bool ResolveInputAudioFile(const UDreamLipSyncClip* Clip, FString& OutAudioFilePath, FString& OutMessage);
	static bool ResolveRhubarbExecutable(FString& OutExecutablePath, FString& OutMessage);
};
