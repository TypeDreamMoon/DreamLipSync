// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#include "DreamLipSyncFrameGenerationUtils.h"

#include "DreamLipSyncClip.h"
#include "DreamLipSyncSettings.h"

namespace
{
float GetWeightFromFrame(const FDreamLipSyncMorphFrame& Frame, FName MorphTargetName)
{
	for (const FDreamLipSyncMorphWeight& Weight : Frame.Weights)
	{
		if (Weight.MorphTargetName == MorphTargetName)
		{
			return Weight.Weight;
		}
	}

	return 0.f;
}

void SetFrameWeights(FDreamLipSyncMorphFrame& Frame, const TSet<FName>& MorphNames, const TMap<FName, float>& Values)
{
	Frame.Weights.Reset();
	for (const FName& MorphName : MorphNames)
	{
		const float Weight = Values.Contains(MorphName) ? Values[MorphName] : 0.f;
		if (!MorphName.IsNone() && Weight > 0.0001f)
		{
			FDreamLipSyncMorphWeight& NewWeight = Frame.Weights.AddDefaulted_GetRef();
			NewWeight.MorphTargetName = MorphName;
			NewWeight.Weight = Weight;
		}
	}
}

void CollectMorphNames(const TArray<FDreamLipSyncMorphFrame>& Frames, TSet<FName>& OutMorphNames)
{
	for (const FDreamLipSyncMorphFrame& Frame : Frames)
	{
		for (const FDreamLipSyncMorphWeight& Weight : Frame.Weights)
		{
			if (!Weight.MorphTargetName.IsNone())
			{
				OutMorphNames.Add(Weight.MorphTargetName);
			}
		}
	}
}

void SortAndClampFrames(TArray<FDreamLipSyncMorphFrame>& Frames, const FDreamLipSyncFrameGenerationSettings& Settings)
{
	Frames.Sort([](const FDreamLipSyncMorphFrame& A, const FDreamLipSyncMorphFrame& B)
	{
		return A.Time < B.Time;
	});

	TArray<FDreamLipSyncMorphFrame> MergedFrames;
	MergedFrames.Reserve(Frames.Num());

	for (FDreamLipSyncMorphFrame& Frame : Frames)
	{
		Frame.Time = FMath::Max(0.f, Frame.Time);
		for (int32 Index = Frame.Weights.Num() - 1; Index >= 0; --Index)
		{
			FDreamLipSyncMorphWeight& Weight = Frame.Weights[Index];
			Weight.Weight = FMath::Clamp(Weight.Weight * Settings.WeightScale, 0.f, FMath::Max(0.f, Settings.MaxWeight));
			if (Weight.MorphTargetName.IsNone() || Weight.Weight <= 0.0001f)
			{
				Frame.Weights.RemoveAtSwap(Index);
			}
		}

		if (!MergedFrames.IsEmpty() && FMath::IsNearlyEqual(MergedFrames.Last().Time, Frame.Time, 0.001f))
		{
			MergedFrames.Last().Weights = MoveTemp(Frame.Weights);
			continue;
		}

		MergedFrames.Add(MoveTemp(Frame));
	}

	Frames = MoveTemp(MergedFrames);
}

void SortAndMergeFrames(TArray<FDreamLipSyncMorphFrame>& Frames)
{
	Frames.Sort([](const FDreamLipSyncMorphFrame& A, const FDreamLipSyncMorphFrame& B)
	{
		return A.Time < B.Time;
	});

	TArray<FDreamLipSyncMorphFrame> MergedFrames;
	MergedFrames.Reserve(Frames.Num());
	for (FDreamLipSyncMorphFrame& Frame : Frames)
	{
		Frame.Time = FMath::Max(0.f, Frame.Time);
		if (!MergedFrames.IsEmpty() && FMath::IsNearlyEqual(MergedFrames.Last().Time, Frame.Time, 0.001f))
		{
			MergedFrames.Last().Weights = MoveTemp(Frame.Weights);
			continue;
		}

		MergedFrames.Add(MoveTemp(Frame));
	}

	Frames = MoveTemp(MergedFrames);
}

void EvaluateFramesAtTime(const TArray<FDreamLipSyncMorphFrame>& Frames, float Time, const TSet<FName>& MorphNames, TMap<FName, float>& OutWeights)
{
	OutWeights.Reset();
	if (Frames.IsEmpty())
	{
		return;
	}

	if (Time < Frames[0].Time)
	{
		return;
	}

	if (Time >= Frames.Last().Time)
	{
		for (const FName& MorphName : MorphNames)
		{
			OutWeights.Add(MorphName, GetWeightFromFrame(Frames.Last(), MorphName));
		}
		return;
	}

	const FDreamLipSyncMorphFrame* PreviousFrame = &Frames[0];
	const FDreamLipSyncMorphFrame* NextFrame = &Frames[0];
	for (int32 Index = 1; Index < Frames.Num(); ++Index)
	{
		if (Time <= Frames[Index].Time)
		{
			PreviousFrame = &Frames[Index - 1];
			NextFrame = &Frames[Index];
			break;
		}
	}

	const float Range = FMath::Max(KINDA_SMALL_NUMBER, NextFrame->Time - PreviousFrame->Time);
	const float Alpha = FMath::Clamp((Time - PreviousFrame->Time) / Range, 0.f, 1.f);
	for (const FName& MorphName : MorphNames)
	{
		const float PreviousWeight = GetWeightFromFrame(*PreviousFrame, MorphName);
		const float NextWeight = GetWeightFromFrame(*NextFrame, MorphName);
		OutWeights.Add(MorphName, FMath::Lerp(PreviousWeight, NextWeight, Alpha));
	}
}

void BakeFixedFrameRate(TArray<FDreamLipSyncMorphFrame>& Frames, float Duration, const FDreamLipSyncFrameGenerationSettings& Settings)
{
	if (Frames.IsEmpty())
	{
		return;
	}

	TSet<FName> MorphNames;
	CollectMorphNames(Frames, MorphNames);
	if (MorphNames.IsEmpty())
	{
		return;
	}

	const float EndTime = FMath::Max(Duration, Frames.Last().Time);
	const float FrameRate = FMath::Clamp(Settings.BakeFrameRate, 1.f, 240.f);
	const float Step = 1.f / FrameRate;

	TArray<FDreamLipSyncMorphFrame> BakedFrames;
	const int32 FrameCount = FMath::Max(1, FMath::CeilToInt(EndTime / Step) + 1);
	BakedFrames.Reserve(FrameCount);

	for (int32 FrameIndex = 0; FrameIndex < FrameCount; ++FrameIndex)
	{
		const float Time = FMath::Min(EndTime, FrameIndex * Step);
		TMap<FName, float> Values;
		EvaluateFramesAtTime(Frames, Time, MorphNames, Values);

		FDreamLipSyncMorphFrame& Frame = BakedFrames.AddDefaulted_GetRef();
		Frame.Time = Time;
		SetFrameWeights(Frame, MorphNames, Values);
	}

	Frames = MoveTemp(BakedFrames);
}

void SmoothFrames(TArray<FDreamLipSyncMorphFrame>& Frames, const FDreamLipSyncFrameGenerationSettings& Settings)
{
	const int32 Passes = FMath::Clamp(Settings.SmoothingPasses, 0, 8);
	const float Strength = FMath::Clamp(Settings.SmoothingStrength, 0.f, 1.f);
	if (Passes <= 0 || Strength <= 0.f || Frames.Num() < 3)
	{
		return;
	}

	TSet<FName> MorphNames;
	CollectMorphNames(Frames, MorphNames);
	if (MorphNames.IsEmpty())
	{
		return;
	}

	for (int32 PassIndex = 0; PassIndex < Passes; ++PassIndex)
	{
		TArray<FDreamLipSyncMorphFrame> SourceFrames = Frames;
		for (int32 FrameIndex = 1; FrameIndex < Frames.Num() - 1; ++FrameIndex)
		{
			TMap<FName, float> Values;
			for (const FName& MorphName : MorphNames)
			{
				const float PreviousWeight = GetWeightFromFrame(SourceFrames[FrameIndex - 1], MorphName);
				const float CurrentWeight = GetWeightFromFrame(SourceFrames[FrameIndex], MorphName);
				const float NextWeight = GetWeightFromFrame(SourceFrames[FrameIndex + 1], MorphName);
				const float SmoothedWeight = FMath::Lerp(CurrentWeight, (PreviousWeight + NextWeight) * 0.5f, Strength);
				Values.Add(MorphName, SmoothedWeight);
			}

			SetFrameWeights(Frames[FrameIndex], MorphNames, Values);
		}
	}
}
}

