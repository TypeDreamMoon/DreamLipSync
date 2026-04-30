// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/SoftObjectPath.h"
#include "DreamLipSyncTypes.h"
#include "DreamLipSyncClip.generated.h"

class USoundBase;

UCLASS(BlueprintType)
class DREAMLIPSYNC_API UDreamLipSyncClip : public UDataAsset
{
	GENERATED_BODY()

public:
	UDreamLipSyncClip();

	UFUNCTION(BlueprintPure, Category = "Dream Lip Sync")
	float GetResolvedDuration() const;

	UFUNCTION(BlueprintPure, Category = "Dream Lip Sync")
	void GetMorphWeightsAtTime(float Time, TArray<FDreamLipSyncMorphWeight>& OutWeights) const;

	UFUNCTION(BlueprintPure, Category = "Dream Lip Sync")
	void GetControlledMorphTargetNames(TArray<FName>& OutMorphTargetNames) const;

	UFUNCTION(BlueprintPure, Category = "Dream Lip Sync")
	void BuildMorphWeightsForViseme(FName Viseme, float Weight, TArray<FDreamLipSyncMorphWeight>& OutWeights) const;

	void EvaluateAtTime(float Time, TMap<FName, float>& OutWeights) const;
	void GetControlledMorphTargets(TSet<FName>& OutMorphTargetNames) const;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lip Sync")
	TObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	FString Locale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Duration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	EDreamLipSyncDataMode DataMode = EDreamLipSyncDataMode::VisemeKeys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	bool bHoldLastKey = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Timing", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float PlaybackBlendTime = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Generation", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float GenerationBlendTime = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Viseme")
	TArray<FDreamLipSyncVisemeMapping> VisemeMappings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Viseme")
	TArray<FDreamLipSyncVisemeRecipe> VisemeRecipes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Viseme")
	TArray<FDreamLipSyncVisemeKey> VisemeKeys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Morph Frame")
	TArray<FDreamLipSyncMorphFrame> MorphFrames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|External Import")
	TArray<FDreamLipSyncSourceMorphMapping> SourceMorphMappings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Generation", meta = (FilePathFilter = "wav;ogg"))
	FFilePath SourceAudioFileOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Generation", meta = (MultiLine = true))
	FText DialogueText;

private:
	void EvaluateVisemeKeys(float Time, TMap<FName, float>& OutWeights) const;
	void EvaluateMorphFrames(float Time, TMap<FName, float>& OutWeights) const;
	void AddMappedVisemeWeight(FName Viseme, float Weight, TMap<FName, float>& OutWeights) const;
	void AddRawMappedVisemeWeight(FName Viseme, float Weight, TMap<FName, float>& OutWeights) const;
	const FDreamLipSyncVisemeRecipe* FindVisemeRecipe(FName Viseme) const;
};
