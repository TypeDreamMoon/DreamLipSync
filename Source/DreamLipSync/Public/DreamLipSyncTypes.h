// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DreamLipSyncTypes.generated.h"

UENUM(BlueprintType)
enum class EDreamLipSyncDataMode : uint8
{
	VisemeKeys UMETA(DisplayName = "Viseme Keys"),
	MorphFrames UMETA(DisplayName = "Morph Frames")
};

UENUM(BlueprintType)
enum class EDreamLipSyncRhubarbRecognizer : uint8
{
	Auto UMETA(DisplayName = "Auto"),
	PocketSphinx UMETA(DisplayName = "PocketSphinx (English)"),
	Phonetic UMETA(DisplayName = "Phonetic (Language Independent)")
};

USTRUCT(BlueprintType)
struct DREAMLIPSYNC_API FDreamLipSyncMorphWeight
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	FName MorphTargetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Weight = 0.f;
};

USTRUCT(BlueprintType)
struct DREAMLIPSYNC_API FDreamLipSyncMorphFrame
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Time = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	TArray<FDreamLipSyncMorphWeight> Weights;
};

USTRUCT(BlueprintType)
struct DREAMLIPSYNC_API FDreamLipSyncVisemeKey
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Time = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	FName Viseme = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Weight = 1.f;
};

USTRUCT(BlueprintType)
struct DREAMLIPSYNC_API FDreamLipSyncVisemeMapping
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	FName Viseme = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	FName MorphTargetName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Scale = 1.f;
};

USTRUCT(BlueprintType)
struct DREAMLIPSYNC_API FDreamLipSyncVisemeRecipeMix
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	FName Viseme = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Scale = 1.f;
};

USTRUCT(BlueprintType)
struct DREAMLIPSYNC_API FDreamLipSyncVisemeRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	FName SourceViseme = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	bool bIncludeBaseViseme = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bIncludeBaseViseme"))
	float BaseScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	TArray<FDreamLipSyncVisemeRecipeMix> VisemeMixes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	TArray<FDreamLipSyncMorphWeight> ExtraMorphWeights;
};

USTRUCT(BlueprintType)
struct DREAMLIPSYNC_API FDreamLipSyncSourceMorphMapping
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	FName SourceName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	FName MorphTargetName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Scale = 1.f;
};

USTRUCT(BlueprintType)
struct DREAMLIPSYNC_API FDreamLipSyncFrameGenerationSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "-1.0", ClampMax = "1.0", UIMin = "-0.25", UIMax = "0.25"))
	float TimeOffsetSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MinCueDuration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CueEndPadding = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0"))
	float WeightScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0"))
	float MaxWeight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	FName NeutralViseme = TEXT("Neutral");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	bool bInsertNeutralFramesInGaps = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bInsertNeutralFramesInGaps"))
	float NeutralGapThreshold = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bInsertNeutralFramesInGaps"))
	float NeutralBlendTime = 0.03f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	bool bAddNeutralFrameAtStart = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	bool bAddNeutralFrameAtEnd = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Bake")
	bool bBakeFixedFrameRate = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Bake", meta = (ClampMin = "1.0", ClampMax = "240.0", UIMin = "24.0", UIMax = "120.0", EditCondition = "bBakeFixedFrameRate"))
	float BakeFrameRate = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Smoothing", meta = (ClampMin = "0", ClampMax = "8", UIMin = "0", UIMax = "4"))
	int32 SmoothingPasses = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Smoothing", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "SmoothingPasses > 0"))
	float SmoothingStrength = 0.35f;
};

USTRUCT(BlueprintType)
struct DREAMLIPSYNC_API FDreamLipSyncRhubarbGenerationSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	bool bOverrideProjectSettings = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (EditCondition = "bOverrideProjectSettings"))
	EDreamLipSyncRhubarbRecognizer Recognizer = EDreamLipSyncRhubarbRecognizer::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (EditCondition = "bOverrideProjectSettings"))
	FString ExtendedShapes = TEXT("GHX");
};

