// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UDreamLipSyncClip;

struct FDreamLipSyncAudio2FaceImporter
{
	static bool ImportAudio2FaceJsonFile(UDreamLipSyncClip* Clip, const FString& JsonFilePath, FString& OutMessage);
	static bool ImportAudio2FaceJsonString(UDreamLipSyncClip* Clip, const FString& JsonString, FString& OutMessage);
};
