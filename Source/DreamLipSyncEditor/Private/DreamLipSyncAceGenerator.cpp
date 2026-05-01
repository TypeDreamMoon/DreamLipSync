// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#include "DreamLipSyncAceGenerator.h"

#include "DreamLipSyncSettings.h"

#if WITH_DREAMLIPSYNC_ACE

#include "ACEAudioCurveSourceComponent.h"
#include "ACERuntimeModule.h"
#include "ACETypes.h"
#include "A2FProvider.h"
#include "Audio.h"
#include "AudioWaveFormatParser.h"
#include "DreamLipSyncClip.h"
#include "DreamLipSyncFrameGenerationUtils.h"
#include "EditorFramework/AssetImportData.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopedSlowTask.h"
#include "ScopedTransaction.h"
#include "Sound/SoundWave.h"

#include <atomic>

#define LOCTEXT_NAMESPACE "DreamLipSyncAceGenerator"

namespace DreamLipSyncAce
{
struct FSourceMapping
{
	FName MorphTargetName = NAME_None;
	float Scale = 1.f;
};

struct FAceFrame
{
	float Time = 0.f;
	TArray<float> Weights;
};

struct FWavAudioData
{
	TArray<int16> SamplesInt16;
	TArray<float> SamplesFloat;
	int32 NumChannels = 0;
	int32 SampleRate = 0;
	float Duration = 0.f;
	bool bFloat32 = false;
};

class FOfflineAceConsumer final : public IACEAnimDataConsumer
{
public:
	virtual void PrepareNewStream_AnyThread(int32 StreamID, uint32 InSampleRate, int32 InNumChannels, int32 InSampleByteSize) override
	{
		FScopeLock Lock(&DataCS);
		Frames.Reset();
		BlendShapeNames.Reset();
		FirstTimestamp.Reset();
		bCompleted = false;
		bUnexpectedOutput = false;
		SampleRate = InSampleRate;
		NumChannels = InNumChannels;
		SampleByteSize = InSampleByteSize;
		CurrentStreamID = StreamID;
	}

	virtual void ConsumeAnimData_AnyThread(const FACEAnimDataChunk& Chunk, int32 StreamID) override
	{
		FScopeLock Lock(&DataCS);
		CurrentStreamID = StreamID;

		if (Chunk.Status == EACEAnimDataStatus::OK_NO_MORE_DATA)
		{
			bCompleted = true;
			return;
		}

		if (Chunk.Status == EACEAnimDataStatus::ERROR_UNEXPECTED_OUTPUT)
		{
			bUnexpectedOutput = true;
		}

		if (!Chunk.BlendShapeNames.IsEmpty())
		{
			BlendShapeNames.Reset(Chunk.BlendShapeNames.Num());
			for (const FName& Name : Chunk.BlendShapeNames)
			{
				BlendShapeNames.Add(Name);
			}
		}

		if (Chunk.BlendShapeWeights.IsEmpty())
		{
			return;
		}

		if (!FirstTimestamp.IsSet())
		{
			FirstTimestamp = Chunk.Timestamp;
		}

		FAceFrame& Frame = Frames.AddDefaulted_GetRef();
		Frame.Time = FMath::Max(0.f, static_cast<float>(Chunk.Timestamp - FirstTimestamp.GetValue()));
		Frame.Weights.Reset(Chunk.BlendShapeWeights.Num());
		for (float Weight : Chunk.BlendShapeWeights)
		{
			Frame.Weights.Add(FMath::IsFinite(Weight) ? Weight : 0.f);
		}
	}

	bool IsCompleted() const
	{
		return bCompleted;
	}

