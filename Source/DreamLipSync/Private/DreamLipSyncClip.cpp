// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#include "DreamLipSyncClip.h"

#include "Sound/SoundBase.h"

namespace DreamLipSyncClip
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

void AddClampedWeight(FName MorphTargetName, float Weight, TMap<FName, float>& OutWeights)
{
	if (MorphTargetName.IsNone())
	{
		return;
	}

	float& ExistingWeight = OutWeights.FindOrAdd(MorphTargetName);
	ExistingWeight = FMath::Clamp(ExistingWeight + Weight, 0.f, 1.f);
}

TArray<FDreamLipSyncMorphWeight> ConvertWeightMap(const TMap<FName, float>& WeightMap)
{
	TArray<FDreamLipSyncMorphWeight> Weights;
	Weights.Reserve(WeightMap.Num());

	for (const TPair<FName, float>& Pair : WeightMap)
	{
		FDreamLipSyncMorphWeight& Weight = Weights.AddDefaulted_GetRef();
		Weight.MorphTargetName = Pair.Key;
		Weight.Weight = Pair.Value;
	}

	return Weights;
}
}

UDreamLipSyncClip::UDreamLipSyncClip()
{
	VisemeMappings =
	{
		{ TEXT("Neutral"), TEXT("Fcl_MTH_Neutral"), 1.f },
		{ TEXT("A"), TEXT("Fcl_MTH_A"), 1.f },
		{ TEXT("I"), TEXT("Fcl_MTH_I"), 1.f },
		{ TEXT("U"), TEXT("Fcl_MTH_U"), 1.f },
		{ TEXT("E"), TEXT("Fcl_MTH_E"), 1.f },
		{ TEXT("O"), TEXT("Fcl_MTH_O"), 1.f },
		{ TEXT("Close"), TEXT("Fcl_MTH_Close"), 1.f }
	};

	SourceMorphMappings =
	{
		{ TEXT("jawOpen"), TEXT("Fcl_MTH_A"), 0.85f },
		{ TEXT("mouthClose"), TEXT("Fcl_MTH_Close"), 1.f },
		{ TEXT("mouthFunnel"), TEXT("Fcl_MTH_O"), 1.f },
		{ TEXT("mouthPucker"), TEXT("Fcl_MTH_U"), 1.f },
		{ TEXT("mouthStretchLeft"), TEXT("Fcl_MTH_E"), 0.5f },
		{ TEXT("mouthStretchRight"), TEXT("Fcl_MTH_E"), 0.5f },
		{ TEXT("mouthUpperUpLeft"), TEXT("Fcl_MTH_Up"), 0.5f },
		{ TEXT("mouthUpperUpRight"), TEXT("Fcl_MTH_Up"), 0.5f },
		{ TEXT("mouthLowerDownLeft"), TEXT("Fcl_MTH_Down"), 0.5f },
		{ TEXT("mouthLowerDownRight"), TEXT("Fcl_MTH_Down"), 0.5f }
	};
}

float UDreamLipSyncClip::GetResolvedDuration() const
{
	if (Duration > 0.f)
	{
		return Duration;
	}

	if (Sound)
	{
		const float SoundDuration = Sound->GetDuration();
		if (SoundDuration > 0.f && SoundDuration < 100000.f)
		{
			return SoundDuration;
		}
	}

	float LastTime = 0.f;
	for (const FDreamLipSyncVisemeKey& Key : VisemeKeys)
	{
		LastTime = FMath::Max(LastTime, Key.Time);
	}

	for (const FDreamLipSyncMorphFrame& Frame : MorphFrames)
	{
		LastTime = FMath::Max(LastTime, Frame.Time);
	}

	return LastTime;
}

void UDreamLipSyncClip::GetMorphWeightsAtTime(float Time, TArray<FDreamLipSyncMorphWeight>& OutWeights) const
{
	OutWeights.Reset();

	TMap<FName, float> WeightMap;
	EvaluateAtTime(Time, WeightMap);

	OutWeights.Reserve(WeightMap.Num());
	for (const TPair<FName, float>& Pair : WeightMap)
	{
		FDreamLipSyncMorphWeight& Weight = OutWeights.AddDefaulted_GetRef();
		Weight.MorphTargetName = Pair.Key;
		Weight.Weight = Pair.Value;
	}
}

void UDreamLipSyncClip::BuildMorphWeightsForViseme(FName Viseme, float Weight, TArray<FDreamLipSyncMorphWeight>& OutWeights) const
{
	OutWeights.Reset();

	TMap<FName, float> WeightMap;
	AddMappedVisemeWeight(Viseme, Weight, WeightMap);
	OutWeights = DreamLipSyncClip::ConvertWeightMap(WeightMap);
}

