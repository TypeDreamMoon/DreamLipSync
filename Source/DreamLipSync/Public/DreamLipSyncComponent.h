// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DreamLipSyncComponent.generated.h"

class UAudioComponent;
class UDreamLipSyncClip;
class USkeletalMeshComponent;

UCLASS(ClassGroup = (Dream), meta = (BlueprintSpawnableComponent))
class DREAMLIPSYNC_API UDreamLipSyncComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDreamLipSyncComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Dream Lip Sync")
	void PlayLipSyncClip(UDreamLipSyncClip* InClip, bool bPlayAudio = true, float StartTime = 0.f, float InPlaybackRate = 1.f);

	UFUNCTION(BlueprintCallable, Category = "Dream Lip Sync")
	void StopLipSync(bool bResetMorphTargets = true, bool bStopAudio = true);

	UFUNCTION(BlueprintCallable, Category = "Dream Lip Sync")
	void SetLipSyncPlaybackTime(float NewTime, bool bApplyImmediately = true);

	UFUNCTION(BlueprintPure, Category = "Dream Lip Sync")
	bool IsLipSyncPlaying() const { return bPlaying; }

	UFUNCTION(BlueprintPure, Category = "Dream Lip Sync")
	float GetLipSyncPlaybackTime() const { return PlaybackTime; }

	UFUNCTION(BlueprintPure, Category = "Dream Lip Sync")
	UDreamLipSyncClip* GetActiveLipSyncClip() const { return ActiveClip; }

protected:
	USkeletalMeshComponent* ResolveTargetMesh() const;
	UAudioComponent* ResolveAudioComponent() const;
	void ApplyCurrentFrame();
	void StartClipAudio(float StartTime);
	void StopClipAudio();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	TObjectPtr<USkeletalMeshComponent> TargetMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	TObjectPtr<UAudioComponent> AudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	TObjectPtr<UDreamLipSyncClip> DefaultClip;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	bool bAutoPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	bool bResetControlledMorphTargetsEachFrame = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	bool bResetMorphTargetsOnStop = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WeightScale = 1.f;

protected:
	UPROPERTY(Transient)
	TObjectPtr<UDreamLipSyncClip> ActiveClip;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ManagedAudioComponent;

	float PlaybackTime = 0.f;
	float PlaybackRate = 1.f;
	bool bPlaying = false;
};