	void CopyResult(TArray<FName>& OutBlendShapeNames, TArray<FAceFrame>& OutFrames, bool& bOutUnexpectedOutput) const
	{
		FScopeLock Lock(&DataCS);
		OutBlendShapeNames = BlendShapeNames;
		OutFrames = Frames;
		bOutUnexpectedOutput = bUnexpectedOutput;
	}

private:
	mutable FCriticalSection DataCS;
	TArray<FName> BlendShapeNames;
	TArray<FAceFrame> Frames;
	TOptional<double> FirstTimestamp;
	std::atomic_bool bCompleted{ false };
	bool bUnexpectedOutput = false;
	uint32 SampleRate = 0;
	int32 NumChannels = 0;
	int32 SampleByteSize = 0;
	int32 CurrentStreamID = INDEX_NONE;
};

void AddDefaultAceCurveNames(TArray<FName>& OutNames, int32 RequiredCount)
{
	const int32 DefaultCount = UE_ARRAY_COUNT(UACEAudioCurveSourceComponent::CurveNames);
	const int32 Count = RequiredCount > 0 ? FMath::Min(RequiredCount, DefaultCount) : DefaultCount;
	OutNames.Reset(Count);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		OutNames.Add(UACEAudioCurveSourceComponent::CurveNames[Index]);
	}
}

bool LoadWavAudioData(const FString& AudioFilePath, FWavAudioData& OutAudioData, FString& OutMessage)
{
	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *AudioFilePath))
	{
		OutMessage = FString::Printf(TEXT("Could not read audio file:\n%s"), *AudioFilePath);
		return false;
	}

	FWaveModInfo WaveInfo;
	FString Error;
	if (!WaveInfo.ReadWaveInfo(FileData.GetData(), FileData.Num(), &Error))
	{
		OutMessage = FString::Printf(TEXT("Could not parse WAV file:\n%s\n%s"), *AudioFilePath, *Error);
		return false;
	}

	OutAudioData.NumChannels = *WaveInfo.pChannels;
	OutAudioData.SampleRate = *WaveInfo.pSamplesPerSec;
	if (OutAudioData.NumChannels <= 0 || OutAudioData.NumChannels > 2 || OutAudioData.SampleRate <= 0)
	{
		OutMessage = FString::Printf(TEXT("ACE offline generation supports mono or stereo WAV files. Current channels=%d sampleRate=%d."),
			OutAudioData.NumChannels,
			OutAudioData.SampleRate);
		return false;
	}

	const uint16 FormatTag = *WaveInfo.pFormatTag;
	const uint16 BitsPerSample = *WaveInfo.pBitsPerSample;
	if (FormatTag == 1 && BitsPerSample == 16)
	{
		const int32 SampleCount = WaveInfo.SampleDataSize / sizeof(int16);
		OutAudioData.SamplesInt16.Append(reinterpret_cast<const int16*>(WaveInfo.SampleDataStart), SampleCount);
		OutAudioData.Duration = static_cast<float>(SampleCount) / static_cast<float>(OutAudioData.NumChannels * OutAudioData.SampleRate);
		return true;
	}

	if (FormatTag == 3 && BitsPerSample == 32)
	{
		const int32 SampleCount = WaveInfo.SampleDataSize / sizeof(float);
		OutAudioData.SamplesFloat.Append(reinterpret_cast<const float*>(WaveInfo.SampleDataStart), SampleCount);
		OutAudioData.Duration = static_cast<float>(SampleCount) / static_cast<float>(OutAudioData.NumChannels * OutAudioData.SampleRate);
		OutAudioData.bFloat32 = true;
		return true;
	}

	OutMessage = FString::Printf(TEXT("ACE offline generation supports PCM16 WAV and IEEE float32 WAV. Current formatTag=%u bitsPerSample=%u."),
		FormatTag,
		BitsPerSample);
	return false;
}

