// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sections/MovieSceneHookSection.h"
#include "DreamLipSyncSection.generated.h"

class UDreamLipSyncClip;
class USkeletalMeshComponent;

UCLASS(BlueprintType)
class DREAMLIPSYNC_API UDreamLipSyncSection : public UMovieSceneHookSection
{
	GENERATED_BODY()

public:
	UDreamLipSyncSection(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void Begin(TSharedRef<FSharedPlaybackState> SharedPlaybackState, const UE::MovieScene::FEvaluationHookParams& Params) const override;
	virtual void Update(TSharedRef<FSharedPlaybackState> SharedPlaybackState, const UE::MovieScene::FEvaluationHookParams& Params) const override;
	virtual void End(TSharedRef<FSharedPlaybackState> SharedPlaybackState, const UE::MovieScene::FEvaluationHookParams& Params) const override;
	virtual TOptional<TRange<FFrameNumber>> GetAutoSizeRange() const override;
	virtual void SetStartFrame(TRangeBound<FFrameNumber> NewStartFrame) override;
	virtual void SetEndFrame(TRangeBound<FFrameNumber> NewEndFrame) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UDreamLipSyncClip* GetClip() const { return Clip; }
	void SetClip(UDreamLipSyncClip* InClip);

	UFUNCTION(BlueprintCallable, Category = "Dream Lip Sync")
	void AutoSizeToClipDuration();

protected:
	void ApplyAtCurrentTime(TSharedRef<FSharedPlaybackState> SharedPlaybackState, const UE::MovieScene::FEvaluationHookParams& Params) const;
	void ResetBoundTargets(TSharedRef<FSharedPlaybackState> SharedPlaybackState, const UE::MovieScene::FEvaluationHookParams& Params) const;
	float GetLocalLipSyncTime(const UE::MovieScene::FEvaluationHookParams& Params) const;
	float GetNaturalSectionDuration() const;
	FFrameNumber GetNaturalSectionFrameCount() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	TObjectPtr<UDreamLipSyncClip> Clip;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float StartOffset = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.001", UIMin = "0.001"))
	float TimeScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Timing")
	bool bAutoSizeToClipDuration = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Timing")
	bool bLockSectionLengthToClipDuration = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Timing")
	bool bClampEvaluationToClipDuration = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync|Timing")
	bool bStretchClipToSection = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WeightScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	bool bResetControlledMorphTargetsEachFrame = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	bool bResetMorphTargetsOnEnd = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	bool bEvaluateWhenSilent = true;
};