USTRUCT(BlueprintType)
struct DREAMLIPSYNC_API FDreamLipSyncMfaGenerationSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	bool bOverrideProjectSettings = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (EditCondition = "bOverrideProjectSettings"))
	FDirectoryPath MfaRootDirectory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (EditCondition = "bOverrideProjectSettings"))
	FString MfaDictionary = TEXT("mandarin_china_mfa");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (EditCondition = "bOverrideProjectSettings"))
	FString MfaAcousticModel = TEXT("mandarin_mfa");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "1", UIMin = "1", EditCondition = "bOverrideProjectSettings"))
	int32 MfaNumJobs = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Advanced", meta = (EditCondition = "bOverrideProjectSettings"))
	FFilePath MfaConfigPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Advanced", meta = (EditCondition = "bOverrideProjectSettings"))
	FString MfaSpeakerCharacters;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Advanced", meta = (EditCondition = "bOverrideProjectSettings"))
	FString MfaG2PModelPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Advanced", meta = (EditCondition = "bOverrideProjectSettings"))
	bool bMfaClean = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Advanced", meta = (EditCondition = "bOverrideProjectSettings"))
	bool bMfaOverwrite = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Advanced", meta = (EditCondition = "bOverrideProjectSettings"))
	bool bMfaQuiet = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Advanced", meta = (EditCondition = "bOverrideProjectSettings"))
	bool bMfaVerbose = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Advanced", meta = (EditCondition = "bOverrideProjectSettings"))
	bool bMfaFinalClean = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Advanced", meta = (EditCondition = "bOverrideProjectSettings"))
	bool bMfaUseMultiprocessing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Advanced", meta = (EditCondition = "bOverrideProjectSettings"))
	bool bMfaUseThreading = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Advanced", meta = (EditCondition = "bOverrideProjectSettings"))
	bool bMfaUsePostgres = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Advanced", meta = (EditCondition = "bOverrideProjectSettings"))
	bool bMfaSingleSpeaker = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Advanced", meta = (EditCondition = "bOverrideProjectSettings"))
	bool bMfaNoTokenization = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Advanced", meta = (EditCondition = "bOverrideProjectSettings"))
	bool bMfaTextGridCleanup = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Advanced", meta = (EditCondition = "bOverrideProjectSettings"))
	bool bMfaIncludeOriginalText = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Advanced", meta = (EditCondition = "bOverrideProjectSettings"))
	bool bMfaFineTune = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Advanced", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bOverrideProjectSettings && bMfaFineTune"))
	float MfaFineTuneBoundaryTolerance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Advanced", meta = (EditCondition = "bOverrideProjectSettings"))
	FString MfaExtraArguments;
};

USTRUCT(BlueprintType)
struct DREAMLIPSYNC_API FDreamLipSyncAceEmotionOverride
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Emotion Overrides")
	bool bOverrideAmazement = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Emotion Overrides", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bOverrideAmazement"))
	float Amazement = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Emotion Overrides")
	bool bOverrideAnger = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Emotion Overrides", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bOverrideAnger"))
	float Anger = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Emotion Overrides")
	bool bOverrideCheekiness = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Emotion Overrides", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bOverrideCheekiness"))
	float Cheekiness = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Emotion Overrides")
	bool bOverrideDisgust = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Emotion Overrides", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bOverrideDisgust"))
	float Disgust = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Emotion Overrides")
	bool bOverrideFear = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Emotion Overrides", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bOverrideFear"))
	float Fear = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Emotion Overrides")
	bool bOverrideGrief = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Emotion Overrides", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bOverrideGrief"))
	float Grief = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Emotion Overrides")
	bool bOverrideJoy = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Emotion Overrides", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bOverrideJoy"))
	float Joy = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Emotion Overrides")
	bool bOverrideOutOfBreath = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Emotion Overrides", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bOverrideOutOfBreath"))
	float OutOfBreath = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Emotion Overrides")
	bool bOverridePain = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Emotion Overrides", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bOverridePain"))
	float Pain = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Emotion Overrides")
	bool bOverrideSadness = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Emotion Overrides", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bOverrideSadness"))
	float Sadness = 0.f;
};

USTRUCT(BlueprintType)
struct DREAMLIPSYNC_API FDreamLipSyncAceEmotionParameters
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Emotion Parameters", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float OverallEmotionStrength = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Detected Emotion", meta = (ClampMin = "0.3", ClampMax = "3.0", UIMin = "0.3", UIMax = "3.0"))
	float DetectedEmotionContrast = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Detected Emotion", meta = (ClampMin = "1", ClampMax = "6", UIMin = "1", UIMax = "6"))
	int32 MaxDetectedEmotions = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Detected Emotion", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float DetectedEmotionSmoothing = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Application Overrides", meta = (InlineEditConditionToggle))
	bool bEnableEmotionOverride = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Application Overrides", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bEnableEmotionOverride"))
	float EmotionOverrideStrength = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Application Overrides", meta = (EditCondition = "bEnableEmotionOverride && (EmotionOverrideStrength > 0.0)"))
	FDreamLipSyncAceEmotionOverride EmotionOverrides;
};

USTRUCT(BlueprintType)
struct DREAMLIPSYNC_API FDreamLipSyncAceGenerationSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	bool bOverrideProjectSettings = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (EditCondition = "bOverrideProjectSettings", GetOptions = "DreamLipSyncEditor.DreamLipSyncAceProviderOptions.GetAceProviderOptions"))
	FName AceProviderName = TEXT("Default");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (EditCondition = "bOverrideProjectSettings"))
	bool bAceForceBurstMode = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "1.0", UIMin = "1.0", EditCondition = "bOverrideProjectSettings"))
	float AceGenerationTimeoutSeconds = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Emotion Parameters", meta = (EditCondition = "bOverrideProjectSettings"))
	bool bAceUseEmotionParameters = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Emotion Parameters", meta = (EditCondition = "bOverrideProjectSettings && bAceUseEmotionParameters"))
	FDreamLipSyncAceEmotionParameters AceEmotionParameters;
};