bool ResolveInputAudioFile(const UDreamLipSyncClip* Clip, FString& OutAudioFilePath, FString& OutMessage)
{
	if (!Clip)
	{
		OutMessage = TEXT("Invalid DreamLipSyncClip.");
		return false;
	}

	if (!Clip->SourceAudioFileOverride.FilePath.IsEmpty())
	{
		OutAudioFilePath = FPaths::ConvertRelativePathToFull(Clip->SourceAudioFileOverride.FilePath);
	}
	else if (const USoundWave* SoundWave = Cast<USoundWave>(Clip->Sound))
	{
		if (SoundWave->AssetImportData)
		{
			OutAudioFilePath = SoundWave->AssetImportData->GetFirstFilename();
		}
	}

	if (OutAudioFilePath.IsEmpty() || !FPaths::FileExists(OutAudioFilePath))
	{
		OutMessage = TEXT("No readable source audio file found. Set SourceAudioFileOverride on the Clip, or use a SoundWave imported from a .wav file.");
		return false;
	}

	return true;
}

FString JoinProviderNames(const TArray<FName>& ProviderNames)
{
	TArray<FString> Names;
	Names.Reserve(ProviderNames.Num());
	for (const FName& ProviderName : ProviderNames)
	{
		Names.Add(ProviderName.ToString());
	}

	return Names.IsEmpty() ? TEXT("<none>") : FString::Join(Names, TEXT(", "));
}

IA2FProvider* FindPreferredLocalProvider(FName& OutProviderName)
{
	TArray<FName> ProviderNames = IA2FProvider::GetAvailableProviderNames();
	ProviderNames.Sort([](const FName& A, const FName& B)
	{
		return A.LexicalLess(B);
	});

	for (const FName& ProviderName : ProviderNames)
	{
		if (!ProviderName.ToString().StartsWith(TEXT("LocalA2F"), ESearchCase::IgnoreCase))
		{
			continue;
		}

		if (IA2FProvider* Provider = IA2FProvider::FindProvider(ProviderName))
		{
			OutProviderName = ProviderName;
			return Provider;
		}
	}

	for (const FName& ProviderName : ProviderNames)
	{
		IA2FProvider* Provider = IA2FProvider::FindProvider(ProviderName);
		if (Provider && Provider->GetRemoteProvider() == nullptr)
		{
			OutProviderName = ProviderName;
			return Provider;
		}
	}

	return nullptr;
}

bool ResolveAceProvider(FName RequestedProviderName, IA2FProvider*& OutProvider, FName& OutResolvedProviderName, FString& OutMessage)
{
	const bool bRequestedDefaultProvider = RequestedProviderName.IsNone() || RequestedProviderName == FName(TEXT("Default"));
	OutResolvedProviderName = RequestedProviderName;
	if (bRequestedDefaultProvider)
	{
		OutResolvedProviderName = GetDefaultProviderName();
	}

	OutProvider = GetProviderFromName(RequestedProviderName);
	if (!OutProvider)
	{
		OutMessage = FString::Printf(
			TEXT("NVIDIA ACE provider '%s' is not registered. Resolved provider: '%s'. Available providers: %s."),
			*RequestedProviderName.ToString(),
			*OutResolvedProviderName.ToString(),
			*JoinProviderNames(IA2FProvider::GetAvailableProviderNames()));
		return false;
	}

	if (IA2FRemoteProvider* RemoteProvider = OutProvider->GetRemoteProvider())
	{
		const FACEConnectionInfo ConnectionInfo = RemoteProvider->GetConnectionInfo();
		if (ConnectionInfo.DestURL.TrimStartAndEnd().IsEmpty())
		{
			if (bRequestedDefaultProvider)
			{
				FName LocalProviderName = NAME_None;
				if (IA2FProvider* LocalProvider = FindPreferredLocalProvider(LocalProviderName))
				{
					OutProvider = LocalProvider;
					OutResolvedProviderName = LocalProviderName;
					return true;
				}
			}

			OutMessage = FString::Printf(
				TEXT("NVIDIA ACE provider '%s' resolved to remote provider '%s', but its A2F-3D Server URL is empty.\n\nSet it in Project Settings -> Plugins -> NVIDIA ACE -> Default A2F-3D Server Config, or set Dream Lip Sync -> Ace Provider Name to a local provider such as LocalA2F-James.\n\nAvailable providers: %s."),
				*RequestedProviderName.ToString(),
				*OutResolvedProviderName.ToString(),
				*JoinProviderNames(IA2FProvider::GetAvailableProviderNames()));
			return false;
		}
	}

	return true;
}

