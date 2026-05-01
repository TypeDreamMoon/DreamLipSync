// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#include "DreamLipSyncRhubarbGenerator.h"

#include "Dom/JsonObject.h"
#include "DreamLipSyncClip.h"
#include "DreamLipSyncFrameGenerationUtils.h"
#include "DreamLipSyncProcessRunner.h"
#include "DreamLipSyncSettings.h"
#include "EditorFramework/AssetImportData.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ScopedTransaction.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundWave.h"

#define LOCTEXT_NAMESPACE "DreamLipSyncRhubarbGenerator"

namespace DreamLipSyncRhubarb
{
struct FCue
{
	float Start = 0.f;
	float End = 0.f;
	FName Viseme;
};

FString Quote(const FString& Value)
{
	FString Escaped = Value;
	Escaped.ReplaceInline(TEXT("\""), TEXT("\\\""));
	return FString::Printf(TEXT("\"%s\""), *Escaped);
}

FName MapRhubarbShapeToViseme(const FString& Shape)
{
	if (Shape.Equals(TEXT("A"), ESearchCase::IgnoreCase)) return TEXT("Close");
	if (Shape.Equals(TEXT("B"), ESearchCase::IgnoreCase)) return TEXT("I");
	if (Shape.Equals(TEXT("C"), ESearchCase::IgnoreCase)) return TEXT("E");
	if (Shape.Equals(TEXT("D"), ESearchCase::IgnoreCase)) return TEXT("A");
	if (Shape.Equals(TEXT("E"), ESearchCase::IgnoreCase)) return TEXT("O");
	if (Shape.Equals(TEXT("F"), ESearchCase::IgnoreCase)) return TEXT("U");
	if (Shape.Equals(TEXT("G"), ESearchCase::IgnoreCase)) return TEXT("E");
	if (Shape.Equals(TEXT("H"), ESearchCase::IgnoreCase)) return TEXT("A");
	if (Shape.Equals(TEXT("X"), ESearchCase::IgnoreCase)) return TEXT("Neutral");
	return FName(*Shape);
}

FString ResolveRecognizer(const UDreamLipSyncClip* Clip, const UDreamLipSyncSettings* Settings)
{
	EDreamLipSyncRhubarbRecognizer Recognizer = Settings ? Settings->RhubarbRecognizer : EDreamLipSyncRhubarbRecognizer::Auto;
	if (Clip && Clip->RhubarbGenerationSettings.bOverrideProjectSettings)
	{
		Recognizer = Clip->RhubarbGenerationSettings.Recognizer;
	}

	if (Recognizer == EDreamLipSyncRhubarbRecognizer::PocketSphinx)
	{
		return TEXT("pocketSphinx");
	}

	if (Recognizer == EDreamLipSyncRhubarbRecognizer::Phonetic)
	{
		return TEXT("phonetic");
	}

	const FString Locale = Clip ? Clip->Locale : FString();
	return Locale.StartsWith(TEXT("en"), ESearchCase::IgnoreCase) ? TEXT("pocketSphinx") : TEXT("phonetic");
}

TArray<FDreamLipSyncMorphWeight> BuildWeightsForViseme(const UDreamLipSyncClip* Clip, FName Viseme, const FDreamLipSyncFrameGenerationSettings& FrameSettings)
{
	if (!Clip)
	{
		return {};
	}

	TArray<FDreamLipSyncMorphWeight> Weights;
	Clip->BuildMorphWeightsForViseme(Viseme, 1.f, Weights);
	return Weights;
}

void AddFrame(TArray<FDreamLipSyncMorphFrame>& Frames, float Time, TArray<FDreamLipSyncMorphWeight>&& Weights)
{
	Time = FMath::Max(0.f, Time);
	if (!Frames.IsEmpty() && FMath::IsNearlyEqual(Frames.Last().Time, Time, 0.001f))
	{
		Frames.Last().Weights = MoveTemp(Weights);
		return;
	}

	FDreamLipSyncMorphFrame& Frame = Frames.AddDefaulted_GetRef();
	Frame.Time = Time;
	Frame.Weights = MoveTemp(Weights);
}

FCue ApplyFrameSettingsToCue(const FCue& Cue, const FDreamLipSyncFrameGenerationSettings& FrameSettings)
{
	FCue Result = Cue;
	Result.Start = FMath::Max(0.f, Result.Start + FrameSettings.TimeOffsetSeconds);
	Result.End = FMath::Max(Result.Start, Result.End + FrameSettings.TimeOffsetSeconds + FMath::Max(0.f, FrameSettings.CueEndPadding));

	const float MinDuration = FMath::Max(0.f, FrameSettings.MinCueDuration);
	if (MinDuration > 0.f && Result.End - Result.Start < MinDuration)
	{
		const float Center = (Result.Start + Result.End) * 0.5f;
		Result.Start = FMath::Max(0.f, Center - MinDuration * 0.5f);
		Result.End = Result.Start + MinDuration;
	}

	return Result;
}

TArray<FDreamLipSyncMorphFrame> BuildMorphFrames(const UDreamLipSyncClip* Clip, const TArray<FCue>& Cues, float BlendTime, const FDreamLipSyncFrameGenerationSettings& FrameSettings)
{
	TArray<FDreamLipSyncMorphFrame> Frames;
	Frames.Reserve(Cues.Num() * 3 + 2);

	float PreviousEnd = -1.f;
	for (const FCue& Cue : Cues)
	{
		const FCue AdjustedCue = ApplyFrameSettingsToCue(Cue, FrameSettings);
		if (AdjustedCue.End <= AdjustedCue.Start)
		{
			continue;
		}

		if (Frames.IsEmpty() && FrameSettings.bAddNeutralFrameAtStart && AdjustedCue.Start > 0.001f)
		{
			AddFrame(Frames, 0.f, BuildWeightsForViseme(Clip, FrameSettings.NeutralViseme, FrameSettings));
		}

		if (FrameSettings.bInsertNeutralFramesInGaps && PreviousEnd >= 0.f)
		{
			const float Gap = AdjustedCue.Start - PreviousEnd;
			if (Gap >= FrameSettings.NeutralGapThreshold)
			{
				const float NeutralBlendTime = FMath::Clamp(FrameSettings.NeutralBlendTime, 0.f, Gap * 0.5f);
				AddFrame(Frames, PreviousEnd + NeutralBlendTime, BuildWeightsForViseme(Clip, FrameSettings.NeutralViseme, FrameSettings));
				AddFrame(Frames, AdjustedCue.Start - NeutralBlendTime, BuildWeightsForViseme(Clip, FrameSettings.NeutralViseme, FrameSettings));
			}
		}

		AddFrame(Frames, AdjustedCue.Start, BuildWeightsForViseme(Clip, AdjustedCue.Viseme, FrameSettings));

		const float HoldEnd = AdjustedCue.End - FMath::Max(0.f, BlendTime);
		if (HoldEnd > AdjustedCue.Start + 0.001f)
		{
			AddFrame(Frames, HoldEnd, BuildWeightsForViseme(Clip, AdjustedCue.Viseme, FrameSettings));
		}

		PreviousEnd = AdjustedCue.End;
	}

	if (!Frames.IsEmpty() && FrameSettings.bAddNeutralFrameAtEnd)
	{
		AddFrame(Frames, FMath::Max(Frames.Last().Time, PreviousEnd) + FMath::Max(0.f, FrameSettings.NeutralBlendTime), BuildWeightsForViseme(Clip, FrameSettings.NeutralViseme, FrameSettings));
	}

	return Frames;
}

bool ParseRhubarbJson(const FString& JsonString, TArray<FCue>& OutCues, float& OutDuration, FString& OutMessage)
{
	TSharedPtr<FJsonObject> RootObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		OutMessage = TEXT("Failed to parse Rhubarb JSON.");
		return false;
	}

