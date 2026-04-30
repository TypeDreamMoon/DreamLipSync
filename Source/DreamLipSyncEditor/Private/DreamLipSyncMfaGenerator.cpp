// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#include "DreamLipSyncMfaGenerator.h"

#include "DreamLipSyncClip.h"
#include "DreamLipSyncProcessRunner.h"
#include "DreamLipSyncSettings.h"
#include "EditorFramework/AssetImportData.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "ScopedTransaction.h"
#include "Sound/SoundWave.h"

#define LOCTEXT_NAMESPACE "DreamLipSyncMfaGenerator"

namespace DreamLipSyncMfa
{
struct FInterval
{
	float Start = 0.f;
	float End = 0.f;
	FString Text;
};

struct FTier
{
	FString Name;
	FString ClassName;
	TArray<FInterval> Intervals;
};

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

void AppendOption(FString& Params, const TCHAR* OptionName, const FString& Value, bool bQuoteValue = true)
{
	FString TrimmedValue = Value;
	TrimmedValue.TrimStartAndEndInline();
	if (TrimmedValue.IsEmpty())
	{
		return;
	}

	Params += FString::Printf(TEXT(" %s %s"), OptionName, bQuoteValue ? *Quote(TrimmedValue) : *TrimmedValue);
}

void AppendBoolOption(FString& Params, const TCHAR* EnabledOptionName, const TCHAR* DisabledOptionName, bool bEnabled)
{
	Params += FString::Printf(TEXT(" %s"), bEnabled ? EnabledOptionName : DisabledOptionName);
}

FString ResolveOptionalPath(const FString& Value)
{
	if (Value.IsEmpty())
	{
		return FString();
	}

	const bool bLooksLikePath = Value.Contains(TEXT("\\")) || Value.Contains(TEXT("/")) || FPaths::FileExists(Value) || FPaths::DirectoryExists(Value);
	return bLooksLikePath ? FPaths::ConvertRelativePathToFull(Value) : Value;
}

void AppendMfaOptions(FString& Params, const UDreamLipSyncSettings* Settings)
{
	if (!Settings)
	{
		Params += TEXT(" -j 1 --clean --overwrite --quiet --use_mp --no_use_threading --no_use_postgres --textgrid_cleanup --output_format long_textgrid");
		return;
	}

	AppendOption(Params, TEXT("--config_path"), ResolveOptionalPath(Settings->MfaConfigPath.FilePath));
	AppendOption(Params, TEXT("--speaker_characters"), Settings->MfaSpeakerCharacters, false);
	AppendOption(Params, TEXT("--g2p_model_path"), ResolveOptionalPath(Settings->MfaG2PModelPath));
	AppendOption(Params, TEXT("--temporary_directory"), ResolveOptionalPath(Settings->MfaRootDirectory.Path));

	Params += FString::Printf(TEXT(" -j %d"), FMath::Max(1, Settings->MfaNumJobs));
	AppendBoolOption(Params, TEXT("--clean"), TEXT("--no_clean"), Settings->bMfaClean);
	AppendBoolOption(Params, TEXT("--overwrite"), TEXT("--no_overwrite"), Settings->bMfaOverwrite);
	AppendBoolOption(Params, TEXT("--final_clean"), TEXT("--no_final_clean"), Settings->bMfaFinalClean);
	AppendBoolOption(Params, TEXT("--use_mp"), TEXT("--no_use_mp"), Settings->bMfaUseMultiprocessing);
	AppendBoolOption(Params, TEXT("--use_threading"), TEXT("--no_use_threading"), Settings->bMfaUseThreading);
	AppendBoolOption(Params, TEXT("--use_postgres"), TEXT("--no_use_postgres"), Settings->bMfaUsePostgres);
	AppendBoolOption(Params, TEXT("--textgrid_cleanup"), TEXT("--no_textgrid_cleanup"), Settings->bMfaTextGridCleanup);

	if (Settings->bMfaQuiet)
	{
		Params += TEXT(" --quiet --no_verbose");
	}
	else
	{
		AppendBoolOption(Params, TEXT("--verbose"), TEXT("--no_verbose"), Settings->bMfaVerbose);
		Params += TEXT(" --no_quiet");
	}

	if (Settings->bMfaSingleSpeaker)
	{
		Params += TEXT(" --single_speaker");
	}

	if (Settings->bMfaNoTokenization)
	{
		Params += TEXT(" --no_tokenization");
	}

	if (Settings->bMfaIncludeOriginalText)
	{
		Params += TEXT(" --include_original_text");
	}

	if (Settings->bMfaFineTune)
	{
		Params += TEXT(" --fine_tune");
		if (Settings->MfaFineTuneBoundaryTolerance > 0.f)
		{
			Params += FString::Printf(TEXT(" --fine_tune_boundary_tolerance %.4f"), Settings->MfaFineTuneBoundaryTolerance);
		}
	}

	Params += TEXT(" --output_format long_textgrid");

	const FString ExtraArguments = Settings->MfaExtraArguments.TrimStartAndEnd();
	if (!ExtraArguments.IsEmpty())
	{
		Params += TEXT(" ") + ExtraArguments;
	}
}

FString SanitizeFileBase(const FString& Value)
{
	FString Result = Value;
	const TCHAR InvalidChars[] = TEXT("\\/:*?\"<>|");
	for (int32 CharIndex = 0; InvalidChars[CharIndex] != 0; ++CharIndex)
	{
		Result.ReplaceCharInline(InvalidChars[CharIndex], TEXT('_'));
	}

	Result.TrimStartAndEndInline();
	return Result.IsEmpty() ? TEXT("DreamLipSync") : Result;
}

FString ParseValueAfterEquals(const FString& Line)
{
	FString Left;
	FString Right;
	if (Line.Split(TEXT("="), &Left, &Right))
	{
		Right.TrimStartAndEndInline();
		return Right;
	}

	return FString();
}

FString ParseTextGridStringValue(const FString& Line)
{
	FString Value = ParseValueAfterEquals(Line);
	if (Value.StartsWith(TEXT("\"")) && Value.EndsWith(TEXT("\"")) && Value.Len() >= 2)
	{
		Value = Value.Mid(1, Value.Len() - 2);
		Value.ReplaceInline(TEXT("\"\""), TEXT("\""));
	}

	return Value;
}

float ParseTextGridNumberValue(const FString& Line)
{
	return static_cast<float>(FCString::Atod(*ParseValueAfterEquals(Line)));
}

FString NormalizePhone(FString Phone)
{
	Phone.TrimStartAndEndInline();
	Phone.ToLowerInline();

	if (Phone.StartsWith(TEXT("\"")) && Phone.EndsWith(TEXT("\"")) && Phone.Len() >= 2)
	{
		Phone = Phone.Mid(1, Phone.Len() - 2);
	}

	FString Normalized;
	for (TCHAR Character : Phone)
	{
		if (FChar::IsDigit(Character))
		{
			continue;
		}

		if (FChar::IsWhitespace(Character))
		{
			continue;
		}

		if (Character == TEXT('_') || Character == TEXT('-') || Character == TEXT('.') || Character == TEXT(':'))
		{
			continue;
		}

		Normalized.AppendChar(Character);
	}

	return Normalized;
}

bool IsSilencePhone(const FString& Phone)
{
	return Phone.IsEmpty() ||
		Phone == TEXT("sil") ||
		Phone == TEXT("sp") ||
		Phone == TEXT("spn") ||
		Phone == TEXT("silence") ||
		Phone == TEXT("eps") ||
		Phone == TEXT("<eps>");
}

FName MapPhoneToViseme(const FString& RawPhone)
{
	const FString Phone = NormalizePhone(RawPhone);
	if (IsSilencePhone(Phone))
	{
		return TEXT("Neutral");
	}

	if (Phone.StartsWith(TEXT("b")) || Phone.StartsWith(TEXT("p")) || Phone.StartsWith(TEXT("m")) ||
		Phone.StartsWith(TEXT("f")) || Phone.StartsWith(TEXT("v")))
	{
		return TEXT("Close");
	}

	if (Phone.StartsWith(TEXT("u")) || Phone.StartsWith(TEXT("w")) || Phone.StartsWith(TEXT("v")) ||
		Phone.Contains(TEXT("uu")) || Phone.Contains(TEXT("uo")) || Phone.Contains(TEXT("ua")))
	{
		return TEXT("U");
	}

	if (Phone.StartsWith(TEXT("o")) || Phone.Contains(TEXT("ou")) || Phone.Contains(TEXT("ong")))
	{
		return TEXT("O");
	}

	if (Phone.StartsWith(TEXT("i")) || Phone.StartsWith(TEXT("y")) || Phone.Contains(TEXT("ii")) ||
		Phone.Contains(TEXT("ih")) || Phone.Contains(TEXT("iy")))
	{
		return TEXT("I");
	}

	if (Phone.StartsWith(TEXT("e")) || Phone.StartsWith(TEXT("er")) || Phone.Contains(TEXT("ei")) ||
		Phone.Contains(TEXT("eh")) || Phone.Contains(TEXT("en")) || Phone.Contains(TEXT("eng")))
	{
		return TEXT("E");
	}

	if (Phone.Contains(TEXT("a")) || Phone.Contains(TEXT("aa")) || Phone.Contains(TEXT("ae")) || Phone.Contains(TEXT("ao")))
	{
		return TEXT("A");
	}

	if (Phone.Contains(TEXT("o")))
	{
		return TEXT("O");
	}

	if (Phone.Contains(TEXT("i")))
	{
		return TEXT("I");
	}

	if (Phone.Contains(TEXT("u")))
	{
		return TEXT("U");
	}

	if (Phone.Contains(TEXT("e")))
	{
		return TEXT("E");
	}

	if (Phone.StartsWith(TEXT("s")) || Phone.StartsWith(TEXT("z")) || Phone.StartsWith(TEXT("c")) ||
		Phone.StartsWith(TEXT("sh")) || Phone.StartsWith(TEXT("zh")) || Phone.StartsWith(TEXT("ch")) ||
		Phone.StartsWith(TEXT("j")) || Phone.StartsWith(TEXT("q")) || Phone.StartsWith(TEXT("x")) ||
		Phone.StartsWith(TEXT("r")))
	{
		return TEXT("E");
	}

	return TEXT("Neutral");
}

bool ParseTextGrid(const FString& TextGridString, TArray<FInterval>& OutIntervals, float& OutDuration, FString& OutMessage)
{
	TArray<FString> Lines;
	TextGridString.ParseIntoArrayLines(Lines, false);

	TArray<FTier> Tiers;
	int32 CurrentTierIndex = INDEX_NONE;
	int32 CurrentIntervalIndex = INDEX_NONE;

	for (FString Line : Lines)
	{
		Line.TrimStartAndEndInline();
		if (Line.IsEmpty())
		{
			continue;
		}

		if (Line.StartsWith(TEXT("item [")) && !Line.StartsWith(TEXT("item []")))
		{
			CurrentTierIndex = Tiers.AddDefaulted();
			CurrentIntervalIndex = INDEX_NONE;
			continue;
		}

		if (!Tiers.IsValidIndex(CurrentTierIndex))
		{
			continue;
		}

		FTier& CurrentTier = Tiers[CurrentTierIndex];
		if (Line.StartsWith(TEXT("class =")))
		{
			CurrentTier.ClassName = ParseTextGridStringValue(Line);
			continue;
		}

		if (Line.StartsWith(TEXT("name =")))
		{
			CurrentTier.Name = ParseTextGridStringValue(Line);
			continue;
		}

		if (Line.StartsWith(TEXT("intervals [")))
		{
			CurrentIntervalIndex = CurrentTier.Intervals.AddDefaulted();
			continue;
		}

		if (CurrentTier.Intervals.IsValidIndex(CurrentIntervalIndex))
		{
			FInterval& CurrentInterval = CurrentTier.Intervals[CurrentIntervalIndex];
			if (Line.StartsWith(TEXT("xmin =")))
			{
				CurrentInterval.Start = ParseTextGridNumberValue(Line);
				continue;
			}

			if (Line.StartsWith(TEXT("xmax =")))
			{
				CurrentInterval.End = ParseTextGridNumberValue(Line);
				OutDuration = FMath::Max(OutDuration, CurrentInterval.End);
				continue;
			}

			if (Line.StartsWith(TEXT("text =")))
			{
				CurrentInterval.Text = ParseTextGridStringValue(Line);
				continue;
			}
		}
	}

	int32 BestTierIndex = INDEX_NONE;
	for (int32 TierIndex = 0; TierIndex < Tiers.Num(); ++TierIndex)
	{
		const FTier& Tier = Tiers[TierIndex];
		if (!Tier.Intervals.IsEmpty() && Tier.Name.Contains(TEXT("phone"), ESearchCase::IgnoreCase))
		{
			BestTierIndex = TierIndex;
			break;
		}
	}

	if (BestTierIndex == INDEX_NONE)
	{
		for (int32 TierIndex = Tiers.Num() - 1; TierIndex >= 0; --TierIndex)
		{
			const FTier& Tier = Tiers[TierIndex];
			if (!Tier.Intervals.IsEmpty() &&
				(Tier.ClassName.IsEmpty() || Tier.ClassName.Equals(TEXT("IntervalTier"), ESearchCase::IgnoreCase)))
			{
				BestTierIndex = TierIndex;
				break;
			}
		}
	}

	if (BestTierIndex == INDEX_NONE)
	{
		OutMessage = TEXT("No interval tier was found in the TextGrid.");
		return false;
	}

	OutIntervals = Tiers[BestTierIndex].Intervals;
	OutIntervals.RemoveAll([](const FInterval& Interval)
	{
		return Interval.End <= Interval.Start;
	});

	if (OutIntervals.IsEmpty())
	{
		OutMessage = TEXT("The selected TextGrid tier contains no valid intervals.");
		return false;
	}

	if (OutDuration <= 0.f)
	{
		for (const FInterval& Interval : OutIntervals)
		{
			OutDuration = FMath::Max(OutDuration, Interval.End);
		}
	}

	return true;
}

TArray<FCue> BuildCuesFromIntervals(const TArray<FInterval>& Intervals)
{
	TArray<FCue> Cues;
	Cues.Reserve(Intervals.Num());

	for (const FInterval& Interval : Intervals)
	{
		const FName Viseme = MapPhoneToViseme(Interval.Text);
		if (!Cues.IsEmpty() && Cues.Last().Viseme == Viseme && FMath::IsNearlyEqual(Cues.Last().End, Interval.Start, 0.001f))
		{
			Cues.Last().End = Interval.End;
			continue;
		}

		FCue& Cue = Cues.AddDefaulted_GetRef();
		Cue.Start = Interval.Start;
		Cue.End = Interval.End;
		Cue.Viseme = Viseme;
	}

	return Cues;
}

TArray<FDreamLipSyncMorphWeight> BuildWeightsForViseme(const UDreamLipSyncClip* Clip, FName Viseme)
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

TArray<FDreamLipSyncMorphFrame> BuildMorphFrames(const UDreamLipSyncClip* Clip, const TArray<FCue>& Cues, float BlendTime)
{
	TArray<FDreamLipSyncMorphFrame> Frames;
	Frames.Reserve(Cues.Num() * 2);

	for (const FCue& Cue : Cues)
	{
		if (Cue.End <= Cue.Start)
		{
			continue;
		}

		AddFrame(Frames, Cue.Start, BuildWeightsForViseme(Clip, Cue.Viseme));

		const float HoldEnd = Cue.End - FMath::Max(0.f, BlendTime);
		if (HoldEnd > Cue.Start + 0.001f)
		{
			AddFrame(Frames, HoldEnd, BuildWeightsForViseme(Clip, Cue.Viseme));
		}
	}

	return Frames;
}

void FindTextGridFiles(const FString& Directory, TArray<FString>& OutFiles)
{
	IFileManager::Get().FindFilesRecursive(OutFiles, *Directory, TEXT("*.TextGrid"), true, false);

	TArray<FString> LowercaseFiles;
	IFileManager::Get().FindFilesRecursive(LowercaseFiles, *Directory, TEXT("*.textgrid"), true, false);
	for (const FString& File : LowercaseFiles)
	{
		OutFiles.AddUnique(File);
	}
}
}

