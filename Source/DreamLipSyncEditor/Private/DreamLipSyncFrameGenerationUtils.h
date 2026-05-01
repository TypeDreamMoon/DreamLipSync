// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DreamLipSyncTypes.h"

class UDreamLipSyncClip;
class UDreamLipSyncSettings;

namespace DreamLipSyncFrameGeneration
{
FDreamLipSyncFrameGenerationSettings ResolveSettings(const UDreamLipSyncClip* Clip, const UDreamLipSyncSettings* Settings);
float ResolveBlendTime(const UDreamLipSyncClip* Clip, const UDreamLipSyncSettings* Settings);
void ApplyWeightSettings(TArray<FDreamLipSyncMorphWeight>& Weights, const FDreamLipSyncFrameGenerationSettings& Settings);
void PostProcessFrames(TArray<FDreamLipSyncMorphFrame>& Frames, float Duration, const FDreamLipSyncFrameGenerationSettings& Settings);
}