void UDreamLipSyncClip::GetControlledMorphTargetNames(TArray<FName>& OutMorphTargetNames) const
{
	OutMorphTargetNames.Reset();

	TSet<FName> Names;
	GetControlledMorphTargets(Names);

	OutMorphTargetNames.Reserve(Names.Num());
	for (const FName& Name : Names)
	{
		OutMorphTargetNames.Add(Name);
	}
}

void UDreamLipSyncClip::EvaluateAtTime(float Time, TMap<FName, float>& OutWeights) const
{
	OutWeights.Reset();

	if (DataMode == EDreamLipSyncDataMode::MorphFrames)
	{
		EvaluateMorphFrames(Time, OutWeights);
	}
	else
	{
		EvaluateVisemeKeys(Time, OutWeights);
	}
}

void UDreamLipSyncClip::GetControlledMorphTargets(TSet<FName>& OutMorphTargetNames) const
{
	OutMorphTargetNames.Reset();

	for (const FDreamLipSyncVisemeMapping& Mapping : VisemeMappings)
	{
		if (!Mapping.MorphTargetName.IsNone())
		{
			OutMorphTargetNames.Add(Mapping.MorphTargetName);
		}
	}

	for (const FDreamLipSyncMorphFrame& Frame : MorphFrames)
	{
		for (const FDreamLipSyncMorphWeight& Weight : Frame.Weights)
		{
			if (!Weight.MorphTargetName.IsNone())
			{
				OutMorphTargetNames.Add(Weight.MorphTargetName);
			}
		}
	}

	for (const FDreamLipSyncVisemeRecipe& Recipe : VisemeRecipes)
	{
		for (const FDreamLipSyncMorphWeight& Weight : Recipe.ExtraMorphWeights)
		{
			if (!Weight.MorphTargetName.IsNone())
			{
				OutMorphTargetNames.Add(Weight.MorphTargetName);
			}
		}
	}

	for (const FDreamLipSyncSourceMorphMapping& Mapping : SourceMorphMappings)
	{
		if (!Mapping.MorphTargetName.IsNone())
		{
			OutMorphTargetNames.Add(Mapping.MorphTargetName);
		}
	}
}

void UDreamLipSyncClip::EvaluateVisemeKeys(float Time, TMap<FName, float>& OutWeights) const
{
	TArray<const FDreamLipSyncVisemeKey*> SortedKeys;
	SortedKeys.Reserve(VisemeKeys.Num());

	for (const FDreamLipSyncVisemeKey& Key : VisemeKeys)
	{
		if (!Key.Viseme.IsNone())
		{
			SortedKeys.Add(&Key);
		}
	}

	SortedKeys.Sort([](const FDreamLipSyncVisemeKey& A, const FDreamLipSyncVisemeKey& B)
	{
		return A.Time < B.Time;
	});

	if (SortedKeys.IsEmpty())
	{
		return;
	}

	if (Time < SortedKeys[0]->Time)
	{
		return;
	}

	if (Time >= SortedKeys.Last()->Time)
	{
		if (bHoldLastKey || FMath::IsNearlyEqual(Time, SortedKeys.Last()->Time))
		{
			AddMappedVisemeWeight(SortedKeys.Last()->Viseme, SortedKeys.Last()->Weight, OutWeights);
		}
		return;
	}

	const FDreamLipSyncVisemeKey* PreviousKey = SortedKeys[0];
	const FDreamLipSyncVisemeKey* NextKey = SortedKeys[0];

	for (int32 Index = 1; Index < SortedKeys.Num(); ++Index)
	{
		if (Time <= SortedKeys[Index]->Time)
		{
			PreviousKey = SortedKeys[Index - 1];
			NextKey = SortedKeys[Index];
			break;
		}
	}

	const float Range = FMath::Max(KINDA_SMALL_NUMBER, NextKey->Time - PreviousKey->Time);
	const float BlendTime = FMath::Clamp(PlaybackBlendTime, 0.f, Range);
	const float BlendStartTime = NextKey->Time - BlendTime;

	if (BlendTime <= KINDA_SMALL_NUMBER || Time < BlendStartTime)
	{
		AddMappedVisemeWeight(PreviousKey->Viseme, PreviousKey->Weight, OutWeights);
		return;
	}

	const float Alpha = FMath::Clamp((Time - BlendStartTime) / BlendTime, 0.f, 1.f);

	AddMappedVisemeWeight(PreviousKey->Viseme, PreviousKey->Weight * (1.f - Alpha), OutWeights);
	AddMappedVisemeWeight(NextKey->Viseme, NextKey->Weight * Alpha, OutWeights);
}