bool FDreamLipSyncMfaGenerator::GenerateClipFromMfa(UDreamLipSyncClip* Clip, FString& OutMessage)
{
	if (!Clip)
	{
		OutMessage = TEXT("Invalid DreamLipSyncClip.");
		return false;
	}

	FString MfaCommand;
	if (!ResolveMfaCommand(MfaCommand, OutMessage))
	{
		return false;
	}

	FString AudioFilePath;
	if (!ResolveInputAudioFile(Clip, AudioFilePath, OutMessage))
	{
		return false;
	}

	const FString DialogueText = Clip->DialogueText.ToString();
	if (DialogueText.TrimStartAndEnd().IsEmpty())
	{
		OutMessage = TEXT("MFA generation requires DialogueText. MFA needs a transcript matching the audio.");
		return false;
	}

	const UDreamLipSyncSettings* Settings = UDreamLipSyncSettings::Get();
	const FString Dictionary = Settings && !Settings->MfaDictionary.IsEmpty() ? Settings->MfaDictionary : TEXT("mandarin_china_mfa");
	const FString AcousticModel = Settings && !Settings->MfaAcousticModel.IsEmpty() ? Settings->MfaAcousticModel : TEXT("mandarin_mfa");
	const FString MfaRootDirectory = Settings ? Settings->MfaRootDirectory.Path : FString();

	const FString WorkRoot = FPaths::ConvertRelativePathToFull(FPaths::ProjectIntermediateDir() / TEXT("DreamLipSync") / TEXT("MFA") / FGuid::NewGuid().ToString(EGuidFormats::Digits));
	const FString CorpusDir = WorkRoot / TEXT("corpus");
	const FString OutputDir = WorkRoot / TEXT("aligned");

	IFileManager::Get().MakeDirectory(*CorpusDir, true);
	IFileManager::Get().MakeDirectory(*OutputDir, true);

	const FString FileBase = DreamLipSyncMfa::SanitizeFileBase(Clip->GetName());
	const FString AudioExtension = FPaths::GetExtension(AudioFilePath, false);
	const FString CorpusAudioPath = CorpusDir / FString::Printf(TEXT("%s.%s"), *FileBase, *AudioExtension);
	const FString TranscriptPath = CorpusDir / FString::Printf(TEXT("%s.lab"), *FileBase);

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.CopyFile(*CorpusAudioPath, *AudioFilePath))
	{
		OutMessage = FString::Printf(TEXT("Could not copy audio into MFA corpus:\n%s"), *CorpusAudioPath);
		return false;
	}

	if (!FFileHelper::SaveStringToFile(DialogueText, *TranscriptPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		OutMessage = FString::Printf(TEXT("Could not write MFA transcript:\n%s"), *TranscriptPath);
		return false;
	}

	FString Params = TEXT("align");
	DreamLipSyncMfa::AppendMfaOptions(Params, Settings);

	Params += FString::Printf(
		TEXT(" %s %s %s %s"),
		*DreamLipSyncMfa::Quote(CorpusDir),
		*DreamLipSyncMfa::Quote(Dictionary),
		*DreamLipSyncMfa::Quote(AcousticModel),
		*DreamLipSyncMfa::Quote(OutputDir));

	const FString OriginalPath = FPlatformMisc::GetEnvironmentVariable(TEXT("PATH"));
	const FString OriginalMfaRootDirectory = FPlatformMisc::GetEnvironmentVariable(TEXT("MFA_ROOT_DIR"));
	const FString OriginalPythonUtf8 = FPlatformMisc::GetEnvironmentVariable(TEXT("PYTHONUTF8"));
	const FString OriginalPythonIoEncoding = FPlatformMisc::GetEnvironmentVariable(TEXT("PYTHONIOENCODING"));
	bool bPathWasAdjusted = false;
	bool bMfaRootWasAdjusted = false;
	if (FPaths::FileExists(MfaCommand))
	{
		const FString ScriptsDir = FPaths::GetPath(MfaCommand);
		const FString EnvRootDir = FPaths::GetPath(ScriptsDir);
		const FString LibraryBinDir = EnvRootDir / TEXT("Library") / TEXT("bin");
		const FString EnvPathPrefix = EnvRootDir + TEXT(";") + ScriptsDir + TEXT(";") + LibraryBinDir;
		if (!OriginalPath.Contains(ScriptsDir, ESearchCase::IgnoreCase))
		{
			FPlatformMisc::SetEnvironmentVar(TEXT("PATH"), *(EnvPathPrefix + TEXT(";") + OriginalPath));
			bPathWasAdjusted = true;
		}
	}
	if (!MfaRootDirectory.IsEmpty())
	{
		const FString FullMfaRootDirectory = FPaths::ConvertRelativePathToFull(MfaRootDirectory);
		IFileManager::Get().MakeDirectory(*FullMfaRootDirectory, true);
		FPlatformMisc::SetEnvironmentVar(TEXT("MFA_ROOT_DIR"), *FullMfaRootDirectory);
		bMfaRootWasAdjusted = true;
	}

	FPlatformMisc::SetEnvironmentVar(TEXT("PYTHONUTF8"), TEXT("1"));
	FPlatformMisc::SetEnvironmentVar(TEXT("PYTHONIOENCODING"), TEXT("utf-8"));

	FDreamLipSyncProcessResult ProcessResult;
	const bool bStarted = FDreamLipSyncProcessRunner::RunWithProgress(
		MfaCommand,
		Params,
		WorkRoot,
		FText::Format(LOCTEXT("GeneratingDreamLipSyncFromMfaProgress", "Generating lip sync for {0}"), FText::FromString(Clip->GetName())),
		LOCTEXT("RunningMfaProgress", "Running Montreal Forced Aligner. This can take a while for first-time model setup."),
		ProcessResult);

	if (bPathWasAdjusted)
	{
		FPlatformMisc::SetEnvironmentVar(TEXT("PATH"), *OriginalPath);
	}
	if (bMfaRootWasAdjusted)
	{
		FPlatformMisc::SetEnvironmentVar(TEXT("MFA_ROOT_DIR"), *OriginalMfaRootDirectory);
	}
	FPlatformMisc::SetEnvironmentVar(TEXT("PYTHONUTF8"), *OriginalPythonUtf8);
	FPlatformMisc::SetEnvironmentVar(TEXT("PYTHONIOENCODING"), *OriginalPythonIoEncoding);
	if (ProcessResult.bCanceled)
	{
		OutMessage = TEXT("MFA generation was canceled.");
		return false;
	}

	if (!bStarted || ProcessResult.ReturnCode != 0)
	{
		OutMessage = FString::Printf(TEXT("MFA failed. ReturnCode=%d\n%s\n%s"), ProcessResult.ReturnCode, *ProcessResult.StdErr, *ProcessResult.StdOut);
		if (OutMessage.Contains(TEXT("spacy-pkuseg")) ||
			OutMessage.Contains(TEXT("Chinese tokenization support"), ESearchCase::IgnoreCase))
		{
			OutMessage += TEXT("\n\nChinese MFA models require extra tokenizer packages. Recommended fix:\n")
				TEXT("conda create -n aligner310 -c conda-forge python=3.10 montreal-forced-aligner\n")
				TEXT("conda activate aligner310\n")
				TEXT("python -m pip install spacy-pkuseg dragonmapper hanziconv\n")
				TEXT("mfa model download acoustic mandarin_mfa\n")
				TEXT("mfa model download dictionary mandarin_china_mfa\n")
				TEXT("Then set Mfa Command to the new aligner310\\Scripts\\mfa.exe path.");
		}
		return false;
	}

	TArray<FString> TextGridFiles;
	DreamLipSyncMfa::FindTextGridFiles(OutputDir, TextGridFiles);
	if (TextGridFiles.IsEmpty())
	{
		OutMessage = FString::Printf(TEXT("MFA finished, but no TextGrid files were found under:\n%s\n%s"), *OutputDir, *ProcessResult.StdOut);
		return false;
	}

	FString TextGridPath = TextGridFiles[0];
	for (const FString& Candidate : TextGridFiles)
	{
		if (FPaths::GetBaseFilename(Candidate).Equals(FileBase, ESearchCase::IgnoreCase))
		{
			TextGridPath = Candidate;
			break;
		}
	}

	const bool bImported = ImportMfaTextGridFile(Clip, TextGridPath, OutMessage);
	if (!bImported)
	{
		OutMessage += FString::Printf(TEXT("\nMFA output file:\n%s"), *TextGridPath);
		return false;
	}

	OutMessage = FString::Printf(TEXT("Generated %d MorphFrames from MFA TextGrid."), Clip->MorphFrames.Num());
	return true;
}