FAudio2FaceEmotion ConvertEmotionParameters(const FDreamLipSyncAceEmotionParameters& Source)
{
	FAudio2FaceEmotion Result;
	Result.OverallEmotionStrength = Source.OverallEmotionStrength;
	Result.DetectedEmotionContrast = Source.DetectedEmotionContrast;
	Result.MaxDetectedEmotions = Source.MaxDetectedEmotions;
	Result.DetectedEmotionSmoothing = Source.DetectedEmotionSmoothing;
	Result.bEnableEmotionOverride = Source.bEnableEmotionOverride;
	Result.EmotionOverrideStrength = Source.EmotionOverrideStrength;

	Result.EmotionOverrides.bOverrideAmazement = Source.EmotionOverrides.bOverrideAmazement;
	Result.EmotionOverrides.Amazement = Source.EmotionOverrides.Amazement;
	Result.EmotionOverrides.bOverrideAnger = Source.EmotionOverrides.bOverrideAnger;
	Result.EmotionOverrides.Anger = Source.EmotionOverrides.Anger;
	Result.EmotionOverrides.bOverrideCheekiness = Source.EmotionOverrides.bOverrideCheekiness;
	Result.EmotionOverrides.Cheekiness = Source.EmotionOverrides.Cheekiness;
	Result.EmotionOverrides.bOverrideDisgust = Source.EmotionOverrides.bOverrideDisgust;
	Result.EmotionOverrides.Disgust = Source.EmotionOverrides.Disgust;
	Result.EmotionOverrides.bOverrideFear = Source.EmotionOverrides.bOverrideFear;
	Result.EmotionOverrides.Fear = Source.EmotionOverrides.Fear;
	Result.EmotionOverrides.bOverrideGrief = Source.EmotionOverrides.bOverrideGrief;
	Result.EmotionOverrides.Grief = Source.EmotionOverrides.Grief;
	Result.EmotionOverrides.bOverrideJoy = Source.EmotionOverrides.bOverrideJoy;
	Result.EmotionOverrides.Joy = Source.EmotionOverrides.Joy;
	Result.EmotionOverrides.bOverrideOutOfBreath = Source.EmotionOverrides.bOverrideOutOfBreath;
	Result.EmotionOverrides.OutOfBreath = Source.EmotionOverrides.OutOfBreath;
	Result.EmotionOverrides.bOverridePain = Source.EmotionOverrides.bOverridePain;
	Result.EmotionOverrides.Pain = Source.EmotionOverrides.Pain;
	Result.EmotionOverrides.bOverrideSadness = Source.EmotionOverrides.bOverrideSadness;
	Result.EmotionOverrides.Sadness = Source.EmotionOverrides.Sadness;

	return Result;
}