	const TSharedPtr<FJsonObject>* Metadata = nullptr;
	if (RootObject->TryGetObjectField(TEXT("metadata"), Metadata) && Metadata && Metadata->IsValid())
	{
		double Duration = 0.0;
		if ((*Metadata)->TryGetNumberField(TEXT("duration"), Duration))
		{
			OutDuration = static_cast<float>(Duration);
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* CueValues = nullptr;
	if (!RootObject->TryGetArrayField(TEXT("mouthCues"), CueValues) || !CueValues)
	{
		OutMessage = TEXT("Rhubarb JSON does not contain a mouthCues array.");
		return false;
	}

	for (const TSharedPtr<FJsonValue>& CueValue : *CueValues)
	{
		const TSharedPtr<FJsonObject> CueObject = CueValue.IsValid() ? CueValue->AsObject() : nullptr;
		if (!CueObject.IsValid())
		{
			continue;
		}

		double Start = 0.0;
		double End = 0.0;
		FString Shape;
		if (!CueObject->TryGetNumberField(TEXT("start"), Start) ||
			!CueObject->TryGetNumberField(TEXT("end"), End) ||
			!CueObject->TryGetStringField(TEXT("value"), Shape))
		{
			continue;
		}

		FCue& Cue = OutCues.AddDefaulted_GetRef();
		Cue.Start = static_cast<float>(Start);
		Cue.End = static_cast<float>(End);
		Cue.Viseme = MapRhubarbShapeToViseme(Shape);
	}

	if (OutCues.IsEmpty())
	{
		OutMessage = TEXT("No valid mouth cues were found in Rhubarb JSON.");
		return false;
	}

	if (OutDuration <= 0.f)
	{
		OutDuration = OutCues.Last().End;
	}

	return true;
}
}

bool FDreamLipSyncRhubarbGenerator::GenerateClipFromRhubarb(UDreamLipSyncClip* Clip, FString& OutMessage)
{
	if (!Clip)
	{
		OutMessage = TEXT("Invalid DreamLipSyncClip.");
		return false;
	}

	FString ExecutablePath;
	if (!ResolveRhubarbExecutable(ExecutablePath, OutMessage))
	{
		return false;
	}

	FString AudioFilePath;
	if (!ResolveInputAudioFile(Clip, AudioFilePath, OutMessage))
	{
		return false;
	}

	const UDreamLipSyncSettings* Settings = UDreamLipSyncSettings::Get();
	const FString Recognizer = DreamLipSyncRhubarb::ResolveRecognizer(Clip, Settings);
	const FString ExtendedShapes = Clip->RhubarbGenerationSettings.bOverrideProjectSettings
		? Clip->RhubarbGenerationSettings.ExtendedShapes
		: (Settings ? Settings->ExtendedShapes : TEXT("GHX"));

	const FString WorkDir = FPaths::ProjectIntermediateDir() / TEXT("DreamLipSync") / TEXT("Rhubarb");
	IFileManager::Get().MakeDirectory(*WorkDir, true);

	const FString OutputJsonPath = FPaths::CreateTempFilename(*WorkDir, TEXT("Rhubarb_"), TEXT(".json"));
	FString DialogFilePath;

	FString Params = FString::Printf(
		TEXT("-f json -o %s -r %s --extendedShapes %s"),
		*DreamLipSyncRhubarb::Quote(OutputJsonPath),
		*Recognizer,
		*DreamLipSyncRhubarb::Quote(ExtendedShapes));

	const FString DialogueText = Clip->DialogueText.ToString();
	if (!DialogueText.IsEmpty())
	{
		DialogFilePath = FPaths::CreateTempFilename(*WorkDir, TEXT("Dialog_"), TEXT(".txt"));
		if (FFileHelper::SaveStringToFile(DialogueText, *DialogFilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		{
			Params += FString::Printf(TEXT(" -d %s"), *DreamLipSyncRhubarb::Quote(DialogFilePath));
		}
	}

	Params += TEXT(" ") + DreamLipSyncRhubarb::Quote(AudioFilePath);

	FDreamLipSyncProcessResult ProcessResult;
	const bool bStarted = FDreamLipSyncProcessRunner::RunWithProgress(
		ExecutablePath,
		Params,
		WorkDir,
		FText::Format(LOCTEXT("GeneratingDreamLipSyncFromRhubarbProgress", "Generating lip sync for {0}"), FText::FromString(Clip->GetName())),
		LOCTEXT("RunningRhubarbProgress", "Running Rhubarb Lip Sync."),
		ProcessResult);

	if (ProcessResult.bCanceled)
	{
		OutMessage = TEXT("Rhubarb generation was canceled.");
		return false;
	}

	if (!bStarted || ProcessResult.ReturnCode != 0)
	{
		OutMessage = FString::Printf(TEXT("Rhubarb failed. ReturnCode=%d\n%s\n%s"), ProcessResult.ReturnCode, *ProcessResult.StdErr, *ProcessResult.StdOut);
		return false;
	}

	const bool bImported = ImportRhubarbJsonFile(Clip, OutputJsonPath, OutMessage);
	if (!bImported)
	{
		OutMessage += FString::Printf(TEXT("\nRhubarb stderr:\n%s"), *ProcessResult.StdErr);
		return false;
	}

	OutMessage = FString::Printf(TEXT("Generated %d MorphFrames from Rhubarb."), Clip->MorphFrames.Num());
	return true;
}

bool FDreamLipSyncRhubarbGenerator::ImportRhubarbJsonFile(UDreamLipSyncClip* Clip, const FString& JsonFilePath, FString& OutMessage)
{
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *JsonFilePath))
	{
		OutMessage = FString::Printf(TEXT("Could not read Rhubarb JSON file:\n%s"), *JsonFilePath);
		return false;
	}

	return ImportRhubarbJsonString(Clip, JsonString, OutMessage);
}

bool FDreamLipSyncRhubarbGenerator::ImportRhubarbJsonString(UDreamLipSyncClip* Clip, const FString& JsonString, FString& OutMessage)
{
	if (!Clip)
	{
		OutMessage = TEXT("Invalid DreamLipSyncClip.");
		return false;
	}

	TArray<DreamLipSyncRhubarb::FCue> Cues;
	float Duration = 0.f;
	if (!DreamLipSyncRhubarb::ParseRhubarbJson(JsonString, Cues, Duration, OutMessage))
	{
		return false;
	}

	const UDreamLipSyncSettings* Settings = UDreamLipSyncSettings::Get();
	const FDreamLipSyncFrameGenerationSettings FrameSettings = DreamLipSyncFrameGeneration::ResolveSettings(Clip, Settings);
	const float BlendTime = DreamLipSyncFrameGeneration::ResolveBlendTime(Clip, Settings);
	TArray<FDreamLipSyncMorphFrame> Frames = DreamLipSyncRhubarb::BuildMorphFrames(Clip, Cues, BlendTime, FrameSettings);
	DreamLipSyncFrameGeneration::PostProcessFrames(Frames, Duration, FrameSettings);
	if (Frames.IsEmpty())
	{
		OutMessage = TEXT("Rhubarb cues parsed, but no MorphFrames could be generated. Check VisemeMappings on the clip.");
		return false;
	}

	const FScopedTransaction Transaction(LOCTEXT("GenerateDreamLipSyncFromRhubarb", "Generate Dream Lip Sync From Rhubarb"));
	Clip->Modify();
	Clip->DataMode = EDreamLipSyncDataMode::MorphFrames;
	Clip->MorphFrames = MoveTemp(Frames);
	Clip->VisemeKeys.Reset();
	Clip->Duration = FMath::Max(Duration, Clip->MorphFrames.Last().Time);
	Clip->bHoldLastKey = false;
	Clip->MarkPackageDirty();

	OutMessage = FString::Printf(TEXT("Imported %d Rhubarb cues and generated %d MorphFrames."), Cues.Num(), Clip->MorphFrames.Num());
	return true;
}

bool FDreamLipSyncRhubarbGenerator::ResolveInputAudioFile(const UDreamLipSyncClip* Clip, FString& OutAudioFilePath, FString& OutMessage)
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
		OutMessage = TEXT("No readable source audio file found. Set SourceAudioFileOverride on the Clip, or use a SoundWave imported from a .wav/.ogg file.");
		return false;
	}

	const FString Extension = FPaths::GetExtension(OutAudioFilePath, false);
	if (!Extension.Equals(TEXT("wav"), ESearchCase::IgnoreCase) && !Extension.Equals(TEXT("ogg"), ESearchCase::IgnoreCase))
	{
		OutMessage = FString::Printf(TEXT("Rhubarb only supports .wav and .ogg input files. Current file:\n%s"), *OutAudioFilePath);
		return false;
	}

	return true;
}

bool FDreamLipSyncRhubarbGenerator::ResolveRhubarbExecutable(FString& OutExecutablePath, FString& OutMessage)
{
	const UDreamLipSyncSettings* Settings = UDreamLipSyncSettings::Get();
	OutExecutablePath = Settings ? Settings->RhubarbExecutablePath.FilePath : FString();

	if (OutExecutablePath.IsEmpty())
	{
		OutMessage = TEXT("Rhubarb executable path is empty. Set it in Project Settings -> DreamPlugin -> Dream Lip Sync.");
		return false;
	}

	OutExecutablePath = FPaths::ConvertRelativePathToFull(OutExecutablePath);
	if (!FPaths::FileExists(OutExecutablePath))
	{
		OutMessage = FString::Printf(TEXT("Rhubarb executable was not found:\n%s"), *OutExecutablePath);
		return false;
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
