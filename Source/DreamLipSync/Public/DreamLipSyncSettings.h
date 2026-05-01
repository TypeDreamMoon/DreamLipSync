// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DreamLipSyncTypes.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPath.h"
#include "DreamLipSyncSettings.generated.h"

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Dream Lip Sync"))
class DREAMLIPSYNC_API UDreamLipSyncSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UDreamLipSyncSettings* Get();

	virtual FName GetCategoryName() const override;

	UPROPERTY(config, EditAnywhere, Category = "Rhubarb", meta = (FilePathFilter = "exe"))
	FFilePath RhubarbExecutablePath;

	UPROPERTY(config, EditAnywhere, Category = "Rhubarb")
	EDreamLipSyncRhubarbRecognizer RhubarbRecognizer = EDreamLipSyncRhubarbRecognizer::Auto;

	UPROPERTY(config, EditAnywhere, Category = "Rhubarb")
	FString ExtendedShapes = TEXT("GHX");

	UPROPERTY(config, EditAnywhere, Category = "Rhubarb", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BlendTime = 0.04f;

	UPROPERTY(config, EditAnywhere, Category = "Generation")
	FDreamLipSyncFrameGenerationSettings FrameGenerationSettings;

	UPROPERTY(config, EditAnywhere, Category = "NVIDIA ACE")
	bool bEnableAceGeneration = true;

	UPROPERTY(config, EditAnywhere, Category = "NVIDIA ACE", meta = (EditCondition = "bEnableAceGeneration", GetOptions = "DreamLipSyncEditor.DreamLipSyncAceProviderOptions.GetAceProviderOptions"))
	FName AceProviderName = TEXT("Default");

	UPROPERTY(config, EditAnywhere, Category = "NVIDIA ACE", meta = (EditCondition = "bEnableAceGeneration"))
	bool bAceForceBurstMode = true;

	UPROPERTY(config, EditAnywhere, Category = "NVIDIA ACE", meta = (ClampMin = "1.0", UIMin = "1.0", EditCondition = "bEnableAceGeneration"))
	float AceGenerationTimeoutSeconds = 300.f;

	UPROPERTY(config, EditAnywhere, Category = "NVIDIA ACE|Emotion Parameters", meta = (EditCondition = "bEnableAceGeneration"))
	bool bAceUseEmotionParameters = false;

	UPROPERTY(config, EditAnywhere, Category = "NVIDIA ACE|Emotion Parameters", meta = (EditCondition = "bEnableAceGeneration && bAceUseEmotionParameters"))
	FDreamLipSyncAceEmotionParameters AceEmotionParameters;

	UPROPERTY(config, EditAnywhere, Category = "MFA")
	FString MfaCommand = TEXT("mfa");

	UPROPERTY(config, EditAnywhere, Category = "MFA")
	FDirectoryPath MfaRootDirectory;

	UPROPERTY(config, EditAnywhere, Category = "MFA")
	FString MfaDictionary = TEXT("mandarin_china_mfa");

	UPROPERTY(config, EditAnywhere, Category = "MFA")
	FString MfaAcousticModel = TEXT("mandarin_mfa");

	UPROPERTY(config, EditAnywhere, Category = "MFA|Advanced")
	FFilePath MfaConfigPath;

	UPROPERTY(config, EditAnywhere, Category = "MFA|Advanced")
	FString MfaSpeakerCharacters;

	UPROPERTY(config, EditAnywhere, Category = "MFA|Advanced")
	FString MfaG2PModelPath;

	UPROPERTY(config, EditAnywhere, Category = "MFA", meta = (ClampMin = "1", UIMin = "1"))
	int32 MfaNumJobs = 1;

	UPROPERTY(config, EditAnywhere, Category = "MFA|Advanced")
	bool bMfaClean = true;

	UPROPERTY(config, EditAnywhere, Category = "MFA|Advanced")
	bool bMfaOverwrite = true;

	UPROPERTY(config, EditAnywhere, Category = "MFA|Advanced")
	bool bMfaQuiet = true;

	UPROPERTY(config, EditAnywhere, Category = "MFA|Advanced")
	bool bMfaVerbose = false;

	UPROPERTY(config, EditAnywhere, Category = "MFA|Advanced")
	bool bMfaFinalClean = false;

	UPROPERTY(config, EditAnywhere, Category = "MFA|Advanced")
	bool bMfaUseMultiprocessing = true;

	UPROPERTY(config, EditAnywhere, Category = "MFA|Advanced")
	bool bMfaUseThreading = false;

	UPROPERTY(config, EditAnywhere, Category = "MFA|Advanced")
	bool bMfaUsePostgres = false;

	UPROPERTY(config, EditAnywhere, Category = "MFA|Advanced")
	bool bMfaSingleSpeaker = false;

	UPROPERTY(config, EditAnywhere, Category = "MFA|Advanced")
	bool bMfaNoTokenization = false;

	UPROPERTY(config, EditAnywhere, Category = "MFA|Advanced")
	bool bMfaTextGridCleanup = true;

	UPROPERTY(config, EditAnywhere, Category = "MFA|Advanced")
	bool bMfaIncludeOriginalText = false;

	UPROPERTY(config, EditAnywhere, Category = "MFA|Advanced")
	bool bMfaFineTune = false;

	UPROPERTY(config, EditAnywhere, Category = "MFA|Advanced", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bMfaFineTune"))
	float MfaFineTuneBoundaryTolerance = 0.f;

	UPROPERTY(config, EditAnywhere, Category = "MFA|Advanced")
	FString MfaExtraArguments;
};