TOptional<FAudio2FaceEmotion> ResolveEmotionParameters(const UDreamLipSyncClip* Clip, const UDreamLipSyncSettings* Settings)
{
	const bool bUseClipAceSettings = Clip && Clip->AceGenerationSettings.bOverrideProjectSettings;
	const bool bUseEmotionParameters = bUseClipAceSettings
		? Clip->AceGenerationSettings.bAceUseEmotionParameters
		: (Settings && Settings->bAceUseEmotionParameters);

	if (!bUseEmotionParameters)
	{
		return TOptional<FAudio2FaceEmotion>();
	}

	const FDreamLipSyncAceEmotionParameters& Source = bUseClipAceSettings
		? Clip->AceGenerationSettings.AceEmotionParameters
		: Settings->AceEmotionParameters;

	return ConvertEmotionParameters(Source);
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

void BuildMappingLookup(const UDreamLipSyncClip* Clip, TMap<FName, TArray<FSourceMapping>>& OutMappingLookup, TSet<FName>& OutKnownTargetMorphs)
{
	if (!Clip)
	{
		return;
	}

	for (const FDreamLipSyncVisemeMapping& Mapping : Clip->VisemeMappings)
	{
		if (!Mapping.MorphTargetName.IsNone())
		{
			OutKnownTargetMorphs.Add(Mapping.MorphTargetName);
		}
	}

	for (const FDreamLipSyncSourceMorphMapping& Mapping : Clip->SourceMorphMappings)
	{
		if (Mapping.SourceName.IsNone() || Mapping.MorphTargetName.IsNone())
		{
			continue;
		}

		FSourceMapping& Target = OutMappingLookup.FindOrAdd(Mapping.SourceName).AddDefaulted_GetRef();
		Target.MorphTargetName = Mapping.MorphTargetName;
		Target.Scale = Mapping.Scale;
		OutKnownTargetMorphs.Add(Mapping.MorphTargetName);
	}
}

TArray<FDreamLipSyncMorphFrame> BuildMorphFrames(const UDreamLipSyncClip* Clip, const TArray<FName>& SourceNames, const TArray<FAceFrame>& AceFrames)
{
	TMap<FName, TArray<FSourceMapping>> MappingLookup;
	TSet<FName> KnownTargetMorphs;
	BuildMappingLookup(Clip, MappingLookup, KnownTargetMorphs);

	TArray<FDreamLipSyncMorphFrame> Frames;
	Frames.Reserve(AceFrames.Num());

	for (const FAceFrame& AceFrame : AceFrames)
	{
		TMap<FName, float> AccumulatedWeights;
		const int32 ChannelCount = FMath::Min(SourceNames.Num(), AceFrame.Weights.Num());
		for (int32 ChannelIndex = 0; ChannelIndex < ChannelCount; ++ChannelIndex)
		{
			const FName SourceName = SourceNames[ChannelIndex];
			const float SourceWeight = FMath::Max(0.f, AceFrame.Weights[ChannelIndex]);

			if (const TArray<FSourceMapping>* Mappings = MappingLookup.Find(SourceName))
			{
				for (const FSourceMapping& Mapping : *Mappings)
				{
					AddClampedWeight(Mapping.MorphTargetName, SourceWeight * Mapping.Scale, AccumulatedWeights);
				}
				continue;
			}

			if (KnownTargetMorphs.Contains(SourceName))
			{
				AddClampedWeight(SourceName, SourceWeight, AccumulatedWeights);
			}
		}

		FDreamLipSyncMorphFrame& Frame = Frames.AddDefaulted_GetRef();
		Frame.Time = AceFrame.Time;
		Frame.Weights = ConvertWeightMap(AccumulatedWeights);
	}

	return Frames;
}
}

bool FDreamLipSyncAceGenerator::IsAceAvailable()
{
	const UDreamLipSyncSettings* Settings = UDreamLipSyncSettings::Get();
	return !Settings || Settings->bEnableAceGeneration;
}

