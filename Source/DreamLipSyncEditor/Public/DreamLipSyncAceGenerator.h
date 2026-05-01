// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UDreamLipSyncClip;

struct FDreamLipSyncAceGenerator
{
	static bool IsAceAvailable();
	static bool GenerateClipFromAce(UDreamLipSyncClip* Clip, FString& OutMessage);
};