bool FDreamLipSyncMfaGenerator::ImportMfaTextGridFile(UDreamLipSyncClip* Clip, const FString& TextGridFilePath, FString& OutMessage)
{
	FString TextGridString;
	if (!FFileHelper::LoadFileToString(TextGridString, *TextGridFilePath))
	{
		OutMessage = FString::Printf(TEXT("Could not read MFA TextGrid file:\n%s"), *TextGridFilePath);
		return false;
	}

	return ImportMfaTextGridString(Clip, TextGridString, OutMessage);
}

bool FDreamLipSyncMfaGenerator::ImportMfaTextGridString(UDreamLipSyncClip* Clip, const FString& TextGridString, FString& OutMessage)
{
	if (!Clip)
	{
		OutMessage = TEXT("Invalid DreamLipSyncClip.");
		return false;
	}

	TArray<DreamLipSyncMfa::FInterval> Intervals;
	float Duration = 0.f;
	if (!DreamLipSyncMfa::ParseTextGrid(TextGridString, Intervals, Duration, OutMessage))
	{
		return false;
	}

	const TArray<DreamLipSyncMfa::FCue> Cues = DreamLipSyncMfa::BuildCuesFromIntervals(Intervals);
	const UDreamLipSyncSettings* Settings = UDreamLipSyncSettings::Get();
	const float BlendTime = Clip->GenerationBlendTime >= 0.f ? Clip->GenerationBlendTime : (Settings ? Settings->BlendTime : 0.04f);
	TArray<FDreamLipSyncMorphFrame> Frames = DreamLipSyncMfa::BuildMorphFrames(Clip, Cues, BlendTime);
	if (Frames.IsEmpty())
	{
		OutMessage = TEXT("TextGrid parsed, but no MorphFrames could be generated. Check VisemeMappings on the clip.");
		return false;
	}

	const FScopedTransaction Transaction(LOCTEXT("GenerateDreamLipSyncFromMfa", "Generate Dream Lip Sync From MFA"));
	Clip->Modify();
	Clip->DataMode = EDreamLipSyncDataMode::MorphFrames;
	Clip->MorphFrames = MoveTemp(Frames);
	Clip->VisemeKeys.Reset();
	Clip->Duration = Duration;
	Clip->bHoldLastKey = false;
	Clip->MarkPackageDirty();

	OutMessage = FString::Printf(TEXT("Imported %d TextGrid intervals and generated %d MorphFrames."), Intervals.Num(), Clip->MorphFrames.Num());
	return true;
}

bool FDreamLipSyncMfaGenerator::ResolveInputAudioFile(const UDreamLipSyncClip* Clip, FString& OutAudioFilePath, FString& OutMessage)
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

	return true;
}

bool FDreamLipSyncMfaGenerator::ResolveMfaCommand(FString& OutCommand, FString& OutMessage)
{
	const UDreamLipSyncSettings* Settings = UDreamLipSyncSettings::Get();
	OutCommand = Settings ? Settings->MfaCommand : FString();
	OutCommand.TrimStartAndEndInline();

	if (OutCommand.IsEmpty())
	{
		OutCommand = TEXT("mfa");
		return true;
	}

	const bool bLooksLikePath = OutCommand.Contains(TEXT("\\")) || OutCommand.Contains(TEXT("/")) || OutCommand.EndsWith(TEXT(".exe"), ESearchCase::IgnoreCase);
	if (bLooksLikePath)
	{
		OutCommand = FPaths::ConvertRelativePathToFull(OutCommand);
		if (!FPaths::FileExists(OutCommand))
		{
			OutMessage = FString::Printf(TEXT("MFA command was not found:\n%s"), *OutCommand);
			return false;
		}
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