bool FDreamLipSyncAceGenerator::GenerateClipFromAce(UDreamLipSyncClip* Clip, FString& OutMessage)
{
	if (!Clip)
	{
		OutMessage = TEXT("Invalid DreamLipSyncClip.");
		return false;
	}

	const UDreamLipSyncSettings* Settings = UDreamLipSyncSettings::Get();
	if (Settings && !Settings->bEnableAceGeneration)
	{
		OutMessage = TEXT("NVIDIA ACE generation is disabled in Project Settings -> Plugins -> Dream Lip Sync.");
		return false;
	}

	FString AudioFilePath;
	if (!DreamLipSyncAce::ResolveInputAudioFile(Clip, AudioFilePath, OutMessage))
	{
		return false;
	}

	if (!FPaths::GetExtension(AudioFilePath, false).Equals(TEXT("wav"), ESearchCase::IgnoreCase))
	{
		OutMessage = FString::Printf(TEXT("NVIDIA ACE offline generation currently requires a WAV source file. Current file:\n%s"), *AudioFilePath);
		return false;
	}

	DreamLipSyncAce::FWavAudioData AudioData;
	if (!DreamLipSyncAce::LoadWavAudioData(AudioFilePath, AudioData, OutMessage))
	{
		return false;
	}

	const bool bUseClipAceSettings = Clip->AceGenerationSettings.bOverrideProjectSettings;
	const FName ProviderName = bUseClipAceSettings ? Clip->AceGenerationSettings.AceProviderName : (Settings ? Settings->AceProviderName : FName(TEXT("Default")));
	const float TimeoutSeconds = bUseClipAceSettings
		? FMath::Max(1.f, Clip->AceGenerationSettings.AceGenerationTimeoutSeconds)
		: (Settings ? FMath::Max(1.f, Settings->AceGenerationTimeoutSeconds) : 300.f);
	const bool bForceBurstMode = bUseClipAceSettings ? Clip->AceGenerationSettings.bAceForceBurstMode : (!Settings || Settings->bAceForceBurstMode);
	const TOptional<FAudio2FaceEmotion> EmotionParameters = DreamLipSyncAce::ResolveEmotionParameters(Clip, Settings);

	IA2FProvider* Provider = nullptr;
	FName ResolvedProviderName = NAME_None;
	if (!DreamLipSyncAce::ResolveAceProvider(ProviderName, Provider, ResolvedProviderName, OutMessage))
	{
		return false;
	}

	DreamLipSyncAce::FOfflineAceConsumer Consumer;
	FACERuntimeModule& AceRuntime = FACERuntimeModule::Get();
	const TOptional<bool> PreviousBurstMode = AceRuntime.bOverrideBurstMode;
	if (bForceBurstMode)
	{
		AceRuntime.bOverrideBurstMode = true;
	}

	bool bSent = false;
	{
		FScopedSlowTask SlowTask(100.f, FText::Format(LOCTEXT("GeneratingDreamLipSyncFromAce", "Generating NVIDIA ACE lip sync for {0}"), FText::FromString(Clip->GetName())));
		SlowTask.MakeDialog(true);
		SlowTask.EnterProgressFrame(5.f, LOCTEXT("SendingAudioToAce", "Sending audio to NVIDIA ACE."));

		if (AudioData.bFloat32)
		{
			bSent = AceRuntime.AnimateFromAudioSamples(&Consumer, TArrayView<const float>(AudioData.SamplesFloat.GetData(), AudioData.SamplesFloat.Num()), AudioData.NumChannels, AudioData.SampleRate, true, EmotionParameters, nullptr, ResolvedProviderName);
		}
		else
		{
			bSent = AceRuntime.AnimateFromAudioSamples(&Consumer, TArrayView<const int16>(AudioData.SamplesInt16.GetData(), AudioData.SamplesInt16.Num()), AudioData.NumChannels, AudioData.SampleRate, true, EmotionParameters, nullptr, ResolvedProviderName);
		}

		if (!bSent)
		{
			AceRuntime.bOverrideBurstMode = PreviousBurstMode;
			OutMessage = FString::Printf(
				TEXT("NVIDIA ACE failed to accept audio. Requested provider=%s, resolved provider=%s. Check the NVIDIA ACE log for the provider connection or model load error."),
				*ProviderName.ToString(),
				*ResolvedProviderName.ToString());
			return false;
		}

		const double StartSeconds = FPlatformTime::Seconds();
		while (!Consumer.IsCompleted())
		{
			if (SlowTask.ShouldCancel())
			{
				AceRuntime.CancelAnimationGeneration(&Consumer);
				AceRuntime.bOverrideBurstMode = PreviousBurstMode;
				OutMessage = TEXT("NVIDIA ACE generation was canceled.");
				return false;
			}

			const double ElapsedSeconds = FPlatformTime::Seconds() - StartSeconds;
			if (ElapsedSeconds > TimeoutSeconds)
			{
				AceRuntime.CancelAnimationGeneration(&Consumer);
				AceRuntime.bOverrideBurstMode = PreviousBurstMode;
				OutMessage = FString::Printf(TEXT("NVIDIA ACE generation timed out after %.1f seconds."), TimeoutSeconds);
				return false;
			}

			SlowTask.EnterProgressFrame(0.25f, LOCTEXT("WaitingForAceFrames", "Waiting for NVIDIA ACE animation frames."));
			FPlatformProcess::Sleep(0.05f);
		}
	}

	AceRuntime.bOverrideBurstMode = PreviousBurstMode;

	TArray<FName> SourceNames;
	TArray<DreamLipSyncAce::FAceFrame> AceFrames;
	bool bUnexpectedOutput = false;
	Consumer.CopyResult(SourceNames, AceFrames, bUnexpectedOutput);

	if (AceFrames.IsEmpty())
	{
		OutMessage = TEXT("NVIDIA ACE completed, but no blendshape frames were received.");
		return false;
	}

	if (SourceNames.IsEmpty())
	{
		DreamLipSyncAce::AddDefaultAceCurveNames(SourceNames, AceFrames[0].Weights.Num());
	}

	TArray<FDreamLipSyncMorphFrame> Frames = DreamLipSyncAce::BuildMorphFrames(Clip, SourceNames, AceFrames);
	const FDreamLipSyncFrameGenerationSettings FrameSettings = DreamLipSyncFrameGeneration::ResolveSettings(Clip, Settings);
	if (!FMath::IsNearlyZero(FrameSettings.TimeOffsetSeconds))
	{
		for (FDreamLipSyncMorphFrame& Frame : Frames)
		{
			Frame.Time = FMath::Max(0.f, Frame.Time + FrameSettings.TimeOffsetSeconds);
		}
	}
	DreamLipSyncFrameGeneration::PostProcessFrames(Frames, AudioData.Duration, FrameSettings);
	if (Frames.IsEmpty())
	{
		OutMessage = TEXT("NVIDIA ACE frames were received, but no MorphFrames could be generated.");
		return false;
	}

	int32 WeightedFrameCount = 0;
	for (const FDreamLipSyncMorphFrame& Frame : Frames)
	{
		if (!Frame.Weights.IsEmpty())
		{
			++WeightedFrameCount;
		}
	}

	if (WeightedFrameCount == 0)
	{
		OutMessage = TEXT("NVIDIA ACE frames were received, but none of its curve names matched SourceMorphMappings or known target MorphTargets.");
		return false;
	}

	const FScopedTransaction Transaction(LOCTEXT("GenerateDreamLipSyncFromAceTransaction", "Generate Dream Lip Sync From NVIDIA ACE"));
	Clip->Modify();
	Clip->DataMode = EDreamLipSyncDataMode::MorphFrames;
	Clip->MorphFrames = MoveTemp(Frames);
	Clip->VisemeKeys.Reset();
	Clip->Duration = FMath::Max(AudioData.Duration, Clip->MorphFrames.Last().Time);
	Clip->bHoldLastKey = false;
	Clip->MarkPackageDirty();

	OutMessage = FString::Printf(TEXT("Generated %d MorphFrames from NVIDIA ACE using provider %s.%s"),
		Clip->MorphFrames.Num(),
		*ResolvedProviderName.ToString(),
		bUnexpectedOutput ? TEXT("\nACE reported unexpected output for at least one chunk; generated frames were still imported.") : TEXT(""));
	return true;
}

#undef LOCTEXT_NAMESPACE

#else

bool FDreamLipSyncAceGenerator::IsAceAvailable()
{
	return false;
}

bool FDreamLipSyncAceGenerator::GenerateClipFromAce(UDreamLipSyncClip* Clip, FString& OutMessage)
{
	OutMessage = TEXT("NVIDIA ACE generation is unavailable because the NV_ACE_Reference plugin is not available at build time. DreamLipSync playback and non-ACE generators still work.");
	return false;
}

#endif