namespace DreamLipSyncFrameGeneration
{
FDreamLipSyncFrameGenerationSettings ResolveSettings(const UDreamLipSyncClip* Clip, const UDreamLipSyncSettings* Settings)
{
	if (Clip && Clip->bOverrideProjectFrameGenerationSettings)
	{
		return Clip->FrameGenerationSettings;
	}

	return Settings ? Settings->FrameGenerationSettings : FDreamLipSyncFrameGenerationSettings();
}

float ResolveBlendTime(const UDreamLipSyncClip* Clip, const UDreamLipSyncSettings* Settings)
{
	return Clip && Clip->GenerationBlendTime >= 0.f ? Clip->GenerationBlendTime : (Settings ? Settings->BlendTime : 0.04f);
}

void ApplyWeightSettings(TArray<FDreamLipSyncMorphWeight>& Weights, const FDreamLipSyncFrameGenerationSettings& Settings)
{
	for (int32 Index = Weights.Num() - 1; Index >= 0; --Index)
	{
		FDreamLipSyncMorphWeight& Weight = Weights[Index];
		Weight.Weight = FMath::Clamp(Weight.Weight * Settings.WeightScale, 0.f, FMath::Max(0.f, Settings.MaxWeight));
		if (Weight.MorphTargetName.IsNone() || Weight.Weight <= 0.0001f)
		{
			Weights.RemoveAtSwap(Index);
		}
	}
}

void PostProcessFrames(TArray<FDreamLipSyncMorphFrame>& Frames, float Duration, const FDreamLipSyncFrameGenerationSettings& Settings)
{
	SortAndClampFrames(Frames, Settings);
	if (Settings.bBakeFixedFrameRate)
	{
		BakeFixedFrameRate(Frames, Duration, Settings);
	}
	SmoothFrames(Frames, Settings);
	SortAndMergeFrames(Frames);
}
}