void UDreamLipSyncClip::EvaluateMorphFrames(float Time, TMap<FName, float>& OutWeights) const
{
	TArray<const FDreamLipSyncMorphFrame*> SortedFrames;
	SortedFrames.Reserve(MorphFrames.Num());

	for (const FDreamLipSyncMorphFrame& Frame : MorphFrames)
	{
		SortedFrames.Add(&Frame);
	}

	SortedFrames.Sort([](const FDreamLipSyncMorphFrame& A, const FDreamLipSyncMorphFrame& B)
	{
		return A.Time < B.Time;
	});

	if (SortedFrames.IsEmpty())
	{
		return;
	}

	if (Time < SortedFrames[0]->Time)
	{
		return;
	}

	if (Time >= SortedFrames.Last()->Time)
	{
		if (bHoldLastKey || FMath::IsNearlyEqual(Time, SortedFrames.Last()->Time))
		{
			for (const FDreamLipSyncMorphWeight& Weight : SortedFrames.Last()->Weights)
			{
				DreamLipSyncClip::AddClampedWeight(Weight.MorphTargetName, Weight.Weight, OutWeights);
			}
		}
		return;
	}

	const FDreamLipSyncMorphFrame* PreviousFrame = SortedFrames[0];
	const FDreamLipSyncMorphFrame* NextFrame = SortedFrames[0];

	for (int32 Index = 1; Index < SortedFrames.Num(); ++Index)
	{
		if (Time <= SortedFrames[Index]->Time)
		{
			PreviousFrame = SortedFrames[Index - 1];
			NextFrame = SortedFrames[Index];
			break;
		}
	}

	TSet<FName> MorphNames;
	for (const FDreamLipSyncMorphWeight& Weight : PreviousFrame->Weights)
	{
		if (!Weight.MorphTargetName.IsNone())
		{
			MorphNames.Add(Weight.MorphTargetName);
		}
	}
	for (const FDreamLipSyncMorphWeight& Weight : NextFrame->Weights)
	{
		if (!Weight.MorphTargetName.IsNone())
		{
			MorphNames.Add(Weight.MorphTargetName);
		}
	}

	const float Range = FMath::Max(KINDA_SMALL_NUMBER, NextFrame->Time - PreviousFrame->Time);
	const float Alpha = FMath::Clamp((Time - PreviousFrame->Time) / Range, 0.f, 1.f);

	for (const FName& MorphName : MorphNames)
	{
		const float PreviousWeight = DreamLipSyncClip::GetWeightFromFrame(*PreviousFrame, MorphName);
		const float NextWeight = DreamLipSyncClip::GetWeightFromFrame(*NextFrame, MorphName);
		DreamLipSyncClip::AddClampedWeight(MorphName, FMath::Lerp(PreviousWeight, NextWeight, Alpha), OutWeights);
	}
}

void UDreamLipSyncClip::AddMappedVisemeWeight(FName Viseme, float Weight, TMap<FName, float>& OutWeights) const
{
	const FDreamLipSyncVisemeRecipe* Recipe = FindVisemeRecipe(Viseme);
	if (!Recipe)
	{
		AddRawMappedVisemeWeight(Viseme, Weight, OutWeights);
		return;
	}

	if (Recipe->bIncludeBaseViseme)
	{
		AddRawMappedVisemeWeight(Viseme, Weight * Recipe->BaseScale, OutWeights);
	}

	for (const FDreamLipSyncVisemeRecipeMix& Mix : Recipe->VisemeMixes)
	{
		if (!Mix.Viseme.IsNone())
		{
			AddRawMappedVisemeWeight(Mix.Viseme, Weight * Mix.Scale, OutWeights);
		}
	}

	for (const FDreamLipSyncMorphWeight& ExtraWeight : Recipe->ExtraMorphWeights)
	{
		DreamLipSyncClip::AddClampedWeight(ExtraWeight.MorphTargetName, Weight * ExtraWeight.Weight, OutWeights);
	}
}

void UDreamLipSyncClip::AddRawMappedVisemeWeight(FName Viseme, float Weight, TMap<FName, float>& OutWeights) const
{
	for (const FDreamLipSyncVisemeMapping& Mapping : VisemeMappings)
	{
		if (Mapping.Viseme == Viseme)
		{
			DreamLipSyncClip::AddClampedWeight(Mapping.MorphTargetName, Weight * Mapping.Scale, OutWeights);
		}
	}
}

const FDreamLipSyncVisemeRecipe* UDreamLipSyncClip::FindVisemeRecipe(FName Viseme) const
{
	if (Viseme.IsNone())
	{
		return nullptr;
	}

	for (const FDreamLipSyncVisemeRecipe& Recipe : VisemeRecipes)
	{
		if (Recipe.SourceViseme == Viseme)
		{
			return &Recipe;
		}
	}

	return nullptr;
}
